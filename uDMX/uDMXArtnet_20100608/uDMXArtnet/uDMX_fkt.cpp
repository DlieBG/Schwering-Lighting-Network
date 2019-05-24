// ===== uDMX_fkt.cpp =====

/*
  Copyright (C) 2007 Lutz (ilLU[TZ]minator)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define _MULTI_THREADED

#ifndef WIN32
#include <pthread.h>
#include <math.h>
#define min(x,y) ((x < y) ? x : y)
#else
#include <process.h>
#endif

#ifndef WIN32
// === defines to make it compatible with Win32
#define MAX_PATH    256
#define LOBYTE(x)   (x & 0xFF)
#define HIBYTE(x)   ((x & 0xFF00) >> 8)
#endif

// === "normal" includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef WIN32
#include <termios.h>
#include <unistd.h>
#endif
#include <time.h>
#include <signal.h>    /* signal name macros, and the signal() prototype */
#include <setjmp.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <artnet/artnet.h>
#include "uDMX_fkt.h"

/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */
#define USBDEV_SHARED_VENDOR    0x16C0  /* VOTI */
#define USBDEV_SHARED_PRODUCT   0x05DC  /* Obdev's free shared PID */

// === uDMX specific
#include "uDMX_cmds.h"

#define DMX_CHANS     512

#ifdef WIN32
#include <Windows.h>
#include <conio.h>
#endif


// dbgprintf() - common function for debug out
void dbgprintf(pUIParams pUI, int iTrace, const char *fmt, ... )
{
  char Buffer[512] ;

  va_list args ;
  va_start(args, fmt ) ;
	
	vsnprintf(Buffer, sizeof(Buffer), fmt, args);  
	pUI->cUI->Trace(pUI, iTrace, Buffer) ;
	
  va_end(args) ;
}

// SLEEP() - make common wait function for both operating systems 
void SLEEP(long DelayInMs)
{
#ifndef WIN32
	usleep(DelayInMs * 1000) ;
#else
	Sleep(DelayInMs) ;
#endif							
}

int Time2FirmwareTime(int iTime)
{
  return ( 0xFF - (int)(1.5 * iTime) ) ;
}

// === USB Code (taken from OBJECTIVE DEVELOPMENT Software GmbH command line tools)
int usbGetStringAscii(usb_dev_handle *dev, int index, 
														 int langid, char *buf, int buflen)
{
  char    buffer[256];
  int     rval, i;

  if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, 
                            USB_REQ_GET_DESCRIPTOR, 
                            (USB_DT_STRING << 8) + index, 
                            langid, 
                            buffer, 
                            sizeof(buffer), 
                            1000)) < 0)
    return rval;

  if (buffer[1] != USB_DT_STRING)
    return 0;

  if((unsigned char)buffer[0] < rval)
    rval = (unsigned char)buffer[0];

  rval /= 2;
  /* lossy conversion to ISO Latin1 */
  for(i = 1; i < rval; i++)
  {
    if (i > buflen)  /* destination buffer overflow */
      break;

    buf[i-1] = buffer[2 * i];
    if (buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
      buf[i-1] = '?';
  }

  buf[i-1] = 0;
  return i-1;
}


// === USB Code (taken from OBJECTIVE DEVELOPMENT Software GmbH command line tools)
// added checking of serial number and several traces for locating problems
usb_dev_handle *findUSBDevice(pUIParams pUI, int bDebug)
{
  struct usb_bus      *bus;
  struct usb_device   *dev;
  usb_dev_handle      *handle = 0;

  usb_find_busses();
  usb_find_devices();
  for(bus=usb_busses; bus; bus=bus->next)
  {
    for(dev=bus->devices; dev; dev=dev->next)
    {
      if(dev->descriptor.idVendor == USBDEV_SHARED_VENDOR && 
         dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT)
      {
        char    string[256];
        int     len;

        dbgprintf(pUI, 6, "info: SharedVendor found");

        handle = usb_open(dev); /* we need to open the device in order to query strings */
        if(!handle){
            dbgprintf(pUI, 3, "Warning: cannot open USB device: %s", usb_strerror());
            continue;
        }
        /* now find out whether the device actually is obdev's Remote Sensor: */
        len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 
                                0x0409, string, sizeof(string));
        if(len < 0)
        {
          dbgprintf(pUI, 3, "warning: cannot query manufacturer for device: %s", usb_strerror());
          goto skipDevice;
        }

        dbgprintf(pUI, 6, "info: got Descriptor.iManu [%s]", string);

        /* fprintf(stderr, "seen device from vendor ->%s<-\n", string); */
        if(strcmp(string, "www.anyma.ch") != 0)
        {
          dbgprintf(pUI, 6, "info: found device: %s", string);
          goto skipDevice;
        }

        len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
        if(len < 0)
        {
          dbgprintf(pUI, 3, "warning: cannot query product for device: %s", usb_strerror());
          goto skipDevice;
        }

        dbgprintf(pUI, 6, "info: got Descriptor.iProd [%s]", string);

        /* fprintf(stderr, "seen product ->%s<-\n", string); */
        if(strcmp(string, "uDMX") != 0)
          dbgprintf(pUI, 6, "info: found device: %s", string);
        else
        {
          /* read / check serial number */
          len = usb_get_string_simple(handle, 
                                      dev->descriptor.iSerialNumber, 
                                      string, 
                                      sizeof(string));
          if (len > 0)
            dbgprintf(pUI, 6, "info: got Descriptor.iSerialNumber [%s]", string); 

          if (pUI->bSerial &&
              (strcmp(pUI->cSerial, string) != 0)) 
          { 
            goto skipDevice;
          }        
                  
          break ;
        }

skipDevice:
        usb_close(handle);
        handle = NULL;
      }
      else
      {
        // found something unwanted. show it at least
        dbgprintf(pUI, 6, "  info: no SharedVendor / Product: ");

        char    string[256];
        int     len;

        handle = usb_open(dev); /* we need to open the device in order to query strings */
        if(!handle)
        {
          dbgprintf(pUI, 3, " Warning: cannot open USB device: %s", usb_strerror());
          continue;
        }
        /* now find out whether the device actually is obdev's Remote Sensor: */
        len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 
                                0x0409, string, sizeof(string));
        if(len > 0)
          dbgprintf(pUI, 6, "Manufacturer[%-40s] ", string);

        len = usbGetStringAscii(handle, dev->descriptor.iProduct, 
                                0x0409, string, sizeof(string));
        if(len > 0)
          dbgprintf(pUI, 6, "Product[%-40s]", string);

        //dbgprintf(pUI, 6, "\n");
        usb_close(handle);
        handle = NULL;
      }
    }
    if(handle)
      break;
  }
  if(!handle)
  {
    dbgprintf(pUI, 3, "Could not find USB device www.anyma.ch/uDMX%s%s",
              (pUI->bSerial ? "/" : ""), (pUI->bSerial ? pUI->cSerial : ""));
  }
    
  // show connected  
  pUI->lbUDMX->value((handle != NULL)) ;

  return handle;
}

// USBDetectTransferSize() - let us find out, how much bytes we can transfer with ONE usb_control_msg
#ifdef COMMENT
int USBDetectTransferSize(usb_dev_handle *hUSB, uchar * cMem, 
													bool bDebug, int iDefBlockSize = 128)
{
  int             iBlockSize, iSent, iCnt ;
  //struct timeval  tvStart, tvEnd ;
  //long            lDiff ;

  if (hUSB == NULL)
    // invalid handle -> no output -> debug instead
    return 1 ;

  for (iBlockSize = iDefBlockSize; iBlockSize >= 1; iBlockSize = iBlockSize / 2)
  {
    iSent = usb_control_msg(hUSB,
                            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                            cmd_SetChannelRange,
                            iBlockSize,
                            0,
                            (char *) cMem,
                            iBlockSize,
                            50000);
    if(iSent != iBlockSize)
    {
      dbgprintf(3, "usb_control_msg(..%d..) USB error: %s", 
                        iBlockSize, usb_strerror()) ;
    }
    else
    {
#ifndef WIN32
      gettimeofday(&tvStart, 0) ;
#endif
      for (iCnt = 0; iCnt < 10; iCnt++)
      {
        iSent = usb_control_msg(hUSB,
                                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                cmd_SetChannelRange,
                                iBlockSize,
                                0,
                                (char *) cMem,
                                iBlockSize,
                                50000);
        if(iSent != iBlockSize)
        {
          dbgprintf(3, "usb_control_msg(..%d..) USB error: %s", 
                            iBlockSize, usb_strerror()) ;
          continue ;
        }
      }
#ifndef WIN32
      gettimeofday(&tvEnd, 0) ;
      lDiff = (tvEnd.tv_sec -  tvStart.tv_sec) * 1000000 +
              (tvEnd.tv_usec - tvStart.tv_usec) ;
      dbgprintf(6, "512 channels with blocksize %d: %d mikrosec", 
              /*(512 / iBlockSize) * (lDiff / 10)*/ iBlockSize, 512 * lDiff / iBlockSize / 10) ;
#endif
      break ;
    }
  }

  dbgprintf(3, "usb_control_msg(..%d..) Sent %d -> OK:", iBlockSize, iSent) ;
  return iBlockSize ;
}
#endif

// uDMXthread() - thread to transfer data to uDMX device
#ifndef WIN32
void *uDMXthread(void *pUIvoid)
#else
void uDMXthread(void *pUIvoid)
#endif
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  //opts_t *thPar = (opts_t *) Param ;
	bool bDebug	= (pUI->iVerbose ? 1 : 0) ;	
  usb_dev_handle      *hUSB = NULL ;
  int                 i = 0, DmxDataLen, DmxChans ;
  int                 USBBlock ;
  uchar               DataOld[ARTNET_MAX_DMX], DataNew[ARTNET_MAX_DMX] ;	
  time_t              DataTime[ARTNET_MAX_DMX] ;
  time_t              tNow ;
  int                 timBreak, timMark, timIBGap ;
	
	dbgprintf(pUI, 6, "uDMXthread starting") ;		
  
  memset(&DataOld, 0, sizeof(DataOld)) ;
  memset(&DataNew, 0, sizeof(DataNew)) ;
  memset(&DataTime, 0, sizeof(DataTime)) ;
	
  // try to open uDMX device
  usb_set_debug(bDebug);
  usb_init();
	
	dbgprintf(pUI, 6, "uDMXthread started") ;		

	while(pUI->AnRunning)
	{
		if (hUSB == NULL)
		{
			// no connection -> try a new one ...
		  if((hUSB = findUSBDevice(pUI, bDebug)) == NULL)
		  {
		    dbgprintf(pUI, 3, "Could not find USB device \"uDMX\" with vid=0x%04X pid=0x%04X",
										USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
				SLEEP(500) ;
				continue ;
		  }
			else
			  dbgprintf(pUI, 6, "uDMX connected") ;			
		
		}
			
		memcpy(DataNew, &(pUI->dmx[0][0]), pUI->DmxLen) ;
    
    // copy param to var, as it might change during cycle
    DmxDataLen  = pUI->DmxLen ;     // how many channels read
    DmxChans    = pUI->iDMXChans ;   // how many channels chosen by user
    // dont transfer more than necessary ->
    // so reduce the USBBlock-size to transfer to the number of channels 
    // if this is smaller
    USBBlock    = min(pUI->iUDMXBs, DmxChans) ; 
    
    if ((timBreak != pUI->iUDMXTb) ||
        (timMark  != pUI->iUDMXTm) ||
        (timIBGap != pUI->iUDMXTg))
    {
      // Die engestellten Zeiten in FirmwareZeiten umrechnen
      timBreak    = pUI->iUDMXTb ;
      timMark     = pUI->iUDMXTm ;
      timIBGap    = pUI->iUDMXTg ;
      
      DataNew[0]  = Time2FirmwareTime(timIBGap) ;
      
      if (!usb_control_msg(hUSB,
                            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                            cmd_SetTiming,
                            Time2FirmwareTime(timBreak),
                            Time2FirmwareTime(timMark),
                            (char *)&DataNew[0],
                            1,
                            50000))
        dbgprintf(pUI, 1, "usb_control_msg(..%d, %d..) USB error: %s",
                    timBreak, timMark, usb_strerror());                            
    }
    
    tNow  = time(NULL) ;
			
    // OUTPUT DATA TO uDMX if data has changed
    for (i = 0; i < min(DmxDataLen, DmxChans); i = i + USBBlock)
    {
      // Transfer memory blocks via USB when
      // o USB is available
      // o data changed or
      // o no transfer for the last 4 seconds
      if (hUSB && 
          ((memcmp(&DataNew[i], &DataOld[i], USBBlock) != 0) ||
          ((DataTime[i] + 4) < tNow)) )
      {
        // memory has changed (at least 1 byte)
        int iSent ;

        iSent = usb_control_msg(hUSB,
                                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                cmd_SetChannelRange,
                                USBBlock,
                                i,
                                (char *)&DataNew[i],
                                USBBlock,
                                50000);
        if(iSent != USBBlock)
				{
					// Error in Transfer -> reopen USB
          dbgprintf(pUI, 1, "usb_control_msg(..%d, %d..) USB error: %s",
                      USBBlock, i, usb_strerror());
											
			    usb_close(hUSB) ;
					hUSB = NULL ;
				}
        else
          DataTime[i] = tNow ;
      }
    }	

    // store new data
    memcpy(DataOld, DataNew, DmxDataLen) ;

    // wait a minimum of time - otherwise uses too much CPU
    SLEEP(1) ;
	}
	
//uDMXThreadEnd:		
  // close usb
  if (hUSB)
    usb_close(hUSB) ;
		
	dbgprintf(pUI, 6, "uDMXthread ended") ;		
#ifndef WIN32
  return NULL ;
#else
	return ;
#endif
}
