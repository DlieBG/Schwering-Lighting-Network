// ===== uDMXArtnet.h =====

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

#ifndef uDMXArtnet_H
#define uDMXArtnet_H

#include <string>
#include <iostream>
#include <list>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Button.H>

#include "fltk_threads.h"

#include <artnet/artnet.h>

#define _(X) (X)

#ifndef WIN32
// === Linux ...
// According to chapter Multithreading of FLTK documentation
// locking helps to avoid problems with several threads
// accessing FLTK funktions
// In my experience better NOT let threads access FLTK functions
// and avoid Fl::lock() as it leads to deadlocks.
//#define FL_LOCK
// use real mutex instead
#endif


using namespace std ;

typedef std::list <string *> StringList ; 


class UserInterface ;

typedef struct
{
  UserInterface     * cUI ;
  Fl_Double_Window  * winMain ;
  
  // === GUI Objects
  // == Visu
  Fl_Light_Button   *lbUDMX ;
  Fl_Light_Button   *lbArtnet ;  
  
  Fl_Progress       *progDMX[20] ;
  
  Fl_Browser        * browDebug ;
  
  // == Settings
  // Common
  Fl_Spinner        * spinVerbose ;
  Fl_Choice         * ddScheme ;
  
  // Artnet
  Fl_Check_Button   * chkANReuse ;

  Fl_Input_Choice   * ddANIp ;
  Fl_Input          * inANBc ;  
  
  // DMX
  Fl_Spinner        * spinDMXUniv ;
  Fl_Spinner        * spinDMXChans ; 

  // uDMX
	Fl_Output					* outSerial ;
  Fl_Spinner        * spinUDMXBs ;  
  Fl_Spinner        * spinUDMXTb ;    
  Fl_Spinner        * spinUDMXTm ;   
  Fl_Spinner        * spinUDMXTg ;     
  
  // === Data
  int   iVerbose ;
  int   iANReuse ;
  char  cANIp[80] ;
  char  cANBc[80] ;  
  int   iDMXUniv ;
  int   iDMXChans ;
  int   iUDMXBs ;
  int   iUDMXTb ;
  int   iUDMXTm ;
  int   iUDMXTg ; 
  
  bool	bSerial ;
  char  cSerial[80] ;
  char  cConfig[80] ;
  
  //
  int   bind2Ip ;
  char * ip_addr ;
	int   port_addr ;  
	int   num_ports ;  
	uint8_t dmx[ARTNET_MAX_PORTS][ARTNET_MAX_DMX +1] ;
  int   DmxLen ;  // used by Thread ! 

  	// Artnet-specific
	bool  				AnRunning ;
  bool          AnRestart ;
	artnet_node		AnNode ;
  
  // Trace / Debug
  int   TrcID ;
  StringList trcList ;

  Mutex trcMutex ;
  
} UIParams, *pUIParams ;

void PositionSave(pUIParams pUI) ;

class UserInterface
{
public:
  Fl_Double_Window* make_window(UIParams *pUI);
  
  bool  SettingsRead(pUIParams pUI) ;
  bool  LayoutRead(pUIParams pUI) ;
  bool  Trace(pUIParams pUI, int level, const char *fmt, ...) ;
};
#endif
