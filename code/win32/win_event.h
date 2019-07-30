#pragma once

#include "../qcommon/qcommon.h"

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void * const ptr );
sysEvent_t Sys_GetEvent( void );