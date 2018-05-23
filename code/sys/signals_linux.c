/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include <signal.h>
#include "sys_local.h"


void Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;
	if( signalcaught )
	{
		fprintf( stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n", signal );
	}
	else
	{
		signalcaught = qtrue;
		VM_Forced_Unload_Start();

        char msg[128];
        sprintf(msg, "Signal caught (%d)", signal);

#ifndef DEDICATED
		CL_Shutdown(msg, qtrue, qtrue);
#endif
		SV_Shutdown(msg);
		VM_Forced_Unload_Done();
	}
    //If SIGCONT is received, reinitialize console
    if(signal == SIGCONT)
        CON_Init();
    else if( signal == SIGTERM || signal == SIGINT )
		Sys_Exit( 1 );
	else
		Sys_Exit( 2 );
}



void Sys_InitSignal(void)
{
	signal( SIGINT, Sys_SigHandler );
	signal( SIGILL, Sys_SigHandler );
	signal( SIGTERM, Sys_SigHandler );
	signal( SIGFPE, Sys_SigHandler );
	signal( SIGSEGV, Sys_SigHandler );

    signal( SIGHUP, Sys_SigHandler );
	signal( SIGQUIT, Sys_SigHandler );
	signal( SIGTRAP, Sys_SigHandler );
	signal( SIGABRT, Sys_SigHandler );
	signal( SIGBUS, Sys_SigHandler );

    //	signal( SIGIOT, signal_handler );
    
    // If the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

    // Reinitialize console input after receiving SIGCONT, 
    // as on Linux the terminal seems to lose all set attributes
    // if user did CTRL+Z and then does fg again.

	signal(SIGCONT, Sys_SigHandler);
}
