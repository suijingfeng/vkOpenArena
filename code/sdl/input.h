#ifndef INPUT_H_
#define INPUT_H_

// input system
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);
void IN_Restart(void);

//qboolean IN_MouseActive( void );

//void Sys_SendKeyEvents(void);
char* Sys_GetClipboardData( void );	// note that this isn't journaled...
#endif
