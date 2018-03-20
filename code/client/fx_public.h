/*
===========================================================================
Copyright (C) 2006 Sjoerd van der Berg ( harekiet @ gmail.com )

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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// fx_public.h -- public fx definitions

#include "fx_types.h"

void FX_Init( void );
void FX_Shutdown( void );
fxHandle_t FX_Register( const char *name );

void FX_Debug( void );
void FX_Reset( void );
void FX_Begin( int time, float fraction );
void FX_End( void );
void FX_Run( fxHandle_t handle, const fxParent_t *parent, unsigned int key );
void FX_VibrateView( float scale, vec3_t origin, vec3_t angles );
