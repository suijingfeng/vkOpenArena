#pragma once


// Input subsystem
void IN_Init(void);
void IN_Shutdown(void);

void IN_Activate(qboolean active);
void IN_Frame(void);
void IN_MouseEvent(int mstate);