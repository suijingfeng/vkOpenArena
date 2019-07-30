#include <windows.h>
#include "win_public.h"

#include "../client/client.h"

// sys_consoleInput
#include "win_sysconsole.h"
#include "win_net.h"



#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

extern WinVars_t g_wv;

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead;
static int eventTail;
static unsigned char sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void * const ptr)
{
	sysEvent_t* const ev = &eventQue[eventHead & MASK_QUED_EVENTS];

	if (eventHead - eventTail >= MAX_QUED_EVENTS)
	{
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if (ev->evPtr) {
			Z_Free(ev->evPtr);
		}
		++eventTail;
	}

	++eventHead;


	ev->evTime = (time == 0 ? Sys_Milliseconds() : time);
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

void getNetworkEvent(void)
{
	// check for network packets
	msg_t netmsg;
	netadr_t adr;
	MSG_Init(&netmsg, sys_packetReceived, sizeof(sys_packetReceived));
	if (Sys_GetPacket(&adr, &netmsg))
	{
		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		int len = sizeof(netadr_t) + netmsg.cursize - netmsg.readcount;
		netadr_t* buf = (netadr_t*)Z_Malloc(len);
		*buf = adr;
		memcpy(buf + 1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount);
		Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
	}
}

void getConsoleEvents(void)
{
	// check for console commands
	char* s = Sys_ConsoleInput();
	if (s)
	{
		int len = (int)strlen(s) + 1;
		char* b = (char*)Z_Malloc(len);
		Q_strncpyz(b, s, len - 1);
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
	}
}

/*
================
Sys_GetEvent
EVENT LOOP
================
*/
sysEvent_t Sys_GetEvent(void)
{
	MSG	msg;

	// return if we have data
	if (eventHead > eventTail)
	{
		++eventTail;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// pump the message loop
	// Dispatches incoming sent messages, checks the thread message queue 
	// for a posted message, and retrieves the message (if any exist).
	while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) )
	{
		if (!GetMessage(&msg, NULL, 0, 0)) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	getConsoleEvents();

	getNetworkEvent();

	// return if we have data
	if (eventHead > eventTail)
	{
		++eventTail;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// create an empty event to return
	sysEvent_t ev;
	memset(&ev, 0, sizeof(ev));
	ev.evTime = timeGetTime();

	return ev;
}
