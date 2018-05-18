#ifndef SYS_INPUTS_H_
#define SYS_INPUTS_H_

#include "local.h"

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask )



// input system
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);

void IN_ActivateMouse( void );
void IN_DeactivateMouse( void );
qboolean IN_MouseActive( void );

void Sys_SendKeyEvents(void);

/*****************************************************************************
** KEYBOARD
** NOTE TTimo the keyboard handling is done with KeySyms
**   that means relying on the keyboard mapping provided by X
**   in-game it would probably be better to use KeyCode (i.e. hardware key codes)
**   you would still need the KeySyms in some cases, such as for the console and all entry textboxes
**     (cause there's nothing worse than a qwerty mapping on a french keyboard)
**
** you can turn on some debugging and verbose of the keyboard code with #define KBD_DBG
******************************************************************************/

#endif
