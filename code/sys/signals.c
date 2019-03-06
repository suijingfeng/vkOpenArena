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
        exit( 1 );
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

#ifndef WIN32	
    //If SIGCONT is received, reinitialize console
    if(signal == SIGCONT)
        CON_Init();
    else
#endif
	if( signal == SIGTERM || signal == SIGINT )
		Sys_Exit( 1 );
	else
		Sys_Exit( 2 );
}



void Sys_InitSignal(void)
{
    // The SIGINT signal is sent to a process by its controlling terminal
    // when a user wishes to interrupt the process. 
    // This is typically initiated by pressing Ctrl+C.
	signal( SIGINT, Sys_SigHandler );
	
    // The SIGILL signal is sent to a process when it attempts to 
    // execute an illegal, malformed, unknown, or privileged instruction.
    signal( SIGILL, Sys_SigHandler );

    // The SIGTERM signal is sent to a process to request its termination. 
    // Unlike the SIGKILL signal, it can be caught and interpreted or 
    // ignored by the process. This allows the process to perform nice
    // termination releasing resources and saving state if appropriate.
    // SIGINT is nearly identical to SIGTERM.
	signal( SIGTERM, Sys_SigHandler );

    // The SIGSEGV signal is sent to a process when it makes an invalid 
    // virtual memory reference, or segmentation fault, i.e. when it 
    // performs a segmentation violation.
	signal( SIGSEGV, Sys_SigHandler );

    // The SIGABRT and SIGIOT signal is sent to a process to tell it to abort,
    // i.e. to terminate. The signal is usually initiated by the process itself 
    // when it calls abort() function of the C Standard Library, 
    // but it can be sent to the process from outside like any other signal.
	signal( SIGABRT, Sys_SigHandler );
	
#ifndef WIN32
    //The SIGHUP signal is sent to a process when its controlling terminal is closed.
    //It was originally designed to notify the process of a serial line drop (a hangup).
    //In modern systems, this signal usually means that the controlling pseudo or
    //virtual terminal has been closed. Many daemons will reload their configuration
    //files and reopen their logfiles instead of exiting when receiving this signal.
    //nohup is a command to make a command ignore the signal.
    signal( SIGHUP, Sys_SigHandler );

    // The SIGQUIT signal is sent to a process by its controlling terminal 
    // when the user requests that the process quit and perform a core dump.
	signal( SIGQUIT, Sys_SigHandler );

    //The SIGTRAP signal is sent to a process when an exception (or trap) occurs:
    //a condition that a debugger has requested to be informed of, for example, 
    // when a particular function is executed, or when a particular variable changes value.
	signal( SIGTRAP, Sys_SigHandler );


    // The SIGBUS signal is sent to a process when it causes a bus error.
    // The conditions that lead to the signal being sent are, for example, 
    // incorrect memory access alignment or non-existent physical address
	signal( SIGBUS, Sys_SigHandler );

    //	signal( SIGIOT, signal_handler );
    
    // If the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
   
    // The SIGTTIN and SIGTTOU signals are sent to a process when it attempts
    // to read in or write out respectively from the tty while in the background.
    // Typically, these signals are received only by processes under job control;
    // daemons do not have controlling terminals and, 
    // therefore, should never receive these signals.
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

    // Reinitialize console input after receiving SIGCONT, 
    // as on Linux the terminal seems to lose all set attributes
    // if user did CTRL+Z and then does fg again.

    // The SIGCONT signal instructs the operating system to continue (restart)
    // a process previously paused by the SIGSTOP or SIGTSTP signal. 
    // One important use of this signal is in job control in the Unix shell.
	signal(SIGCONT, Sys_SigHandler);
#endif
}



