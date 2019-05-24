// ===== uDMXArtnet.cpp =====

/*
  Copyright (C) 2009 Lutz (ilLU[TZ]minator)

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

//#include <libintl.h>
#include "uDMXArtnet.h"

// Headers for fltk
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Help_View.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>

// common headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef WIN32
// === Linux ...
#define MAX_PATH    256
#include <pthread.h>

#include "uDMX.xpm"
#include <FL/x.H>
#include <X11/xpm.h>
#else
// === WIN32
#include "uDMXArtnetrc.h"
#include <process.h>
#endif

#include "uDMX_fkt.h"

// artnet identification 
#define SHORT_NAME 	"uDMXArtnet"
#define LONG_NAME 	"uDMXArtnet (by ilLUTZminator)"


// === idle() - update the dialog if nothing else to do
void idle(void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ; 
	uint8_t *data = &pUI->dmx[0][0];
  int i ; 
  StringList::iterator t ;

  // show actual dmx values
	for (i = 0; i < 20; i++)
	{
    pUI->progDMX[i]->value((data[i] * 100) / 255) ;
	}

  // show trace
  if (pUI->trcList.size() > 0)
  {
    // lock while changing list (threads change them too !)
		pUI->trcMutex.lock() ;
    
    for (t = pUI->trcList.begin(); t != pUI->trcList.end(); ++t)
    {
      string * strTrc = *t ;

      while (pUI->browDebug->size() > 100)
        pUI->browDebug->remove(1) ;
      
      pUI->browDebug->insert(pUI->browDebug->size() + 1, strTrc->c_str()) ; 
      pUI->browDebug->bottomline(pUI->browDebug->size()) ;
      
  		pUI->browDebug->redraw() ;

      delete strTrc ;
    }
    pUI->trcList.clear() ;
    // unlock after list changed
		pUI->trcMutex.unlock() ;
  }

  pUI->winMain->flush() ;

  SLEEP(30) ; // SAVE CPU time (dont update too often) !
  Fl::remove_idle(idle) ;  
  
  return ;
}

// dmx_handler() - reading artnet data and do the visualisation
int dmx_handler(artnet_node n, int port, void *pUIvoid)
{
	uint8_t *data ;
	pUIParams pUI = (pUIParams) pUIvoid ;
	int len ;

  // we only use port 0 here
  port = 0 ;
	
	if(port < pUI->num_ports)
	{
		data = artnet_read_dmx(n, port, &len) ;
		//pthread_mutex_lock(&mem_mutex) ;
		memcpy(&pUI->dmx[port][0], data, len) ;
    pUI->DmxLen =len ;
		//pthread_mutex_unlock(&mem_mutex) ;
		
    /*
    // better no output in thread -> moved to idle
		for (i = 0; i < 20; i++)
		{
      pUI->progDMX[i]->value((data[i] * 100) / 255) ;
		}
    */
    
    pUI->cUI->Trace(pUI, 9, "rcvd %d bytes. %02X %02X %02X %02X %02X %02X %02X %02X ", 
                    len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]) ;    
	}
	
	return 0;
}

// artnetStart() - start artnet with given parameters
int artnetStart(pUIParams pUI)
{
	char *								ip_addr = NULL ;	
  artnet_start_params 	anp ;	
	artnet_node 					node ;	

  ip_addr       = pUI->ip_addr;
	anp.verbose		= 0 ;
	anp.bind2Ip 	= pUI->bind2Ip ;
	anp.reuse			= 1 ;
	strcpy(anp.cBcastAddr, pUI->cANBc) ;
  
  if ( (node = artnet_new_mod(ip_addr, anp) ) == NULL )
  {
		pUI->cUI->Trace(pUI, 0, "artnet_new_mod() failed %s" , artnet_strerror() ) ;
    goto artnetStartEnd ;
  }
	else
		pUI->cUI->Trace(pUI, 6, "node <%s> created", ip_addr) ;
  
 	artnet_set_short_name(node, SHORT_NAME) ;
	artnet_set_long_name(node, LONG_NAME) ;
	artnet_set_node_type(node, ARTNET_NODE) ;
	artnet_set_subnet_addr(node, 0) ;

	// set dmx handler
	artnet_set_dmx_handler(node, dmx_handler, (void*) pUI ) ;  

	// set the first port to output dmx data
	artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;

	// set the universe address of the first port
	artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, pUI->port_addr) ;

	if( artnet_start(node) != ARTNET_EOK)
		pUI->cUI->Trace(pUI, 0, "artnet_start(%s) failed %s", ip_addr, artnet_strerror() ) ;
	
artnetStartEnd :  
  pUI->AnNode       = node ;
  
  pUI->lbArtnet->value((node != NULL)) ;
	
	return (node != NULL) ;
}

// artnetStop() - stop artnet
int artnetStop(pUIParams pUI)
{
	// set dmx handler
  if (!pUI->AnNode)
    return false ;
    
	artnet_set_dmx_handler(pUI->AnNode , NULL, (void*) NULL) ; 
	artnet_stop(pUI->AnNode) ;
	artnet_destroy(pUI->AnNode) ;
  pUI->AnNode = NULL ;
  
  // if no trace -> artnet does not restart 
  // (no idea why so far - probably something about message-handling)
  pUI->cUI->Trace(pUI, 0, "artnet reset") ;  
	
	return true ;
}


// artnetThread() - start and restart artnet, read artnet data (blocking)
#ifndef WIN32
void *artnetThread(void *pUIvoid)
#else
void artnetThread(void *pUIvoid)
#endif
{
	pUIParams pUI = (pUIParams) pUIvoid ;
	
	if (!artnetStart(pUI))
		//goto artnetThreadEnd ;
    SLEEP(200) ;
  else
  	pUI->cUI->Trace(pUI, 6, "artnetThread started") ;	

	while(pUI->AnRunning)
	{
    if (!pUI->AnNode)
      if (!artnetStart(pUI))
        SLEEP(2000) ;
  
    if (pUI->AnRestart)
    {
      pUI->AnRestart = false ;
      pUI->cUI->Trace(pUI, 6, "restarting Artnet") ;

      artnetStart(pUI) ;
    }
    // artnet_read returns when NO ARTNET-DATA within the
    // given time in seconds, normally that never happens
    // artnet_read also returns, after artnetStop() was called
		artnet_read(pUI->AnNode, 1) ;	
	}

//artnetThreadEnd:	
	pUI->cUI->Trace(pUI, 6, "artnetThread ending") ;
	artnetStop(pUI) ;
	
	pUI->cUI->Trace(pUI, 6, "artnetThread ended") ;
	
#ifndef WIN32
  return NULL ;
#else
  return ;
#endif
}

// ColorSelectBG() - select background color
void ColorSelectBG(Fl_Widget *, void * pUIvoid)
{
//  UIParams *pUI = (UIParams *)pUIvoid ;  
  Fl_Color c = FL_BACKGROUND_COLOR ;  
  uchar r, g, b; 
  
  Fl::get_color(c, r, g, b) ;
  
  if (fl_color_chooser(0, r, g, b))
  {
    Fl::set_color(c, r, g, b) ; 
    Fl::redraw();
  }

  return ;
}
// ColorSelectFG() - select foreground color
void ColorSelectFG(Fl_Widget *, void * pUIvoid)
{
//  UIParams *pUI = (UIParams *)pUIvoid ;  
  Fl_Color c = FL_FOREGROUND_COLOR ;  
  uchar r, g, b; 
  
  Fl::get_color(c, r, g, b) ;
  
  if (fl_color_chooser(0, r, g, b))
  {
    Fl::set_color(c, r, g, b) ; 
    Fl::redraw();
  }

  return ;
}
// ColorSelectIN() - select inputs background color
void ColorSelectIN(Fl_Widget *, void * pUIvoid)
{
//  UIParams *pUI = (UIParams *)pUIvoid ;  
  Fl_Color c = FL_BACKGROUND2_COLOR ;  
  uchar r, g, b; 
  
  Fl::get_color(c, r, g, b) ;
  
  if (fl_color_chooser(0, r, g, b))
  {
    Fl::set_color(c, r, g, b) ; 
    Fl::redraw();
  }

  return ;
}
// ColorSelectPG() - select progress bars color
void ColorSelectPG(Fl_Widget *, void * pUIvoid)
{
//  UIParams *pUI = (UIParams *)pUIvoid ;  
  Fl_Color c = (Fl_Color)4 ;  
  uchar r, g, b; 
  
  Fl::get_color(c, r, g, b) ;
  
  if (fl_color_chooser(0, r, g, b))
  {
    Fl::set_color(c, r, g, b) ; 
    Fl::redraw();
  }

  return ;
}

void VerboseChange(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  pUI->iVerbose = (int)pUI->spinVerbose->value() ;
}
void DMXUnivChange(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  pUI->iDMXUniv = (int)pUI->spinDMXUniv->value() ;
  
  // transfer parameter to node
  pUI->port_addr  = pUI->iDMXUniv ;	  
  // restart artnet
  artnetStop(pUI) ;
  pUI->AnRestart = true ;    
}
void TimeBreakChange(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  pUI->iUDMXTb  = (int)pUI->spinUDMXTb->value() ;
}
void TimeMarkChange(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  pUI->iUDMXTm  = (int)pUI->spinUDMXTm->value() ;
}
void TimeGapChange(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  pUI->iUDMXTg  = (int)pUI->spinUDMXTg->value() ;
}

void Debug2Clip(Fl_Widget *, void * pUIvoid)
{
	pUIParams pUI = (pUIParams) pUIvoid ;
  const char * cIn ;
  char *cOut = NULL ;
  int sizeOld = 0, sizeNew ;

  for (int i = 1; i <= pUI->browDebug->size(); i++)
  {
    cIn   = pUI->browDebug->text(i) ;
    if (!cIn)
      continue ;

    sizeNew = sizeOld + strlen(cIn) + 3 ;
		cOut  = (char *)realloc(cOut, sizeNew) ;

		cOut[sizeOld]	= 0 ;

    strcat(cOut, "\r\n") ;
    strcat(cOut, cIn) ;

		sizeOld = sizeNew ;
  }

  Fl::copy(cOut, strlen(cOut) + 1, 1) ;
 
  free(cOut) ;
}

// prefsFile() - create filename for prefs depending on parameter cSerial
bool PrefsFile(pUIParams pUI, char *cFile)
{
	strcpy(cFile, "uDMXNode") ;
  if (pUI->bSerial || 
      strlen(pUI->cConfig))
  {
    strcat(cFile, "_") ;
    if (!strlen(pUI->cConfig))
      strcat(cFile, pUI->cSerial) ;
    else
      strcat(cFile, pUI->cConfig) ;    
  }

	return true ;
}

// LayoutSave() - save data responsible for dialogs layout
void LayoutSave(pUIParams pUI)
{
  // Path for Settings
	char cFile[MAX_PATH] ;

  PrefsFile(pUI, (char *)&cFile) ;
  Fl_Preferences prefs( Fl_Preferences::USER, "ilLUTZminator.de", cFile);
  
  // Position
  Fl_Preferences prefPos( prefs, "Layout" ) ; 
  prefPos.set("Left", pUI->winMain->x()) ; 
  prefPos.set("Top",  pUI->winMain->y()) ;   
  
  prefPos.set("Scheme", pUI->ddScheme->value()) ;
  
  prefPos.set("ColorBG", (int)Fl::get_color(FL_BACKGROUND_COLOR)) ;  
  prefPos.set("ColorFG", (int)Fl::get_color(FL_FOREGROUND_COLOR)) ;    
  prefPos.set("ColorIN", (int)Fl::get_color(FL_BACKGROUND2_COLOR)) ;      
  prefPos.set("ColorPG", (int)Fl::get_color((Fl_Color)4)) ;        
  
  return ;
}

// SchemeSet() - set scheme on dropdowns (Fl_Choice) selection
void SchemeSet(Fl_Choice * oChoice, void * pUIvoid)
{
  Fl::scheme(oChoice->text(oChoice->value()));
  return ;
}

// ShutdownQuery() - dont stop program without confirmation
void ShutdownQuery(Fl_Widget *, void * pUIvoid)
{
  UIParams *pUI = (UIParams *)pUIvoid ;

  int rep = fl_choice("uDMX node: Do you want to quit ?", 
                      "Yes", "No", 0);
  if (rep == 0) 
  {
    LayoutSave(pUI) ;
    
    pUI->AnRunning = 0 ;
		artnetStop(pUI) ;
				
		pUI->cUI->Trace(pUI, 6, "waiting for threads to end") ;	
    SLEEP(2000) ;
    
    exit(0);
  }

  return ;
}

// Value2SingleBit() - changes value to value that is limited to
// a value that has only one bit set (1, 2, 4, 8, 16 ... 512)
int Value2SingleBit(int iValue, pUIParams pUI)
{
  int iBit = 0 ;
  
  while(iValue > 0)
  {
    iValue = iValue >> 1 ;
    iBit ++ ;
//		pUI->cUI->Trace(pUI, 0, "iValue: %d iBit %d", iValue, iBit) ;	    
  }
  iValue = (iBit ? (1 << (iBit - 1)) : 0) ;

  return iValue ;
}

// SettingsSave() - save all settings needed for program (except layout)
void SettingsSave(Fl_Widget *, void * pUIvoid)
{
  UIParams *pUI = (UIParams *)pUIvoid ;

  // Path for Settings
	char cFile[MAX_PATH] ;

  PrefsFile(pUI, (char *)&cFile) ;
  Fl_Preferences prefs( Fl_Preferences::USER, "ilLUTZminator.de", cFile);

  // Common settings
  Fl_Preferences prefCommon( prefs, "Common" ) ;
  pUI->iVerbose = (int)pUI->spinVerbose->value() ;
  prefCommon.set( "Verbose", pUI->iVerbose);
  
  // Artnet settings
  Fl_Preferences prefArtnet( prefs, "Artnet" ) ;
  pUI->iANReuse = pUI->chkANReuse->value() ;
  prefArtnet.set( "Reuse", pUI->iANReuse);
  /*
  prefArtnet.set( "IP", pUI->ddANIp->value() );  
  prefArtnet.set( "Mask", pUI->ddANBc->value() );    
  */
  strcpy(pUI->cANIp, pUI->ddANIp->input()->value() ) ;
  prefArtnet.set( "IP", pUI->cANIp ); 
  strcpy(pUI->cANBc, pUI->inANBc->value()) ;  
  prefArtnet.set( "Mask", pUI->cANBc );    
  
  // DMX
  Fl_Preferences prefDMX( prefs, "DMX" ) ;
  pUI->iDMXUniv   = (int)pUI->spinDMXUniv->value() ;
  prefDMX.set( "Universe", pUI->iDMXUniv );
  
  pUI->iDMXChans  = (int)pUI->spinDMXChans->value() ;
  pUI->iDMXChans  = Value2SingleBit(pUI->iDMXChans, pUI) ;
  pUI->spinDMXChans->value(pUI->iDMXChans) ;
  prefDMX.set( "Channels", pUI->iDMXChans );  

  // uDMX
  Fl_Preferences prefUDMX( prefs, "uDMX" ) ;
  
  pUI->iUDMXBs    = (int)pUI->spinUDMXBs->value() ; 
  pUI->iUDMXBs    = Value2SingleBit(pUI->iUDMXBs, pUI) ;
  pUI->spinUDMXBs->value(pUI->iUDMXBs) ;
  prefUDMX.set( "BlockSize", pUI->iUDMXBs );
  
  pUI->iUDMXTb    = (int)pUI->spinUDMXTb->value() ;
  prefUDMX.set( "TimeBreak", pUI->iUDMXTb );
  pUI->iUDMXTm    = (int)pUI->spinUDMXTm->value() ;
  prefUDMX.set( "TimeMark", pUI->iUDMXTm );
  pUI->iUDMXTg    = (int)pUI->spinUDMXTg->value() ;
  prefUDMX.set( "TimeIBGap", pUI->iUDMXTg );  
  
  // calculated values
  pUI->ip_addr	= (strlen(pUI->cANIp) != 0) ? pUI->cANIp : NULL ;
  pUI->bind2Ip	= (strlen(pUI->cANIp) != 0) ;   

  // transfer parameter to node
  pUI->port_addr  = pUI->iDMXUniv ;	  

  // restart artnet
  artnetStop(pUI) ;
  pUI->AnRestart = true ;  
  
  return ;
}

// ButtonUnuse() - make button unusable
// if button is pressed it does NOT change state
static void ButtonUnuse(Fl_Widget * pBtn, void *)
{
  Fl_Light_Button *pL = (Fl_Light_Button *)pBtn ;
  pL->value(!pL->value()) ;
  return ;
}

// Trace() - output in debug-window
bool UserInterface::Trace(pUIParams pUI, int level, const char *fmt, ...)
{
  char Buffer[1024] ;

  if ((level <= pUI->iVerbose) && (fmt))
  {
    va_list arglist ;
    va_start(arglist, fmt) ;	

#ifdef FL_LOCK
  	Fl::lock() ;
#endif

		snprintf(Buffer, sizeof(Buffer), "%04d ", pUI->TrcID) ;
		pUI->TrcID++ ;
		pUI->TrcID = pUI->TrcID % 10000 ;
		
	  vsnprintf(Buffer + 5, sizeof(Buffer) - 5, fmt, arglist);

    /* NO CHANGES OF FLTK ELEMENT FROM THREAD
       THIS LEADS TO CHRASHES WITH X11
    while (pUI->browDebug->size() > 100)
      pUI->browDebug->remove(1) ;
      
    pUI->browDebug->insert(pUI->browDebug->size() + 1, Buffer) ; 
    pUI->browDebug->bottomline(pUI->browDebug->size()) ;
      
		pUI->browDebug->redraw() ;
    */
    // STORE STRING IN LIST AND SHOW LIST IN IDLE FUNCTION
    string *str = new string(Buffer) ;

    // lock while changing list 
    // (main process and threads do tracing / change list ...)
		pUI->trcMutex.lock() ;
    pUI->trcList.push_back(str) ;
    // unlock after changing list
		pUI->trcMutex.unlock() ;

#ifdef FL_LOCK
	  Fl::awake(idle, pUI) ;
  	Fl::unlock() ;
#endif
    
	  va_end(arglist) ;
  }

  return true ;
}

// SettingsRead() - get all parameters for program (except layout)
bool UserInterface::SettingsRead(pUIParams pUI)
{
  // Path for Settings
	char cFile[MAX_PATH] ;

  PrefsFile(pUI, (char *)&cFile) ;
  Fl_Preferences prefs( Fl_Preferences::USER, "ilLUTZminator.de", cFile);

  //char path[ FL_PATH_MAX ];
  //refs.getUserdataPath( path, sizeof(path) );
  
  // Common settings
  Fl_Preferences prefCommon( prefs, "Common" ) ;
  prefCommon.get( "Verbose", pUI->iVerbose, 0) ;
  pUI->spinVerbose->value(pUI->iVerbose) ;  
  
  // Artnet settings
  Fl_Preferences prefArtnet( prefs, "Artnet" ) ;
  prefArtnet.get( "Reuse", pUI->iANReuse, 1) ;
  pUI->chkANReuse->value(pUI->iANReuse);  
  
  // IP ... add last value to drop-down list and select value
#ifdef WIN32
  prefArtnet.get( "IP", pUI->cANIp, "0.0.0.0", 79);
#else
  prefArtnet.get( "IP", pUI->cANIp, "", 79);
#endif
  if (strlen(pUI->cANIp) > 0)
  {
    // select the stored value
    pUI->ddANIp->add(pUI->cANIp) ;  
    pUI->ddANIp->value(0) ;    
  }
  else
  {
    // empty Value -> do not select anything
    pUI->ddANIp->add("0.0.0.0") ;    
  }
  
  prefArtnet.get( "Mask", pUI->cANBc, "255.255.255.255", 79);
  pUI->inANBc->value(pUI->cANBc) ;    
  
  // DMX
  Fl_Preferences prefDMX( prefs, "DMX" ) ;
  prefDMX.get( "Universe", pUI->iDMXUniv, 0) ;
  pUI->spinDMXUniv->value(pUI->iDMXUniv) ;
  prefDMX.get( "Channels", pUI->iDMXChans, 512) ; 
  pUI->spinDMXChans->value(pUI->iDMXChans) ;   

  // uDMX
  Fl_Preferences prefUDMX( prefs, "uDMX" ) ;
  prefUDMX.get( "BlockSize", pUI->iUDMXBs, 128) ;
  pUI->spinUDMXBs->value(pUI->iUDMXBs) ;
  prefUDMX.get( "TimeBreak", pUI->iUDMXTb, 88) ;
  pUI->spinUDMXTb->value(pUI->iUDMXTb) ;
  prefUDMX.get( "TimeMark", pUI->iUDMXTm, 8) ;
  pUI->spinUDMXTm->value(pUI->iUDMXTm) ;
  prefUDMX.get( "TimeIBGap", pUI->iUDMXTg, 0) ;
  pUI->spinUDMXTg->value(pUI->iUDMXTg) ;  
  
  // calculated values
  pUI->ip_addr	= (strlen(pUI->cANIp) != 0) ? pUI->cANIp : NULL ;
  pUI->bind2Ip	= (strlen(pUI->cANIp) != 0) ; 

  // initialisation of structure
  pUI->port_addr  = pUI->iDMXUniv ;
  pUI->num_ports  = 1 ;
  pUI->DmxLen     = 0 ;

  memset(&pUI->dmx[0][0], 0, ARTNET_MAX_DMX +1) ;
  
  pUI->AnRunning  = 1 ;
  pUI->AnRestart  = 1 ;
  
	pUI->trcList.clear() ;
  pUI->TrcID      = 0 ;
  
  return true ;
}

// LayoutRead() - get parameters for layout
bool UserInterface::LayoutRead(pUIParams pUI)
{
  int X, Y, iScheme, iColor ;
  // Path for Settings
	char cFile[MAX_PATH] ;

  PrefsFile(pUI, (char *)&cFile) ;
  Fl_Preferences prefs( Fl_Preferences::USER, "ilLUTZminator.de", cFile);
  
  // Position
  Fl_Preferences prefPos( prefs, "Layout" ) ; 
  prefPos.get("Left", X, 100) ;
  prefPos.get("Top", Y, 100) ;  
  pUI->winMain->position(X, Y) ;

  // Scheme
  prefPos.get("Scheme", iScheme, 0) ;
  pUI->ddScheme->value(iScheme) ;
  SchemeSet(pUI->ddScheme, pUI) ;
  
  // Color
  prefPos.get("ColorBG", iColor, (int)Fl::get_color(FL_BACKGROUND_COLOR)) ; 
  Fl::set_color(FL_BACKGROUND_COLOR, (unsigned) iColor) ; 
  prefPos.get("ColorFG", iColor, (int)Fl::get_color(FL_FOREGROUND_COLOR)) ; 
  Fl::set_color(FL_FOREGROUND_COLOR, (unsigned) iColor) ; 
  prefPos.get("ColorIN", iColor, (int)Fl::get_color(FL_BACKGROUND2_COLOR)) ; 
  Fl::set_color(FL_BACKGROUND2_COLOR, (unsigned) iColor) ; 
  prefPos.get("ColorPG", iColor, (int)Fl::get_color((Fl_Color)4)) ; 
  Fl::set_color((Fl_Color)4, (unsigned) iColor) ; 
  
  Fl::redraw();  
  
  return true ;
}

// make_window() - create dialog
Fl_Double_Window* UserInterface::make_window(UIParams *pUI)
{
  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(424, 316, _("uDMX node"));
    w = o;
    o->user_data((void*)(this));
    o->align(Fl_Align(FL_ALIGN_CLIP|FL_ALIGN_INSIDE));
    { Fl_Tabs* o = new Fl_Tabs(0, 0, 450, 316);
      { Fl_Group* o = new Fl_Group(0, 26, 425, 284, _("&uDMX"));
        { pUI->browDebug = new Fl_Hold_Browser(4, 120, 418, 190);
          pUI->browDebug->callback((Fl_Callback*)Debug2Clip, (void *)pUI);
          pUI->browDebug->tooltip(_("Mouseclick copies contents to clipboard"));
        } // Fl_Browser* o
        { pUI->lbArtnet = new Fl_Light_Button(10, 34, 65, 20, _("Artnet"));
          pUI->lbArtnet->selection_color((Fl_Color)2);
          pUI->lbArtnet->tooltip(_("artnet:\nstartet OK if light green\n(no guarantee to receive data)"));           
          // Dont use deactivate() as color is nearly not recognizable
          // pUI->lbArtnet->deactivate() ;
          // Make Button unusable instead          
          pUI->lbArtnet->callback((Fl_Callback*)ButtonUnuse);                     
        } // Fl_Light_Button* o
        { pUI->lbUDMX = new Fl_Light_Button(90, 34, 65, 20, _("uDMX"));
          pUI->lbUDMX->selection_color((Fl_Color)2);
          pUI->lbUDMX->tooltip(_("uDMX:\nstartet OK if light green"));           
          pUI->lbUDMX->callback((Fl_Callback*)ButtonUnuse);           
          //pUI->lbUDMX->value(1) ;          
        } // Fl_Light_Button* o
        { pUI->progDMX[0] = new Fl_Progress(10, 64, 30, 20, _("001"));
          pUI->progDMX[0]->selection_color((Fl_Color)4);
          pUI->progDMX[0]->tooltip(_("data of first DMX channel"));           
        } // Fl_Progress* o
        { pUI->progDMX[1] = new Fl_Progress(45, 64, 30, 20, _("002"));
          pUI->progDMX[1]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[2] = new Fl_Progress(80, 64, 30, 20, _("003"));
          pUI->progDMX[2]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[3] = new Fl_Progress(115, 64, 30, 20, _("004"));
          pUI->progDMX[3]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[4] = new Fl_Progress(150, 64, 30, 20, _("005"));
          pUI->progDMX[4]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[5] = new Fl_Progress(185, 64, 30, 20, _("006"));
          pUI->progDMX[5]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[6] = new Fl_Progress(220, 64, 30, 20, _("007"));
          pUI->progDMX[6]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[7] = new Fl_Progress(255, 64, 30, 20, _("008"));
          pUI->progDMX[7]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[8] = new Fl_Progress(290, 64, 30, 20, _("009"));
          pUI->progDMX[8]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[9] = new Fl_Progress(325, 64, 30, 20, _("010"));
          pUI->progDMX[9]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[10] = new Fl_Progress(10, 90, 30, 20, _("011"));
          pUI->progDMX[10]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[11] = new Fl_Progress(45, 90, 30, 20, _("012"));
          pUI->progDMX[11]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[12] = new Fl_Progress(80, 90, 30, 20, _("013"));
          pUI->progDMX[12]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[13] = new Fl_Progress(115, 90, 30, 20, _("014"));
          pUI->progDMX[13]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[14] = new Fl_Progress(150, 90, 30, 20, _("015"));
          pUI->progDMX[14]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[15] = new Fl_Progress(185, 90, 30, 20, _("016"));
          pUI->progDMX[15]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[16] = new Fl_Progress(220, 90, 30, 20, _("017"));
          pUI->progDMX[16]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[17] = new Fl_Progress(255, 90, 30, 20, _("018"));
          pUI->progDMX[17]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[18] = new Fl_Progress(290, 90, 30, 20, _("019"));
          pUI->progDMX[18]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        { pUI->progDMX[19] = new Fl_Progress(325, 90, 30, 20, _("020"));
          pUI->progDMX[19]->selection_color((Fl_Color)4);
        } // Fl_Progress* o
        o->end();
      } // Fl_Group* o
      { Fl_Group* o = new Fl_Group(0, 26, 425, 289, _("&Setup"));
        o->hide();
        { Fl_Group* o = new Fl_Group(40, 31, 378, 49, _("Common"));
          o->labelfont(1);
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));
          { pUI->spinVerbose = new Fl_Spinner(120, 56, 45, 20, _("Verbose"));
            pUI->spinVerbose->minimum(0);
#ifdef FL_LOCK
            pUI->spinVerbose->maximum(0);
#else
            pUI->spinVerbose->maximum(9);
#endif
            pUI->spinVerbose->value(0);
            pUI->spinVerbose->tooltip(_("verbose:\nthe higher the value,\nthe more info you will see on uDMX page"));            
            pUI->spinVerbose->callback((Fl_Callback*)VerboseChange, (void *)pUI);                
          } // Fl_Spinner* o
          { pUI->ddScheme = new Fl_Choice(235, 55, 85, 20, _("Scheme"));
            pUI->ddScheme->down_box(FL_BORDER_BOX);
            pUI->ddScheme->add("none");
            pUI->ddScheme->add("gtk+");
            pUI->ddScheme->add("gleam");            
            pUI->ddScheme->add("plastic");
            pUI->ddScheme->callback((Fl_Callback *)SchemeSet, pUI);
            pUI->ddScheme->tooltip(_("scheme:\nchanges the programs look"));            
            
          } // Fl_Choice* o          
          { Fl_Button* o = new Fl_Button(330, 55, 20, 20, _("C"));
            o->tooltip(_("color:\nchanges the programs background color.\nbetter NOT choose same color like foreground ;-)"));           
            o->callback((Fl_Callback*)ColorSelectBG, (void *)pUI);            
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(352, 55, 20, 20, _("C"));
            o->tooltip(_("color:\nchanges the programs foreground color.\nbetter NOT choose same color like background ;-)"));           
            o->callback((Fl_Callback*)ColorSelectFG, (void *)pUI); 
            o->color(FL_FOREGROUND_COLOR);
            o->labelcolor(FL_BACKGROUND_COLOR);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(374, 55, 20, 20, _("C"));
            o->tooltip(_("color:\nchanges the inputs background color.\nbetter NOT choose same color like foreground ;-)")); 
            o->callback((Fl_Callback*)ColorSelectIN, (void *)pUI);             
            o->color(FL_BACKGROUND2_COLOR);
          } // Fl_Button* o          
          { Fl_Button* o = new Fl_Button(396, 55, 20, 20, _("C"));
            o->tooltip(_("color:\nchanges the progress bar color.\n")); 
            o->callback((Fl_Callback*)ColorSelectPG, (void *)pUI);             
            o->color((Fl_Color)4);
          } // Fl_Button* o          
          o->end();
        } // Fl_Group* o        
        { Fl_Group* o = new Fl_Group(40, 86, 345, 60, _("Artnet"));
          o->labelfont(1);
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));
          { pUI->chkANReuse = new Fl_Check_Button(40, 101, 75, 20, _("Reuse"));
            pUI->chkANReuse->down_box(FL_DOWN_BOX);
            pUI->chkANReuse->tooltip(_("reuse:\nreuse artnet-port ?\ncheck if sender and receiver on the same PC"));             
          } // Fl_Check_Button* o
          { pUI->ddANIp   = new Fl_Input_Choice(130, 101, 145, 20, _("IP"));
            pUI->ddANIp->tooltip(_("IP:\nip address to bind to.\n"
                                   "normally 0.0.0.0 for win32 or blank for linux should work\n"
                                   "get the list of available addresses\n"
                                   "with ipconfig (win32) or ifconfig"));             
          }
          { pUI->inANBc   = new Fl_Input(130, 121, 145, 20, _("BCast"));
            pUI->inANBc->tooltip(_("Bc:\nnetmask to use for broadcast.\n255.255.255.255 sends everywhere"));            
          }
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(40, 156, 345, 55, _("DMX"));
          o->labelfont(1);
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));
          { pUI->spinDMXUniv = new Fl_Spinner(130, 172, 45, 20, _("Universe"));
            pUI->spinDMXUniv->minimum(0);
            pUI->spinDMXUniv->maximum(15);
            pUI->spinDMXUniv->value(0);
            pUI->spinDMXUniv->tooltip(_("universe:\nfirst universe to receive data.\ndefault 0"));  
            pUI->spinDMXUniv->callback((Fl_Callback*)DMXUnivChange, (void *)pUI);  
          } // Fl_Spinner* o
          { pUI->spinDMXChans = new Fl_Spinner(275, 172, 45, 20, _("Channels"));
            pUI->spinDMXChans->minimum(1);          
            pUI->spinDMXChans->maximum(512);
            pUI->spinDMXChans->value(512);
            pUI->spinDMXChans->tooltip(_("channels:\nno of channels to use.\ndefault 512\nvalid values 1,2,4,8,16...512"));              
          } // Fl_Spinner* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(40, 213, 345, 67, _("uDMX"));
          o->labelfont(1);
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));
          { pUI->spinUDMXBs = new Fl_Spinner(130, 235, 45, 20, _("USBBlock"));
            pUI->spinUDMXBs->minimum(1);          
            pUI->spinUDMXBs->maximum(512);
            pUI->spinUDMXBs->value(512);
            pUI->spinUDMXBs->tooltip(_("USBBlock:\nblocksize for transfering data to uDMX.\ndefault 128\nvalid values 1,2,4,8,16...512"));             
          } // Fl_Spinner* o
          { pUI->outSerial = new Fl_Output(40, 255, 140, 20, _("SN"));
            //pUI->outSerial->deactivate();
            pUI->outSerial->hide() ;
            pUI->outSerial->tooltip(_("SN:\nserial number passed with commandline -s <serial>"));             
          } // Fl_Output* o
          { pUI->spinUDMXTb = new Fl_Spinner(275, 215, 65, 20, _("TimeBreak"));
            pUI->spinUDMXTb->minimum(88);
            pUI->spinUDMXTb->maximum(170);
            pUI->spinUDMXTb->value(88);
            pUI->spinUDMXTb->tooltip(_("TimeBreak:\nBreak / Reset time of DMX protocol.\nNeeds uDMX firmware >= 1.3\ndefault 88"));  
            pUI->spinUDMXTb->callback((Fl_Callback*)TimeBreakChange, (void *)pUI); 
          } // Fl_Spinner* o
          { pUI->spinUDMXTm = new Fl_Spinner(275, 235, 65, 20, _("TimeMark"));
            pUI->spinUDMXTm->minimum(8);
            pUI->spinUDMXTm->maximum(170);
            pUI->spinUDMXTm->value(8);
            pUI->spinUDMXTm->tooltip(_("TimeMark:\nfirst mark time of DMX protocol.\nNeeds uDMX firmware >= 1.3\ndefault 8"));             
            pUI->spinUDMXTm->callback((Fl_Callback*)TimeMarkChange, (void *)pUI);             
          } // Fl_Spinner* o
          { pUI->spinUDMXTg = new Fl_Spinner(275, 255, 65, 20, _("TimeIBGap"));
            pUI->spinUDMXTg->minimum(0);
            pUI->spinUDMXTg->maximum(170);
            pUI->spinUDMXTg->value(0);
            pUI->spinUDMXTg->tooltip(_("TimeIBGap:\nGaps of DMX protocol.\nNeeds uDMX firmware >= 1.4\ndefault 0"));             
            pUI->spinUDMXTg->callback((Fl_Callback*)TimeGapChange, (void *)pUI);             
          } // Fl_Spinner* o          
          o->end();
        } // Fl_Group* o
        { Fl_Button * o = new Fl_Button(165, 280, 75, 25, _("Save"));
          o->callback((Fl_Callback*)SettingsSave, (void *)pUI);        
        } // Fl_Button* o
        o->end();
      } // Fl_Group* o
      { Fl_Group* o = new Fl_Group(0, 26, 425, 289, _("&Info"));
        o->hide();
        { Fl_Help_View * o = new Fl_Help_View(10, 35, 405, 275);
          //o->box(FL_SHADOW_FRAME); // no update when scrolling    
          o->color(FL_BACKGROUND_COLOR) ;          
          o->value(
              "<HTML>\n"
              "<HEAD>\n"
              "<TITLE>uDMX node - Help</TITLE>\n"
              "</HEAD>\n"
              "<BODY>\n"
              "<H2>uDMX node</H2>\n" 
              "<H6>This program receives artnet-data and transfers them to an DMX transmitter "
              "based on <a href='http://www.anyma.ch/research/udmx/'>uDMX</a></h6>"
              "<H6>see <a href='http://www.ilLUTZminator.de'>http://www.ilLUTZminator.de</a> "
              "for much more infos</H6><BR><BR>"
              "<H2>License</H2>\n"
              "<H6><a href='http://www.gnu.org/copyleft/gpl.html'>GPLv3</A></H6><BR><BR>"
              "<H2>Made with / thanx to</H2>\n"
              "<H6><a href='http://sourceforge.net/projects/libartnet-win32/'>http://sourceforge.net/projects/libartnet-win32/</a><BR>"
              "<a href='http://www.fltk.org/'>http://www.fltk.org/</a><BR>"
              "<a href='http://libusb.sourceforge.net/'>http://libusb.sourceforge.net/</a> or "
              "<a href='http://libusb-win32.sourceforge.net/'>http://libusb-win32.sourceforge.net/</a><BR>"
              "</H6>"              
              "</BODY>\n") ;
        } // Fl_Help_View* o
        o->end();
      } // Fl_Group* o      
      o->end();
    } // Fl_Tabs* o
    o->end();
  } // Fl_Double_Window* o
  w->resizable(w) ;
  return w;
}

// ParseArgs() - get arguments from commandline
// serial number can only be set by commandline so far
void ParseArgs(pUIParams pUI, int argc, char *argv[])
{
	// parse args
	int optc ;

  pUI->bSerial		= false ;
  pUI->cSerial[0] = 0 ;
  pUI->cConfig[0] = 0 ;  

	// parse options 
	while ((optc = getopt (argc, argv, "s:c:")) != EOF)
  {
    // Need looking for serial-Number and
    // then saving to extra file
		switch (optc)
    {
    case 's':
      snprintf(pUI->cSerial, sizeof(pUI->cSerial) - 1, "%s", optarg) ;
      if (strlen(pUI->cSerial))
      {
				char cTmp[256] ;
        pUI->bSerial	= true ;
        
        snprintf(cTmp, sizeof(cTmp), "uDMX node [%s]", pUI->cSerial);  
        pUI->winMain->copy_label(cTmp) ;

        pUI->outSerial->value(pUI->cSerial) ;
        pUI->outSerial->show() ;
      }
      break;
      
    case 'c':
      snprintf(pUI->cConfig, sizeof(pUI->cConfig) - 1, "%s", optarg) ;
      break;      
    }
	}
  return ;
}

// main() - main program starting program loop
int main(int argc, char **argv)
{
  UserInterface  ui ;
  UIParams       uiParams ;
#ifndef WIN32
  int res ;
#endif  
#ifndef WIN32
  pthread_t           pThread ;
#else
  HANDLE              pThread ;
#endif	

#ifdef FL_LOCK
	Fl::lock() ;
#endif

  // create window
  uiParams.cUI      = &ui ;
  uiParams.winMain  = ui.make_window(&uiParams ) ;

  // make windows icon
#ifndef WIN32
  extern Display *fl_display;

  fl_open_display(); // needed if display has not been previously opened
  Pixmap p, mask;
  XpmCreatePixmapFromData(fl_display, 
                          DefaultRootWindow(fl_display),
                          (char **)uDMX_xpm, &p, &mask, NULL);
  uiParams.winMain->icon((char *)p);
#else
#define USE_RC_ICON
#ifdef USE_RC_ICON
  extern HINSTANCE fl_display;
  
  uiParams.winMain->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#else    
  HANDLE hImage ;
  
  hImage = LoadImage(NULL, "uDMX.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE) ;
  if (hImage)
    uiParams.winMain->icon(hImage);  
#endif
#endif

  // Arguments, Settings, Layout ...
  ParseArgs(&uiParams, argc, argv) ;
  
  ui.SettingsRead(&uiParams) ;

  // dont transfer parameters to mainwindow
  // but we need to use show() with arguments to use the icon
  // thus we reduce the amount of arguments to 1
  //uiParams.winMain->show(argc, argv) ;
  uiParams.winMain->show(1, argv) ;
  
  // Read layout after show() - otherwise color is not restored
  ui.LayoutRead(&uiParams) ;      
  uiParams.winMain->callback(ShutdownQuery, &uiParams) ;


  // show some infos about the program
  uiParams.cUI->Trace(&uiParams, 0, "uDMX node") ;
  uiParams.cUI->Trace(&uiParams, 0, ".written by ilLU[TZ]minator for") ;
  uiParams.cUI->Trace(&uiParams, 0, "..uDMX hardware developed by http://www.anyma.ch/research/udmx/") ;
  uiParams.cUI->Trace(&uiParams, 0, ".based on") ;
  uiParams.cUI->Trace(&uiParams, 0, "..http://sourceforge.net/projects/libartnet-win32/") ;  
  uiParams.cUI->Trace(&uiParams, 0, "..libusb / libusb-win32 and http://www.fltk.org/") ;  
  uiParams.cUI->Trace(&uiParams, 0, "uDMXArtnet started. Use Setup->Verbose to see more or less infos") ;    

  // create thread for getting the Art-Net packets
#ifndef WIN32
  if ((res = pthread_create(&pThread, NULL, artnetThread, &uiParams)) != 0)
    uiParams.cUI->Trace(&uiParams, 0, "pthread_create failed. Error %d", res) ;
#else
  pThread = (HANDLE) _beginthread(artnetThread, 0, (void *)&uiParams) ;
	if ((int)pThread == -1)
    uiParams.cUI->Trace(&uiParams, 0, "_beginthread failed. Error %d", errno) ;	
#endif	

SLEEP(200) ;

  // create thread for sending the DMX data
#ifndef WIN32
  if ((res = pthread_create(&pThread, NULL, uDMXthread, &uiParams)) != 0)
    uiParams.cUI->Trace(&uiParams, 0, "pthread_create failed. Error %d", res) ;
#else
  pThread = (HANDLE) _beginthread(uDMXthread, 0, (void *)&uiParams) ;
	if ((int)pThread == -1)
    uiParams.cUI->Trace(&uiParams, 0, "_beginthread failed. Error %d", errno) ;	
#endif	

  // set idle function for boring moments
  Fl::add_idle(idle, &uiParams) ;
  
  // finally begin message loop
	return Fl::run();
}
