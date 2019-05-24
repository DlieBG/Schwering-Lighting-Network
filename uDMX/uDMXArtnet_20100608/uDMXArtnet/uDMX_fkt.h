// ===== uDMX_fkt.h =====

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


typedef unsigned char uchar ;

// === USB 
#include <usb.h>    /* this is libusb, see http://libusb.sourceforge.net/ */

#include "uDMXArtnet.h"

void SLEEP(long ) ;

void dbgprintf(pUIParams pUI, int bDebug, char *fmt, ... ) ;

int usbGetStringAscii(usb_dev_handle *dev, int index, 
														 int langid, char *buf, int buflen) ;
														 
usb_dev_handle *findUSBDevice(int bDebug) ;

int USBDetectTransferSize(usb_dev_handle *hUSB, uchar * cMem, 
													bool bDebug, int iDefBlockSize) ;
													
#ifndef WIN32
void *uDMXthread(void *Param);
#else
void uDMXthread(void *Param);
#endif

