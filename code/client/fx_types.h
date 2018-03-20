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
// fx_types.h -- shared fx types
typedef int fxHandle_t;

#define FXP_ORIGIN		0x0001
#define FXP_VELOCITY	0x0002
#define FXP_DIR			0x0004
#define FXP_ANGLES		0x0008
#define FXP_SIZE		0x0010
#define FXP_WIDTH		0x0020
#define FXP_COLOR		0x0040
#define FXP_SHADER		0x0080
#define FXP_MODEL		0x0100
#define FXP_AXIS		0x0200
#define FXP_LIFE		0x0400

typedef struct {
	int			flags;
	vec3_t		origin;
	vec3_t		velocity;
	vec3_t		dir;
	color4ub_t	color;
	qhandle_t	shader;
	float		life;
	float		size;
	float		width;
	qhandle_t	model;
	vec3_t		angles;
	vec3_t		axis[3];
	color4ub_t	color2;
} fxParent_t;
