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

// fx_parse.c -- Parse and load FX Scripts and handle the fx related math

#include "client.h"
#include "fx_local.h"

fxMain_t fx;

#define parentOfs(m)   ((size_t)&((((fxParent_t*)0)->m)))
#define parentSize(m)   (sizeof((((fxParent_t*)0)->m)) / sizeof(float))
#define runOfs(m)   ((size_t)&((((fxRun_t*)0)->m)))
#define runSize(m)   (sizeof((((fxRun_t*)0)->m)) / sizeof(float))

static fxScript_t *fxFindScript( const char *name );

static unsigned int fxGenerateHash( const char *name ) {
	unsigned int hash = 0;
	while (*name) {
		char c = tolower(name[0]);
		hash = (hash << 5 ) ^ (hash >> 27) ^ c;
		name++;
	}
	return (hash ^ (hash >> FX_HASH_SHIFT) ^ (hash >> (FX_HASH_SHIFT*2))) & FX_HASH_MASK;
}

typedef enum {
	fxParentScript,
	fxParentEmitter,
} fxParseParent_t;

typedef struct {
	const char *last, *text, *name, *line;
	int lineCount;
	char token[1024];
	fxParseParent_t parent;
	fxRunEmitter_t *emitter;
	int loopDepth;
	unsigned int readMask, writeMask, vectorMask;
} fxParse_t;

typedef struct {
	int size, used;
	byte *data;
} fxParseOut_t;


static ID_INLINE void fxParseWrite( fxParse_t *parse, unsigned int mask ) {
	parse->writeMask |= mask;
}

static ID_INLINE void fxParseRead( fxParse_t *parse, unsigned int mask ) {
	mask &= ~parse->writeMask;
	parse->readMask |= mask;
}

qboolean fxParseError( fxParse_t *parse, const char *fmt, ... ) {
	va_list		argptr;
	char		msg[512];
	char		line[512];
	int			i = 0;

	if ( parse->line ) {
		while (i < sizeof( line ) - 1) {
			if ( !parse->line[i] )
				break;
			if ( parse->line[i] == '\n' )
				break;
			if ( parse->line[i] == '\r' )
				break;
			line[i] = parse->line[i];
			i++;
		}
	}
	line[i] = 0;

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Printf( S_COLOR_YELLOW "fxError:'%s' in line '%s' in script '%s'\n", msg ,line, parse->name );
	return qfalse;
}

qboolean fxParseErrorIllegal( fxParse_t *parse ) {
	return fxParseError( parse, "Illegal token %s", parse->token );
}

static void *fxParseAlloc( int size ) {
	return Z_TagMalloc( size, TAG_FX );
}
static void fxParseFree( void *free ) {
	Z_Free( free );
}

static qboolean fxParseCreateMask( fxParse_t *parse, byte *table, int tableLen, unsigned short *allocAdd ) {
	byte modMask[ runOfs( stackUsed ) / 4 ];
	unsigned int mask = parse->readMask;
	int modeCount, maskIndex, maskLast, tableIndex = 0;
	int totalCount;
	byte checkVal;

	memset( modMask, 0, sizeof( modMask ));
#define MODSET( m ) memset( modMask + (runOfs( m ) / 4), 1, runSize( m ) );
#if 0
	if ( mask & FX_MASK_ORIGIN )	MODSET( origin );
	if ( mask & FX_MASK_VELOCITY )	MODSET( velocity );
#endif 
	if ( mask & FX_MASK_SHADER )	MODSET( shader );
	if ( mask & FX_MASK_SIZE )		MODSET( size );
	if ( mask & FX_MASK_ROTATE )	MODSET( rotate );
	if ( mask & FX_MASK_COLOR )		MODSET( color );

	if ( mask & FX_MASK_PARENT )	MODSET( parent );
	if ( mask & FX_MASK_DIR )		MODSET( dir );
	if ( mask & FX_MASK_MODEL )		MODSET( model );
	if ( mask & FX_MASK_AXIS )		MODSET( axis );
	if ( mask & FX_MASK_ANGLES )	MODSET( angles );
	if ( mask & FX_MASK_WIDTH )		MODSET( width );
	if ( mask & FX_MASK_T0 )		MODSET( t0 );
	if ( mask & FX_MASK_T1 )		MODSET( t1 );
	if ( mask & FX_MASK_T2 )		MODSET( t2 );
	if ( mask & FX_MASK_T3 )		MODSET( t3 );
	if ( mask & FX_MASK_V0 )		MODSET( v0 );
	if ( mask & FX_MASK_V1 )		MODSET( v1 );
	if ( mask & FX_MASK_V2 )		MODSET( v2 );
	if ( mask & FX_MASK_V3 )		MODSET( v3 );
#undef MODSET

	//This is all somewhat hacky, but seems to work
	maskIndex = 3 + runOfs( velocity ) / 4;
	maskLast = runOfs( parent ) / 4 + runSize( parent );
	modeCount = 0;
	totalCount = 0;
	checkVal = 0;

	while ( maskIndex < maskLast ) {
		if ( checkVal == modMask[maskIndex] ) {
			maskIndex++;
			modeCount++;
			totalCount += checkVal;
		} else {
			if ( tableIndex >= tableLen  )
				return fxParseError( parse, "Ran out of write mask space" );
			checkVal ^= 1;
			table[tableIndex++] = modeCount;
			modeCount = 0;
		}
	}
	if ( tableIndex >= tableLen  )
		return fxParseError( parse, "Ran out of write mask space" );
	table[tableIndex++] = modeCount;
	if (modeCount) {
		if ( tableIndex >= tableLen  )
			return fxParseError( parse, "Ran out of write mask space" );
		table[tableIndex++] = 0;
	}
	*allocAdd += totalCount * 4;
	return qtrue;
}

#define SEPERATOR(_T_) 	((_T_ == ' ') || (_T_ == '\n') || (_T_ == '\t') || (_T_ == '\r'))

static qboolean fxParseToken( fxParse_t *parse, qboolean skipLine ) {
	const char *p = parse->text;
	char *token = parse->token;
	qboolean haveQuote = qfalse;
	int tokenLength = 0;
	
	if (!parse->line)
		parse->line = p;

	parse->last = p;
	while (1) {
		if ( tokenLength >= sizeof( parse->token )) {
			parse->token[ sizeof( parse->token ) -1 ] = 0;
			return fxParseError( parse, "token %s too long", parse->token );
		}
		if ( haveQuote ) {
			if (*p == '\n' || *p == '\r' || *p == 0) {
				return fxParseError( parse, "token missing end quote" );
			}
			if ( *p == '\"' ) {
				if (!SEPERATOR(p[1]) && !p[1])
					return fxParseError( parse, "token quote followed by a character" );
				haveQuote = qfalse;
				p++;
				break;
			}
			token[tokenLength++] = p[0];
		} else if ( tokenLength ) {
			if (SEPERATOR(p[0]) || !p[0])
				break;
			token[tokenLength++] = p[0];
		} else if (!p[0]) {
			break;
		} else if (p[0] == '"' ) {
			haveQuote = qtrue;
		} else if (SEPERATOR(p[0])) {
			if ( p[0] == '\n' || p[0] == '\r' ) {
				if ( !skipLine )
					break;
				parse->line = p+1;
				parse->last = 0;
			}
		} else {
			token[tokenLength++] = p[0];
		}
		p++;
	}
	parse->text = p;
	token[tokenLength] = 0;
	if (!tokenLength ) 
		return qfalse;
	return qtrue;
}

static qboolean fxParseReverse( fxParse_t *parse ) {
	if (!parse->last)
		return fxParseError( parse, "Can't reverse a token" );
	parse->text = parse->last;
	return qtrue;
}

static qboolean fxParseWord( fxParse_t *parse, const char *type ) {
	if (!fxParseToken( parse, qfalse )) {
		if ( !type )
			type = "word";
		return fxParseError( parse, "Missing %s" );
	}
	return qtrue;
}

static qboolean fxParseCompare( fxParse_t *parse, const char *compare ) {
	if (!fxParseWord( parse, compare )) {
		return qfalse;
	}
	if (Q_stricmp( parse->token, compare ))
		return fxParseError( parse, "%s doesn't match %s", parse->token, compare );
	return qtrue;
}

typedef qboolean (*fxListMatch_t)( fxParse_t *, void * );

static qboolean fxParseList( fxParse_t *parse, const char *list, fxListMatch_t match, int dataSize, void *data, int countMax, unsigned int *count ) {
	const char *token = parse->token;

	if (!fxParseCompare( parse, "{"))
		return qfalse;
	*count = 0;
	while ( 1 ) {
		if (!fxParseToken( parse, qtrue )) {
			return fxParseError( parse, "Expected %s entry", list ); 
		}
		if (!Q_stricmp( token, "}"))
			break;
		if (!match( parse, ((char *)data) + *count * dataSize ))
			return qfalse;
		(*count)++;
		if ( *count >= countMax )
			return fxParseError( parse, "Too many entries for %s (max %d)", list, countMax );
	}
	if (!*count) 
		return fxParseError( parse, "%s is empty", list );
	return qtrue;
}

static qboolean fxMatchValidFloat( fxParse_t *parse ) {
	int doneNumber = 0;
	int donePoint = 0;

	char *token = parse->token;
	for ( ;1;token++) {
		switch (*token) {
		case 0:
			if (!doneNumber)
				return fxParseError( parse, "empty float", parse->token );
			return qtrue;
		case '0':	case '1':	case '2':	case '3':	case '4':
		case '5':	case '6':	case '7':	case '8':	case '9':
			doneNumber++;
			continue;
		case '.':
			if ( donePoint )
				return fxParseError( parse, "invalid float %s", parse->token );
			donePoint++;
			continue;
		default:
			return fxParseError( parse, "illegal char %c in float %s", *token, parse->token );
		}
	}
	return qfalse;
}

static qboolean fxParseFloat( fxParse_t *parse, float *data ) {
	if (!fxParseToken( parse, qfalse )) {
		return fxParseError( parse, "Expected float" );
	}
	if (!fxMatchValidFloat( parse ))
		return qfalse;
	//TODO check for valid float?
	*data = atof( parse->token );
	return qtrue;
}

/* Bit of a hack to parse the second 2 values */
static qboolean fxMatchColor( fxParse_t *parse, byte *data ) {
	int i;
	for (i = 0;i<3;i++) {
		float f;
		if (!i) {
			if (!fxMatchValidFloat( parse ))
			 return qfalse;
			f = atof( parse->token );
		} else if (!fxParseFloat( parse, &f)) {
			return qfalse;
		}
		if ( f < 0 )
			f = 0;
		else if ( f > 1)
			f = 1;
		data[i] = f * 255;
	}
	return qtrue;
}

static qboolean fxMatchShader( fxParse_t *parse, qhandle_t *data ) {
	data[0] =  re.RegisterShader( parse->token );
	if (!data[0])
		return fxParseError( parse, "shader %s not found", parse->token );
	return qtrue;
}

static qboolean fxMatchModel( fxParse_t *parse, qhandle_t *data ) {
	data[0] = re.RegisterModel( parse->token );
	if (!data[0])
		return fxParseError( parse, "model %s not found", parse->token );
	return qtrue;
}

static qboolean fxMatchSound( fxParse_t *parse, sfxHandle_t *data ) {
	data[0] = S_RegisterSound( parse->token, qfalse );
	if (!data[0])
		return fxParseError( parse, "Sound %s not found", parse->token );
	return qtrue;
}

static qboolean fxParseColor( fxParse_t *parse, byte *data ) {
	int i;
	for (i = 0;i<3;i++) {
		float f;
		if (!fxParseFloat( parse, &f )) {
			return qfalse;
		}
		if ( f < 0 )
			f = 0;
		else if ( f > 1)
			f = 1;
		data[i] = f * 255;
	}
	return qtrue;
}

static qboolean fxParseRenderFX( fxParse_t *parse, int *renderFX ) {
	*renderFX = RF_NOSHADOW;
	while (fxParseToken( parse, qfalse ) ) {
		const char *token = parse->token;
		if (!Q_stricmp(token, "firstPerson"))  {
			*renderFX |= RF_FIRST_PERSON;
		} else if (!Q_stricmp(token, "thirdPerson"))  {
			*renderFX |= RF_THIRD_PERSON;
		} else if (!Q_stricmp(token, "shadow"))  {
			*renderFX &= ~RF_NOSHADOW;
		}  else if (!Q_stricmp(token, "cullNear") || !Q_stricmp(token, "cullRadius" ) )  {
			*renderFX |= RF_CULLRADIUS;
		}  else if (!Q_stricmp(token, "depthHack")) {
			*renderFX |= RF_DEPTHHACK;
		}  else if (!Q_stricmp(token, "stencil")) {
			*renderFX |= RF_STENCIL;
		} else {
			return fxParseError( parse, "Illegal render keyword %s", token );	
		}
	}
	return qtrue;
}


static qboolean fxParseOut( fxParse_t *parse, fxParseOut_t *out, const void *data, int size ) {
	if ( out->used + size + sizeof( void *) > out->size ) {
		fxParseError( parse, "Overflowing output buffer" );
		return qfalse;
	}
	memcpy( out->data + out->used, data, size );
	out->used += size;
	return qtrue;
}

static qboolean fxParseOutCmd( fxParse_t *parse, fxParseOut_t *out, int cmd, const void *data, int size ) {
	if (!fxParseOut( parse, out, &cmd, sizeof( cmd ) ))
		return qfalse;
	if (!fxParseOut( parse, out, data, size ))
		return qfalse;
	return qtrue;
}

typedef struct {
	fxRun_t *run;
	const int *data;
} fxMathInfo_t;

typedef float (*fxMathHandler)( fxMathInfo_t *info );

static float ID_INLINE fxMath( fxMathInfo_t *info ) {
	fxMathHandler * handler = ( fxMathHandler *)info->data;
	info->data++;
	return (*handler)( info );
}

const void *fxRunMath( fxRun_t *run, const void *data, float *value ) {
	fxMathInfo_t info;
	info.run = run;
	info.data = data;

	*value = fxMath( &info );
	return info.data;
}
static float fxMathNeg( fxMathInfo_t *info ) {
	return -fxMath( info );
}
static float fxMathMul( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return v1 * v2;
}
static float fxMathDiv( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return v1 / v2;
}
static float fxMathAdd( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return v1 + v2;
}
static float fxMathSub( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return v1 - v2;
}

static float fxMathEqual( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return ((v1 != 0) == (v2 != 0)) ? 1 : 0;
}
static float fxMathNotEqual( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return ((v1 != 0) != (v2 != 0)) ? 1 : 0;
}

static float fxMathTE( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return ( v1 == v2 ) ? 1 : 0;
}
static float fxMathTNE( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return ( v1 != v2 ) ? 1 : 0;
}
static float fxMathTG( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 > v2) ? 1 : 0;
}
static float fxMathTGE( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 >= v2) ? 1 : 0;
}
static float fxMathTS( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 < v2) ? 1 : 0;
}
static float fxMathTSE( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 <= v2) ? 1 : 0;
}

static float fxMathAND( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 && v2) ? 1 : 0;
}

static float fxMathOR( fxMathInfo_t *info ) {
	float v1 = fxMath( info );
	float v2 = fxMath( info );
	return (v1 || v2) ? 1 : 0;
}

static float fxMathSin( fxMathInfo_t *info ) {
	return sinf( DEG2RAD( fxMath( info )));
}
static float fxMathCos( fxMathInfo_t *info ) {
	return cosf( DEG2RAD( fxMath( info )));
}
static float fxMathWave( fxMathInfo_t *info ) {
	return sinf( fxMath( info ) * 2 * M_PI );
}
static float fxMathSqrt( fxMathInfo_t *info ) {
	return sqrtf( fxMath( info ));
}
static float fxMathCeil( fxMathInfo_t *info ) {
	return ceilf( fxMath( info ));
}
static float fxMathFloor( fxMathInfo_t *info ) {
	return floorf( fxMath( info ));
}
static float fxMathClip( fxMathInfo_t *info) {
	float v = fxMath( info );
	if ( v < 0.0f)
		return 0.0f;
	if ( v > 1.0f )
		return 1.0f;
	return v;
}
static float fxMathZero( fxMathInfo_t *info ) {
	return 0;
}
static float fxMathPi( fxMathInfo_t *info ) {
	return M_PI;
}
static float fxMathRed( fxMathInfo_t *info ) {
	return info->run->color[0] * (1.0f/255);
}
static float fxMathGreen( fxMathInfo_t *info ) {
	return info->run->color[1] * (1.0f/255);
}
static float fxMathBlue( fxMathInfo_t *info ) {
	return info->run->color[2] * (1.0f/255);
}
static float fxMathAlpha( fxMathInfo_t *info ) {
	return info->run->color[3] * (1.0f/255);
}
static float fxMathValue( fxMathInfo_t *info ) {
	int ofs = info->data[0];
	float *v = (float *)(((char *)info->run) + ofs);
	info->data++;
	return v[0];
}
static float fxMathLength( fxMathInfo_t *info ) {
	int ofs = info->data[0];
	const float *v = fxFloatSrc( info->run, ofs );
	info->data++;
	return VectorLength( v );
}
static float fxMathParent( fxMathInfo_t *info ) {
	int ofs = info->data[0];
	float *v = (float *)(((char *)info->run->parent) + ofs);
	info->data++;
	return v[0];
}
static float fxMathConst( fxMathInfo_t *info) {
	float v1 = ((float *)info->data)[0];
	info->data++;
	return v1;
}
static float fxMathTime( fxMathInfo_t *info) {
	return fx.time * 0.001f;
}
static float fxMathRand( fxMathInfo_t *info) {
	fx.seed = fx.seed * 16091 + 1;
	return (fx.seed & 0xffff) * (1.0f / 0x10000);
}
static float fxMathCrand( fxMathInfo_t *info) {
	fx.seed = fx.seed * 16091 + 1;
	return 1.0f - (2.0f / 0x10000) * (fx.seed & 0xffff);
}
static float fxMathCvar( fxMathInfo_t *info ) {
	cvar_t *cvar;
	memcpy( &cvar, info->data, sizeof( cvar_t *));
	info->data += sizeof( cvar_t *) / sizeof( int );
	return cvar->value;
}

static qboolean fxParseParentValue( const char *val, int *ofs ) {
	if (0) {
	} else if (!Q_stricmp( val, "parentorigin0")) {
		*ofs = parentOfs( origin[0] );
	} else if (!Q_stricmp( val, "parentorigin1")) {
		*ofs = parentOfs( origin[1] );
	} else if (!Q_stricmp( val, "parentorigin2")) {
		*ofs = parentOfs( origin[2] );
	} else if (!Q_stricmp( val, "parentvelocity0")) {
		*ofs = parentOfs( velocity[0] );
	} else if (!Q_stricmp( val, "parentvelocity1")) {
		*ofs = parentOfs( velocity[1] );
	} else if (!Q_stricmp( val, "parentvelocity2")) {
		*ofs = parentOfs( velocity[2] );
	} else if (!Q_stricmp( val, "parentangles0")) {
		*ofs = parentOfs( angles[0] );
	} else if (!Q_stricmp( val, "parentangles1")) {
		*ofs = parentOfs( angles[1] );
	} else if (!Q_stricmp( val, "parentangles2")) {
		*ofs = parentOfs( angles[2] );
	} else if (!Q_stricmp( val, "parentyaw")) {
		*ofs = parentOfs( angles[YAW] );
	} else if (!Q_stricmp( val, "parentpitch")) {
		*ofs = parentOfs( angles[PITCH] );
	} else if (!Q_stricmp( val, "parentroll")) {
		*ofs = parentOfs( angles[ROLL] );
	} else if (!Q_stricmp( val, "parentangle")) {
		*ofs = parentOfs( angles[ROLL] );
	} else if (!Q_stricmp( val, "parentsize")) {
		*ofs = parentOfs( size );
	} else {
		return qfalse;
	}
	return qtrue;
}


#define PARSE_RUN_VECTOR_ENTRY( _VEC, _MASK, _NUM )		\
	if (!Q_stricmp( val, #_VEC #_NUM)) {			\
		*ofs = runOfs( _VEC[_NUM] );				\
		*mask = _MASK;								\
	} 


#define PARSE_RUN_VECTOR( _VEC, _MASK )		\
	PARSE_RUN_VECTOR_ENTRY( _VEC, _MASK, 0 )	\
	else								\
	PARSE_RUN_VECTOR_ENTRY( _VEC, _MASK, 1 )	\
	else								\
	PARSE_RUN_VECTOR_ENTRY( _VEC, _MASK, 2 ) 
 

static qboolean fxMatchFloat( const char *val, int *ofs, unsigned int *mask ) {
	PARSE_RUN_VECTOR( origin, FX_MASK_ORIGIN )
	else PARSE_RUN_VECTOR( velocity, FX_MASK_VELOCITY )
	else PARSE_RUN_VECTOR( angles, FX_MASK_ANGLES )
	else PARSE_RUN_VECTOR( dir, FX_MASK_DIR )
	else PARSE_RUN_VECTOR( v0, FX_MASK_V0 )
	else PARSE_RUN_VECTOR( v1, FX_MASK_V1 )
	else PARSE_RUN_VECTOR( v2, FX_MASK_V2 )
	else PARSE_RUN_VECTOR( v3, FX_MASK_V3 )
	else if (!Q_stricmp( val, "rotate") || !Q_stricmp( val, "angle")) {
		*ofs = runOfs( rotate );
		*mask = FX_MASK_ROTATE;
	} else if (!Q_stricmp( val, "size")) {
		*ofs = runOfs( size );
		*mask = FX_MASK_SIZE;
	} else if (!Q_stricmp( val, "width")) {
		*ofs = runOfs( width );
		*mask = FX_MASK_WIDTH;
	} else if (!Q_stricmp( val, "yaw")) {
		*ofs = runOfs( angles[YAW] );
		*mask = FX_MASK_ANGLES;
	} else if (!Q_stricmp( val, "pitch")) {
		*ofs = runOfs( angles[PITCH] );
		*mask = FX_MASK_ANGLES;
	} else if (!Q_stricmp( val, "roll")) {
		*ofs = runOfs( angles[ROLL] );
		*mask = FX_MASK_ANGLES;
	} else if (!Q_stricmp( val, "t0")) {
		*ofs = runOfs( t0 );
		*mask = FX_MASK_T0;
	} else if (!Q_stricmp( val, "t1")) {
		*ofs = runOfs( t1 );
		*mask = FX_MASK_T1;
	} else if (!Q_stricmp( val, "t2")) {
		*ofs = runOfs( t2 );
		*mask = FX_MASK_T2;
	} else if (!Q_stricmp( val, "t3")) {
		*ofs = runOfs( t3 );
		*mask = FX_MASK_T3;
	} else {
		return qfalse;
	}
	return qtrue;
}

static qboolean fxMatchVector( const char *val, int *ofs, unsigned int *mask ) {
	if (!Q_stricmp( val, "origin")) {
		*ofs = runOfs( origin );
		*mask = FX_MASK_ORIGIN;
	} else if (!Q_stricmp( val, "velocity")) {
		*ofs = runOfs( velocity );
		*mask = FX_MASK_VELOCITY;
	} else if (!Q_stricmp( val, "angles")) {
		*ofs = runOfs( angles );
		*mask = FX_MASK_ANGLES;
	} else if (!Q_stricmp( val, "dir")) {
		*ofs = runOfs( dir );
		*mask = FX_MASK_DIR;
	} else if (!Q_stricmp( val, "v0")) {
		*ofs = runOfs( v0 );
		*mask = FX_MASK_V0;
	} else if (!Q_stricmp( val, "v1")) {
		*ofs = runOfs( v1 );
		*mask = FX_MASK_V1;
	} else if (!Q_stricmp( val, "v2")) {
		*ofs = runOfs( v2 );
		*mask = FX_MASK_V2;
	} else if (!Q_stricmp( val, "v3")) {
		*ofs = runOfs( v3 );
		*mask = FX_MASK_V3;
	} else if (!Q_stricmp( val, "parentOrigin" )) {
		*ofs = parentOfs( origin ) | FX_VECTOR_PARENT;
		*mask = FX_MASK_PARENT;
	} else if (!Q_stricmp( val, "parentVelocity" )) {
		*ofs = parentOfs( velocity ) | FX_VECTOR_PARENT;
		*mask = FX_MASK_PARENT;
	} else if (!Q_stricmp( val, "parentAngles" )) {
		*ofs = parentOfs( angles ) | FX_VECTOR_PARENT;
		*mask = FX_MASK_PARENT;
	} else if (!Q_stricmp( val, "parentDir" )) {
		*ofs = parentOfs( dir ) | FX_VECTOR_PARENT;
		*mask = FX_MASK_PARENT;
	} else {
		return qfalse;
	}
	return qtrue;
}

static qboolean fxParseVectorOptional( fxParse_t *parse, int *ofs ) {
	unsigned int mask;
	
	if (!fxParseToken( parse, qfalse )) {
		mask = parse->vectorMask;
	} else if (!fxMatchVector( parse->token, ofs, &mask ) ) {
		mask = parse->vectorMask;
		if (!fxParseReverse( parse ))
			return qfalse;
	}
	if ( mask & FX_MASK_PARENT ) {
		return fxParseError( parse, "Can't write to a parent vector" );
	}
	fxParseWrite( parse, mask );
	return qtrue;
}

static qboolean fxParseVectorRead( fxParse_t *parse, int *ofs ) {
	unsigned int mask;
	if (!fxParseToken( parse, qfalse )) {
		return fxParseError( parse, "Expected read vector name" );
	}
	if ( !fxMatchVector( parse->token, ofs, &mask ) ) {
		return fxParseError( parse, "Unknown read vector name %s", parse->token );
	}
	parse->vectorMask = mask;
	fxParseRead( parse, mask );
	return qtrue;
}

static qboolean fxParseVectorWrite( fxParse_t *parse, int *ofs ) {
	unsigned int mask;
	if (!fxParseToken( parse, qfalse )) {
		return fxParseError( parse, "Expected write vector name" );
	}
	if ( !fxMatchVector( parse->token, ofs, &mask ) ) {
		return fxParseError( parse, "Unknown write vector name %s", parse->token );
	}
	if ( mask & FX_MASK_PARENT ) {
		return fxParseError( parse, "Can't write to a parent vector" );
	}
	fxParseWrite( parse, mask );
	return qtrue;
}

typedef enum {
	mathParseLine,
	mathParseSingle,
	mathParseMul,
} mathParseType_t;

typedef struct {
	byte data[1024];
	int used;
} fxMathOut_t;


static qboolean fxMathAppend( fxParse_t *parse, fxMathOut_t *out, const void *data, int size ) {
	if ( size + out->used > sizeof( out->data )) 
		return fxParseError( parse, "Overflowing math parser" );
	if ( !size || !data)
		return fxParseError( parse, "math adding 0 sized data" );
	memcpy( out->data + out->used, data, size );
	out->used += size;
	return qtrue;
}

static qboolean fxMathAppendOut( fxParse_t *parse, fxMathOut_t *out, const fxMathOut_t *add ) {
	return fxMathAppend( parse, out, add->data, add->used );
}

static qboolean fxMathAppendInt( fxParse_t *parse, fxMathOut_t *out, int val ) {
	return fxMathAppend( parse, out, &val, sizeof( int ));
}

static qboolean fxMathAppendHandler( fxParse_t *parse, fxMathOut_t *out, fxMathHandler handler ) {
	//TODO for 64bit platforms use a relative 32bit offset
	return fxMathAppend( parse, out, &handler, sizeof( handler ));
}

static qboolean fxMathParse( fxParse_t *parse, mathParseType_t parseType, char **line, const fxMathOut_t *in, fxMathOut_t *out );

static qboolean fxMathParseHandler( fxParse_t *parse, char **line, fxMathOut_t *out, fxMathHandler handler ) {
	if (!fxMathAppendHandler( parse, out, handler))
		return qfalse;
	return fxMathParse( parse, mathParseSingle, line, 0, out );
}


static qboolean fxMathParse( fxParse_t *parse, mathParseType_t parseType, char **line, const fxMathOut_t *in, fxMathOut_t *out ) {
	char buf[64];
	int wordSize;
	fxMathOut_t val;
	fxMathHandler tempHandler;

	wordSize = 0;
	val.used = 0;

	if ( parseType == mathParseLine && !in ) {
		if (!fxMathParse( parse, mathParseMul, line, 0, &val ))
			return qfalse;
		return fxMathParse( parse, mathParseLine, line, &val, out );
	}

	while ( 1 ) {
		char c = line[0][0];

		if ( wordSize ) {
			int i;
			qboolean isNumber;

			if ( !c || c == ')' || c == '(' || c == ' ' 
				|| c == '-' || c =='+' || c == '*' || c == '/'
				|| c == '!' || c == '<' || c == '>' || c == '=' ||
				c == '&' || c == '|' ) {
				/* Easier to just parse the value first incase of trouble */
				buf[wordSize] = 0;

				if ( val.used )
					return fxParseError( parse, "math double value %s", buf );
				
				isNumber = qtrue;
				for ( i = 0; i < wordSize;i++) {
					if (!(buf[i] == '.' ||  (buf[i] >= '0' && buf[i] <= '9'))) {
						isNumber = qfalse;
						break;
					}
				}
				
				if ( isNumber ) { 
					float value = atof( buf );
					if (!fxMathAppendHandler( parse, &val, fxMathConst ))
						return qfalse;
					if (!fxMathAppend( parse, &val, &value, sizeof( value )))
						return qfalse;
				} else if (!Q_stricmp( buf, "sin")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathSin ))
						return qfalse;
				} else if (!Q_stricmp( buf, "cos")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathCos ))
						return qfalse;
				} else if (!Q_stricmp( buf, "wave")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathWave))
						return qfalse;
				} else if (!Q_stricmp( buf, "sqrt")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathSqrt ))
						return qfalse;
				} else if (!Q_stricmp( buf, "ceil")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathCeil ))
						return qfalse;
				} else if (!Q_stricmp( buf, "floor")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathFloor ))
						return qfalse;
				} else if (!Q_stricmp( buf, "clip")) {
					if (!fxMathParseHandler( parse, line, &val, &fxMathClip ))
						return qfalse;
				} else if (!Q_stricmp( buf, "rand") || !Q_stricmp( buf, "rnd")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathRand))
						return qfalse;
				} else if (!Q_stricmp( buf, "time")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathTime))
						return qfalse;
				} else if (!Q_stricmp( buf, "crand")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathCrand))
						return qfalse;
				} else if (!Q_stricmp( buf, "pi")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathPi))
						return qfalse;
				} else if (!Q_stricmp( buf, "red")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathRed))
						return qfalse;
					fxParseRead( parse, FX_MASK_COLOR);
				} else if (!Q_stricmp( buf, "green")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathGreen))
						return qfalse;
					fxParseRead( parse, FX_MASK_COLOR);
				} else if (!Q_stricmp( buf, "blue")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathBlue))
						return qfalse;
					fxParseRead( parse, FX_MASK_COLOR);
				} else if (!Q_stricmp( buf, "alpha")) {
					if (!fxMathAppendHandler( parse, &val, &fxMathAlpha))
						return qfalse;
					fxParseRead( parse, FX_MASK_COLOR);
				} else if (!Q_stricmp( buf, "life")) {
					if ( !parse->emitter )
						return fxParseError( parse, "Can't access life outside of emitter" );
					if (!fxMathAppendHandler( parse, &val, &fxMathValue))
						return qfalse;
					if (!fxMathAppendInt( parse, &val, runOfs( life )))
						return qfalse;
				} else if (!Q_stricmp( buf, "lerp")) {
					if ( !parse->emitter )
						return fxParseError( parse, "Can't access lerp outside of emitter" );
					if (!fxMathAppendHandler( parse, &val, &fxMathValue))
						return qfalse;
					if (!fxMathAppendInt( parse, &val, runOfs( lerp )))
						return qfalse;
				} else if (!Q_stricmp( buf, "loop")) {
					if ( parse->loopDepth <= 0 )
						return fxParseError( parse, "Can't access loop outside of a loop" );
					if (!fxMathAppendHandler( parse, &val, &fxMathValue))
						return qfalse;
					if (!fxMathAppendInt( parse, &val, runOfs( loop )))
						return qfalse;
				} else {
					int ofs;
					unsigned int mask;
					cvar_t *cv;
					if (fxMatchFloat( buf, &ofs, &mask)) {
						parse->readMask |= mask;
						if (!fxMathAppendHandler( parse, &val, fxMathValue))
							return qfalse;
						if (!fxMathAppendInt( parse, &val, ofs ))
							return qfalse;
					} else if (fxMatchVector( buf, &ofs, &mask)) {
						parse->readMask |= mask;
						if (!fxMathAppendHandler( parse, &val, fxMathLength))
							return qfalse;
						if (!fxMathAppendInt( parse, &val, ofs ))
							return qfalse;
					} else if (fxParseParentValue( buf, &ofs)) {
						parse->readMask |= FX_MASK_PARENT;
						if (!fxMathAppendHandler( parse, &val, fxMathParent))
							return qfalse;
						if (!fxMathAppendInt( parse, &val, ofs ))
							return qfalse;
					} else if ( (cv = Cvar_FindVar( buf )) ) {
						if (!fxMathAppendHandler( parse, &val, &fxMathCvar))
							return qfalse;
						if (!fxMathAppend( parse, &val, &cv, sizeof( cv )))
							return qfalse;
					} else {
						return fxParseError( parse, "math unknown word %s", buf );
					}
				}
				wordSize = 0;
			} else {
				if (wordSize >= sizeof( buf ) - 1) 
					return fxParseError( parse, "math too long word %s", buf );
				buf[wordSize++] = c;
				line[0]++;
			}
			continue;
		}

		switch (c) {
		case '+':
		case '-':
			if ( parseType != mathParseLine ) {
				if ( val.used )
					return fxMathAppendOut( parse, out, &val );
				if ( c == '-' && !fxMathAppendHandler( parse, out, fxMathNeg ))
					return qfalse;
				line[0]++;
				return fxMathParse( parse, mathParseMul, line, 0, out );
			}
			line[0]++;
			if (!fxMathAppendHandler( parse, &val, c == '+' ? fxMathAdd : fxMathSub ))
				return qfalse;
			if (!fxMathAppendOut( parse, &val, in ))
				return qfalse;
			if (!fxMathParse( parse, mathParseMul, line, 0, &val ))
				return qfalse;
			return fxMathParse( parse, mathParseLine, line, &val, out );
		case '!':
		case '<':
		case '>':
		case '=':
		case '&':
		case '|':
			if ( parseType != mathParseLine ) {
				if (!val.used )
					return fxParseError( parse, "Expected value before %c", c );
				return fxMathAppendOut( parse, out, &val );
			}
			switch (c) {
			case '&':
				if ( line[0][1] == '&' )
					line[0]++;
				tempHandler = fxMathAND;
				break;
			case '|':
				if ( line[0][1] == '|' )
					line[0]++;
				tempHandler = fxMathOR;
				break;
			case '<':
				if ( line[0][1] == '=' ) {
					line[0]++;
					tempHandler = fxMathTSE;
				} else {
					tempHandler = fxMathTS;
				}
				break;
			case '>':
				if ( line[0][1] == '=' ) {
					line[0]++;
					tempHandler = fxMathTGE;
				} else {
					tempHandler = fxMathTG;
				}
				break;
			case '!':
				if ( line[0][1] == '=' ) {
					line[0]++;
					tempHandler = fxMathNotEqual;
				} else {
					tempHandler = fxMathTNE;
				}
				break;
			case '=':
				if ( line[0][1] == '=' ) {
					line[0]++;
					tempHandler = fxMathEqual;
				} else {
					tempHandler = fxMathTE;
				}
				break;
			default:
				return fxParseError( parse, "math unknown char '%c'", c );
			}
			line[0]++;
			if (!fxMathAppendHandler( parse, out, tempHandler ))
				return qfalse;
			if (!fxMathAppendOut( parse, out, in ))
				return qfalse;
			return fxMathParse( parse, mathParseLine, line, 0, out );
		case '*':
		case '/':
			if (!val.used) {
				return fxParseError( parse, "math unexpected %c", c );
			}
			if ( parseType != mathParseMul )
				return fxMathAppendOut( parse, out, &val );
			line[0]++;
			if (!fxMathAppendHandler( parse, out, c == '*' ? fxMathMul : fxMathDiv ))
				return qfalse;
			if (!fxMathAppendOut( parse, out, &val ))
				return qfalse;
			return fxMathParse( parse, mathParseMul, line, 0, out );
		case '(':
			if ( val.used || parseType == mathParseLine)
				return fxParseError( parse, "math unexpected (" );
			line[0]++;
			if (!fxMathParse( parse, mathParseLine, line, 0, &val ))
				return qfalse;
			if (line[0][0] != ')' ) 
				return fxParseError( parse, "math expected )" );
			line[0]++;
			continue;
		case ')':
			if ( parseType == mathParseLine ) {
				if (!in || !in->used ) 
					return fxParseError( parse, "math:unexpected )");
				return fxMathAppendOut( parse, out, in );
			}
			return fxMathAppendOut( parse, out, &val );
		case 0:
			if ( parseType == mathParseLine ) {
				if (!in )
					return fxParseError( parse, "math empty line?" );
				return fxMathAppendOut( parse, out, in );
			}
			if ( !val.used )
				return fxParseError( parse, "math expected some value" );
			return fxMathAppendOut( parse, out, &val );
		case ' ':
			line[0]++;
			continue;
		default:
			if ( val.used )
				return fxParseError( parse, "math unexpected char %c", c );
			buf[wordSize++] = c;
			line[0]++;
			continue;
		}
		//Final break that only reaches when we end this parse
		break;
	}
	return qfalse;
}

static qboolean fxParseMath( fxParse_t *parse, fxParseOut_t *out ) {
	char buf[1024];
	int i;
	const char *p;
	char *line;
	fxMathOut_t mathOut;

	p = parse->text;
	parse->last = p;
	i = 0;

	for( ;; p++ ) {
		switch (*p) {
		case '\n':
		case '\r':
		case '{':
		case 0:
			break;
		case '"':
			continue;
		default:
			if ( i > sizeof( buf) - 2)
				break;
			buf[i++] = *p;
			continue;
		}
			break;
	}
	parse->text = p;
	buf[i] = 0;

	line = buf;
	mathOut.used = 0;
	if (!fxMathParse( parse, mathParseLine, &line, 0, &mathOut ))
		return qfalse;
	if ( out->used + mathOut.used > out->size ) 
		return fxParseError( parse, "Overflowing output buffer" );
	memcpy( out->data + out->used, mathOut.data, mathOut.used );
	out->used += mathOut.used;
	return qtrue;
}

static qboolean fxParseStackName( fxParse_t *parse, int *ofs, int *count, unsigned int *mask ) {

	const char *token = parse->token;
	if (!fxParseWord( parse , "stack name" ))
		return qfalse;

	if ( fxMatchFloat( token, ofs, mask )) {
		*count = 1;
		return qtrue;
	} else if ( fxMatchVector( token, ofs, mask )) {
		if ( *mask & FX_MASK_PARENT )
			return fxParseError( parse, "Can't access parent value %s", token );
		*count = 3;
		return qtrue;
	} else if (!Q_stricmp( token, "color")) {
		*ofs = runOfs( color );
		*count = runSize( color );
		*mask = FX_MASK_COLOR;
	} else if (!Q_stricmp( token, "model")) {
		*ofs = runOfs( model );
		*count = runSize( model );
		*mask = FX_MASK_MODEL;
	} else if (!Q_stricmp( token, "shader")) {
		*ofs = runOfs( shader );
		*count = runSize( shader );
		*mask = FX_MASK_SHADER;
	} else {
		return fxParseError( parse, "Unknown stack tag %s", token );
	}
	return qtrue;
}

static qboolean fxParseParentName( fxParse_t *parse, int *ofs, int *count ) {
	const char *token = parse->token;
	if (!fxParseWord( parse, "parentname" ))
		return qfalse;
	if (!Q_stricmp( token, "origin")) {
		*ofs = parentOfs( origin );
		*count = parentSize( origin );
	} else if (!Q_stricmp( token, "velocity")) {
		*ofs = parentOfs( velocity );
		*count = parentSize( velocity );
	} else if (!Q_stricmp( token, "dir")) {
		*ofs = parentOfs( dir );
		*count = parentSize( dir );
	} else if (!Q_stricmp( token, "angles")) {
		*ofs = parentOfs( angles );
		*count = parentSize( angles );
	} else if (!Q_stricmp( token, "shader")) {
		*ofs = parentOfs( shader );
		*count = parentSize( shader );
	} else if (!Q_stricmp( token, "model")) {
		*ofs = parentOfs( model );
		*count = parentSize( model );
	} else if (!Q_stricmp( token, "size")) {
		*ofs = parentOfs( size );
		*count = parentSize( size );
	} else if (!Q_stricmp( token, "width")) {
		*ofs = parentOfs( width );
		*count = parentSize( width );
	} else if (!Q_stricmp( token, "yaw")) {
		*ofs = parentOfs( angles[YAW] );
		*count = parentSize( angles[YAW] );
	} else if (!Q_stricmp( token, "pitch")) {
		*ofs = parentOfs( angles[PITCH] );
		*count = parentSize( angles[PITCH] );
	} else if (!Q_stricmp( token, "roll")) {
		*ofs = parentOfs( angles[ROLL] );
		*count = parentSize( angles[ROLL] );
	} else if (!Q_stricmp( token, "angle")) {
		*ofs = parentOfs( angles[ROLL] );
		*count = parentSize( angles[ROLL] );
	} else if (!Q_stricmp( token, "color")) {
		*ofs = parentOfs( color );
		*count = parentSize( color );
	} else if (!Q_stricmp( token, "color2")) {
		*ofs = parentOfs( color2 );
		*count = parentSize( color2 );
	} else {
		return fxParseError( parse, "Unknown parent tag %s", token );
	}
	return qtrue;
}

static qboolean fxParseColorMathOut( fxParse_t *parse, fxParseOut_t *out, unsigned int index ) {
	fxRunColorMath_t colorMath;
	colorMath.index = index;
	if (!fxParseOutCmd( parse, out, fxCmdColorMath, &colorMath, sizeof( colorMath )))
		return qfalse;
	if (!fxParseMath( parse, out ))
		return qfalse;
	fxParseWrite( parse, FX_MASK_COLOR );
	return qtrue;
}

static qboolean fxParseBlock( fxParse_t *parse, fxParseOut_t *out );

static qboolean fxParseIfBlock( fxParse_t *parse, fxParseOut_t *out ) {
	int *blockSize = (int *)(out->data + out->used);
	/* Store an int to contain the block size */
	if (!fxParseOut( parse, out, blockSize, sizeof( int )))
		return qfalse;
	blockSize[0] = out->used;
	if (!fxParseBlock( parse, out ))
		return qfalse;
	/* Store the blocksize in front of the block */
	blockSize[0] = out->used - blockSize[0];
	return qtrue;
}

/* This is started up with a parsed token alread in the parse */
static qboolean fxParseBlock( fxParse_t *parse, fxParseOut_t *out ) {
	byte temp[1024];
	const char *token = parse->token;

	if (!fxParseToken( parse, qtrue )) {
		return fxParseError( parse, "missing opening paranthesis" );
	}
	if (Q_stricmp( token, "{" )) 
		return fxParseError( parse, "no opening paranthesis, got %s", token );

	while ( 1 ) {
		if (!fxParseToken( parse, qtrue )) {
			return fxParseError( parse, "missing closing paranthesis" );
		}
		if ( parse->emitter ) {
			fxRunEmitter_t *emitter = parse->emitter;
			if (!Q_stricmp( token, "moveGravity" ) ) {
				if (!fxParseFloat( parse, &emitter->gravity ))
					return qfalse;
				emitter->flags |= FX_EMIT_MOVE;
				continue;
			} else if (!Q_stricmp( token, "moveBounce" ) ) {
				if (!fxParseFloat( parse, &emitter->gravity ))
					return qfalse;
				if (!fxParseFloat( parse, &emitter->bounceFactor ))
					return qfalse;
				emitter->flags |= FX_EMIT_MOVE | FX_EMIT_TRACE;
				continue;
			} else if (!Q_stricmp( token, "impactDeath" ) ) {
				if (!fxParseFloat( parse, &emitter->gravity ))
					return qfalse;
				emitter->flags |= FX_EMIT_MOVE | FX_EMIT_TRACE | FX_EMIT_IMPACT;
				continue;
			} else if (!Q_stricmp( token, "sink" ) ) {
				if (!fxParseFloat( parse, &emitter->sinkDelay ))
					return qfalse;
				if (!fxParseFloat( parse, &emitter->sinkDepth ))
					return qfalse;
				//Scale the sinkdepth up for easy multiplying later
				emitter->sinkDepth *= 1.0f / ( 1.0f - emitter->sinkDelay );
				emitter->flags |= FX_EMIT_MOVE | FX_EMIT_SINK;
				continue;
			} else if (!Q_stricmp( token, "impact" ) ) {
				fxParseOut_t impactOut;
				fxRunSkip_t *skip = (fxRunSkip_t *)temp;

				if (!fxParseFloat( parse, &emitter->impactSpeed ))
					return qfalse;

				emitter->impactSpeed *= emitter->impactSpeed;
				emitter->flags |= FX_EMIT_MOVE | FX_EMIT_TRACE;

				impactOut.data = temp;
				impactOut.size = sizeof( temp );
				impactOut.used = sizeof( fxRunSkip_t );
				if (!fxParseBlock( parse, &impactOut ))
					return qfalse;
				skip->size = impactOut.used;

				emitter->impactRun = out->used + sizeof( void *) + sizeof( fxRunSkip_t );
				if (!fxParseOutCmd( parse, out, fxCmdSkip, skip, skip->size ))
					return qfalse;
				continue;
			} else if (!Q_stricmp( token, "death" ) ) {
				fxParseOut_t deathOut;
				fxRunSkip_t *skip = (fxRunSkip_t *)temp;

				deathOut.data = temp;
				deathOut.size = sizeof( temp );
				deathOut.used = sizeof( fxRunSkip_t );
				if (!fxParseBlock( parse, &deathOut ))
					return qfalse;
				skip->size = deathOut.used;

				emitter->deathRun = out->used + sizeof( void *) + sizeof( fxRunSkip_t );
				if (!fxParseOutCmd( parse, out, fxCmdSkip, skip, skip->size ))
					return qfalse;
				continue;
			}
		}
		if (0) {
		} else if (!Q_stricmp( token, "shaderClear" ) ) {
			fxRunShader_t shader;
			shader.shader = 0;
			if (!fxParseOutCmd( parse, out, fxCmdShader, &shader, sizeof( shader )))
				return qfalse;
			fxParseWrite( parse, FX_MASK_SHADER );
		} else if (!Q_stricmp( token, "shader" ) ) {
			fxRunShader_t shader;
			if (!fxParseWord( parse, "shadername" ))
				return qfalse;
			if (!fxMatchShader( parse, &shader.shader ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdShader, &shader, sizeof( shader )))
				return qfalse;
			fxParseWrite( parse, FX_MASK_SHADER );
		} else if (!Q_stricmp( token, "model" ) ) {
			fxRunModel_t model;
			if (!fxParseWord( parse, "modelname" ))
				return qfalse;
			if (!fxMatchModel( parse, &model.model ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdModel, &model, sizeof( model )))
				return qfalse;
			fxParseWrite( parse, FX_MASK_MODEL );
		} else if (!Q_stricmp( token, "sound" ) ) {
			fxRunSound_t sound;
			if (!fxParseWord( parse, "soundname" ))
				return qfalse;
			if (!fxMatchSound( parse, &sound.handle ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSound, &sound, sizeof( sound )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN );
		} else if (!Q_stricmp( token, "shaderList" ) ) {
			fxRunShaderList_t *shaderList = (fxRunShaderList_t *)&temp;
			fxParseOut_t shaderListOut;

			shaderListOut.data = temp;
			shaderListOut.size = sizeof( temp );
			shaderListOut.used = sizeof( fxRunShaderList_t );
			if (!fxParseMath( parse, &shaderListOut ))
				return qfalse;
			if (!fxParseList( parse, "shaderList", fxMatchShader, sizeof (qhandle_t ),
					temp + shaderListOut.used,
					( shaderListOut.size - shaderListOut.used ) / sizeof (qhandle_t ),
					&shaderList->count )) 
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdShaderList, temp,  shaderListOut.used + shaderList->count * sizeof( qhandle_t ) ))
				return qfalse;
			fxParseWrite( parse, FX_MASK_SHADER );
		} else if (!Q_stricmp( token, "modelList" ) ) {
			fxRunModelList_t *modelList = (fxRunModelList_t *)&temp;
			fxParseOut_t modelListOut;

			modelListOut.data = temp;
			modelListOut.size = sizeof( temp );
			modelListOut.used = sizeof( fxRunModelList_t );
			if (!fxParseMath( parse, &modelListOut ))
				return qfalse;
			if (!fxParseList( parse, "modelList", fxMatchModel, sizeof (qhandle_t ),
					temp + modelListOut.used,
					( modelListOut.size - modelListOut.used ) / sizeof (qhandle_t ),
					&modelList->count )) 
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdModelList, temp,  modelListOut.used + modelList->count * sizeof( qhandle_t ) ))
				return qfalse;
			fxParseWrite( parse, FX_MASK_MODEL );
		} else if (!Q_stricmp( token, "colorList" ) ) {
			fxRunColorList_t *colorList = (fxRunColorList_t *)&temp;
			fxParseOut_t colorListOut;

			colorListOut.data = temp;
			colorListOut.size = sizeof( temp );
			colorListOut.used = sizeof( fxRunColorList_t );
			if (!fxParseMath( parse, &colorListOut ))
				return qfalse;
			if (!fxParseList( parse, "colorList", fxMatchColor, sizeof( color4ub_t ),
					temp + colorListOut.used,
					( colorListOut.size - colorListOut.used ) / sizeof( color4ub_t ),
					&colorList->count )) 
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdColorList, temp,  colorListOut.used + colorList->count * sizeof( color4ub_t ) ))
				return qfalse;
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "colorBlend" ) ) {
			fxRunColorBlend_t *colorBlend = (fxRunColorBlend_t *)&temp;
			fxParseOut_t colorBlendOut;

			colorBlendOut.data = temp;
			colorBlendOut.size = sizeof( temp );
			colorBlendOut.used = sizeof( fxRunColorBlend_t );
			if (!fxParseMath( parse, &colorBlendOut ))
				return qfalse;
			if (!fxParseList( parse, "colorBlend", fxMatchColor, sizeof( color4ub_t ),
					temp + colorBlendOut.used,
					( colorBlendOut.size - colorBlendOut.used ) / sizeof( color4ub_t ),
					&colorBlend->count )) 
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdColorBlend, temp,  colorBlendOut.used + colorBlend->count * sizeof( color4ub_t ) ))
				return qfalse;
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "soundList" ) ) {
			fxRunSoundList_t *soundList = (fxRunSoundList_t *)&temp;
			if (!fxParseList( parse, "SoundList", fxMatchSound, sizeof( sfxHandle_t ),
					soundList + 1,
					(sizeof(temp) - sizeof( *soundList)) / sizeof( sfxHandle_t ),
					&soundList->count )) 
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSoundList, temp,  sizeof(soundList) + soundList->count * sizeof( sfxHandle_t ) ))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN );
		} else if (!Q_stricmp( token, "loopSound" ) ) {
			fxRunLoopSound_t loopSound;
			if (!fxParseWord( parse, "Soundname"  )) {
				return qfalse;
			}
			loopSound.handle = S_RegisterSound( token, qfalse );
			if (!fxParseOutCmd( parse, out, fxCmdLoopSound, &loopSound, sizeof( loopSound )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_VELOCITY );
		} else if (!Q_stricmp( token, "Sprite" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSprite, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_ROTATE | FX_MASK_SHADER | FX_MASK_SIZE );
		} else if (!Q_stricmp( token, "Spark" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSpark, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_VELOCITY | FX_MASK_COLOR | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_WIDTH );
		} else if (!Q_stricmp( token, "Quad" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdQuad, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_ROTATE | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_DIR );
		} else if (!Q_stricmp( token, "Beam" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdBeam, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_ROTATE | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_DIR );
		} else if (!Q_stricmp( token, "Rings" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdRings, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_ROTATE | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_WIDTH | FX_MASK_DIR );
		} else if (!Q_stricmp( token, "anglesModel" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdAnglesModel, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_ANGLES | FX_MASK_MODEL );
		} else if (!Q_stricmp( token, "axisModel" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdAxisModel, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_AXIS | FX_MASK_MODEL );
		} else if (!Q_stricmp( token, "dirModel" ) ) {
			fxRunRender_t runRender;
			if (!fxParseRenderFX( parse, &runRender.renderfx ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdDirModel, &runRender, sizeof( runRender )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_SHADER | FX_MASK_SIZE | FX_MASK_DIR | FX_MASK_MODEL | FX_MASK_ROTATE );
		} else if (!Q_stricmp( token, "Light" ) ) {
			if (!fxParseOutCmd( parse, out, fxCmdLight, 0, 0 ))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_SIZE );
		} else if (!Q_stricmp( token, "Decal" ) ) {
			fxRunDecal_t decal;
			decal.flags = 0;
			decal.life = 10000;
		
			while (fxParseToken( parse, qfalse ) ) {
				if (!Q_stricmp(token, "temp"))  {
					decal.flags |= DECAL_TEMP;
				} else if (!Q_stricmp(token, "alpha"))  {
					decal.flags |= DECAL_ALPHA;
				} else if (!Q_stricmp(token, "energy"))  {
					decal.flags |= DECAL_ENERGY;
				//Last one has to be a float life
				} else {
					if (!fxMatchValidFloat( parse ))
						return qfalse;
					decal.life = atof( token ) * 1000;
				}
			}
			if ( decal.life <= 0 ) {
				 decal.life = 10000;
			}
			if (!fxParseOutCmd( parse, out, fxCmdDecal, &decal, sizeof( decal )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_COLOR | FX_MASK_SIZE | FX_MASK_DIR );
		} else if (!Q_stricmp( token, "Trace" ) ) {
			if (!fxParseOutCmd( parse, out, fxCmdTrace, 0, 0))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN | FX_MASK_DIR );
			fxParseWrite( parse, FX_MASK_ORIGIN | FX_MASK_DIR );
		} else if (!Q_stricmp( token, "vibrate" ) ) {
			fxRunVibrate_t vibrate;
			if (!fxParseFloat( parse, &vibrate.strength ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdVibrate, &vibrate, sizeof( vibrate )))
				return qfalse;
			fxParseRead( parse, FX_MASK_ORIGIN );
		} else if (!Q_stricmp( token, "alphaFade" ) ) {
			fxRunAlphaFade_t alphaFade;
			if (!fxParseFloat( parse, &alphaFade.delay ))
				return qfalse;
			if ( alphaFade.delay < 0 || alphaFade.delay >= 1.0f )
				return fxParseError( parse, "Fade value %f illegal", alphaFade.delay );
			alphaFade.scale = -255.0f / (1.0f - alphaFade.delay );
			if (!fxParseOutCmd( parse, out, fxCmdAlphaFade, &alphaFade, sizeof( alphaFade )))
				return qfalse;
			fxParseRead( parse, FX_MASK_COLOR );
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "color" ) ) {
			fxRunColor_t color;
			if (!fxParseColor( parse, color.value ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdColor, &color, sizeof( color )))
				return qfalse;
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "colorFade" ) ) {
			fxRunColorFade_t colorFade;
			if (!fxParseFloat( parse, &colorFade.delay ))
				return qfalse;
			if ( colorFade.delay < 0 || colorFade.delay >= 1.0f )
				return fxParseError( parse, "Fade value %f illegal", colorFade.delay );
			colorFade.scale = -255.0f / (1.0f - colorFade.delay );
			if (!fxParseOutCmd( parse, out, fxCmdColorFade, &colorFade, sizeof( colorFade )))
				return qfalse;
			fxParseRead( parse, FX_MASK_COLOR );
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "colorScale" ) ) {
			if (!fxParseOutCmd( parse, out, fxCmdColorScale, 0, 0 ))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
			fxParseRead( parse, FX_MASK_COLOR );
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "colorHue" ) ) {
			if (!fxParseOutCmd( parse, out, fxCmdColorHue, 0, 0))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
			fxParseWrite( parse, FX_MASK_COLOR );
		} else if (!Q_stricmp( token, "red" ) ) {
			if (!fxParseColorMathOut( parse, out, 0 ))
				return qfalse;
		} else if (!Q_stricmp( token, "green" ) ) {
			if (!fxParseColorMathOut( parse, out, 1 ))
				return qfalse;
		} else if (!Q_stricmp( token, "blue" ) ) {
			if (!fxParseColorMathOut( parse, out, 2 ))
				return qfalse;
		} else if (!Q_stricmp( token, "alpha" ) ) {
			if (!fxParseColorMathOut( parse, out, 3 ))
				return qfalse;
		} else if (!Q_stricmp( token, "wobble" ) ) {
			fxRunWobble_t wobble;

			if (!fxParseVectorRead( parse, &wobble.src ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &wobble.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdWobble, &wobble, sizeof( wobble )))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
		} else if (!Q_stricmp( token, "makeAngles" ) ) {
			fxRunMakeAngles_t makeAngles;

			if (!fxParseVectorRead( parse, &makeAngles.src ))
				return qfalse;
			makeAngles.dst = makeAngles.src;
			if (!fxParseVectorOptional( parse, &makeAngles.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdMakeAngles, &makeAngles, sizeof( makeAngles )))
				return qfalse;
		} else if (!Q_stricmp( token, "inverse" ) ) {
			fxRunInverse_t inverse;

			if (!fxParseVectorRead( parse, &inverse.src ))
				return qfalse;
			inverse.dst = inverse.src;
			fxParseVectorOptional( parse, &inverse.dst );
			if (!fxParseOutCmd( parse, out, fxCmdInverse, &inverse, sizeof( inverse )))
				return qfalse;
		} else if (!Q_stricmp( token, "normalize" ) ) {
			fxRunNormalize_t normalize;

			if (!fxParseVectorRead( parse, &normalize.src ))
				return qfalse;
			normalize.dst = normalize.src;
			fxParseVectorOptional( parse, &normalize.dst );
			if (!fxParseOutCmd( parse, out, fxCmdNormalize, &normalize, sizeof( normalize )))
				return qfalse;
		} else if (!Q_stricmp( token, "perpendicular" ) ) {
			fxRunPerpendicular_t perpendicular;

			if (!fxParseVectorRead( parse, &perpendicular.src ))
				return qfalse;
			perpendicular.dst = perpendicular.src;
			fxParseVectorOptional( parse, &perpendicular.dst );
			if (!fxParseOutCmd( parse, out, fxCmdPerpendicular, &perpendicular, sizeof( perpendicular )))
				return qfalse;
		} else if (!Q_stricmp( token, "clear" ) ) {
			fxRunClear_t clear;
			if (!fxParseVectorWrite( parse, &clear.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdClear, &clear, sizeof( clear )))
				return qfalse;
		} else if (!Q_stricmp( token, "copy" ) ) {
			fxRunCopy_t copy;
			if (!fxParseVectorRead( parse, &copy.src ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &copy.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdCopy, &copy, sizeof( copy )))
				return qfalse;
		} else if (!Q_stricmp( token, "scale" ) ) {
			fxRunScale_t scale;

			if (!fxParseVectorRead( parse, &scale.src ))
				return qfalse;
//			scale.dst = scale.src;
			if (!fxParseVectorWrite( parse, &scale.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdScale, &scale, sizeof( scale )))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
		} else if (!Q_stricmp( token, "add" ) ) {
			fxRunAdd_t add;

			if (!fxParseVectorRead( parse, &add.src1 ))
				return qfalse;
			if (!fxParseVectorRead( parse, &add.src2 ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &add.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdAdd, &add, sizeof( add )))
				return qfalse;
		} else if (!Q_stricmp( token, "sub" ) ) {
			fxRunSub_t sub;

			if (!fxParseVectorRead( parse, &sub.src1 ))
				return qfalse;
			if (!fxParseVectorRead( parse, &sub.src2 ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &sub.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSub, &sub, sizeof( sub )))
				return qfalse;
		} else if (!Q_stricmp( token, "addScale" ) ) {
			fxRunAddScale_t addScale;

			if (!fxParseVectorRead( parse, &addScale.src ))
				return qfalse;
			if (!fxParseVectorRead( parse, &addScale.scale ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &addScale.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdAddScale, &addScale, sizeof( addScale )))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
		} else if (!Q_stricmp( token, "subScale" ) ) {
			fxRunSubScale_t subScale;

			if (!fxParseVectorRead( parse, &subScale.src ))
				return qfalse;
			if (!fxParseVectorRead( parse, &subScale.scale ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &subScale.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdSubScale, &subScale, sizeof( subScale )))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
		} else if (!Q_stricmp( token, "rotateAround" ) ) {
			fxRunRotateAround_t rotateAround;

			if (!fxParseVectorRead( parse, &rotateAround.src ))
				return qfalse;
			if (!fxParseVectorRead( parse, &rotateAround.dir ))
				return qfalse;
			if (!fxParseVectorWrite( parse, &rotateAround.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdRotateAround, &rotateAround, sizeof( rotateAround )))
				return qfalse;
			if (!fxParseMath( parse, out ))
				return qfalse;
		} else if (!Q_stricmp( token, "random" ) ) {
			fxRunRandom_t random;
			if (!fxParseVectorWrite( parse, &random.dst ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdRandom, &random, sizeof( random )))
				return qfalse;
		} else if (!Q_stricmp( token, "kill" ) ) {
			if (!fxParseOutCmd( parse, out, fxCmdKill, 0, 0 ))
				return qfalse;
		} else if (!Q_stricmp( token, "emitter" ) || !Q_stricmp( token, "entity" )) {
			fxRunEmitter_t *oldEmitter;
			fxRunEmitter_t *newEmitter = (fxRunEmitter_t *)temp;
			fxParseOut_t emitterOut;
			unsigned oldReadMask, oldWriteMask, newReadMask;
			int oldLoopDepth;

			memset( newEmitter, 0, sizeof( fxRunEmitter_t ) );

			emitterOut.data = temp;
			emitterOut.size = sizeof( temp );
			emitterOut.used = sizeof( fxRunEmitter_t );
			if (!fxParseMath( parse, &emitterOut ))
				return qfalse;
			newEmitter->emitRun = emitterOut.used;
			oldEmitter = parse->emitter;
			oldReadMask = parse->readMask;
			oldWriteMask = parse->writeMask;
			oldLoopDepth = parse->loopDepth;

			parse->emitter = newEmitter;
			parse->loopDepth = 0;
			parse->readMask = 0;
			parse->writeMask = 0;
			if (!fxParseBlock( parse, &emitterOut ))
				return qfalse;
			//Create copy table and extra size requirements
			if (!fxParseCreateMask( parse, newEmitter->mask, sizeof( newEmitter->mask ), &newEmitter->allocSize ))
				return qfalse;
			newEmitter->allocSize += sizeof( fxEntity_t );
			newEmitter->size = emitterOut.used;
			//Emitters only read, restore the rest of the previous version
			newReadMask = parse->readMask;
			parse->readMask = oldReadMask;
			parse->writeMask = oldWriteMask;
			parse->loopDepth = oldLoopDepth;
			//forward readmask to caller
			fxParseRead( parse, newReadMask );
			parse->emitter = oldEmitter;
			if (!fxParseOutCmd( parse, out, fxCmdEmitter, newEmitter, emitterOut.used ))
				return qfalse;

		} else if (!Q_stricmp( token, "interval" ) ) {
			fxRunInterval_t *Interval = (fxRunInterval_t *)temp;
			fxParseOut_t intervalOut;

			parse->loopDepth++;
			intervalOut.data = temp;
			intervalOut.size = sizeof( temp );
			intervalOut.used = sizeof( fxRunInterval_t );
			if (!fxParseMath( parse, &intervalOut ))
				return qfalse;
			if (!fxParseBlock( parse, &intervalOut ))
				return qfalse;
			Interval->size = intervalOut.used;
			if (!fxParseOutCmd( parse, out, fxCmdInterval, Interval, intervalOut.used ))
				return qfalse;
			parse->loopDepth--;

		} else if (!Q_stricmp( token, "repeat" ) ) {
			fxRunRepeat_t *repeat = (fxRunRepeat_t *)temp;
			fxParseOut_t repeatOut;

			repeatOut.data = temp;
			repeatOut.size = sizeof( temp );
			repeatOut.used = sizeof( fxRunRepeat_t );
			parse->loopDepth++;
			if (!fxParseMath( parse, &repeatOut ))
				return qfalse;
			if (!fxParseBlock( parse, &repeatOut ))
				return qfalse;
			repeat->size = repeatOut.used;
			if (!fxParseOutCmd( parse, out, fxCmdRepeat, repeat, repeatOut.used ))
				return qfalse;
			parse->loopDepth--;
		} else if (!Q_stricmp( token, "distance" ) ) {
			fxRunDistance_t *distance = (fxRunDistance_t *)temp;
			fxParseOut_t distanceOut;

			parse->loopDepth++;
			distanceOut.data = temp;
			distanceOut.size = sizeof( temp );
			distanceOut.used = sizeof( fxRunDistance_t );
			if (!fxParseMath( parse, &distanceOut ))
				return qfalse;
			if (!fxParseBlock( parse, &distanceOut ))
				return qfalse;
			distance->size = distanceOut.used;
			if (!fxParseOutCmd( parse, out, fxCmdDistance, distance, distanceOut.used ))
				return qfalse;
			parse->loopDepth--;
			fxParseRead( parse, FX_MASK_ORIGIN );
		} else if (!Q_stricmp( token, "if" ) ) {
			fxRunIf_t *doIf = (fxRunIf_t *)temp;
			fxParseOut_t ifOut;

			doIf->testCount = 1;
			doIf->elseStep = 0;
			ifOut.data = temp;
			ifOut.size = sizeof( temp );
			ifOut.used = sizeof( fxRunIf_t );
			if (!fxParseMath( parse, &ifOut ))
				return qfalse;
			if (!fxParseIfBlock( parse, &ifOut ))
				return qfalse;
			while (1) {
				if (!fxParseToken( parse, qfalse ))
					break;
				if (!Q_stricmp( token, "elif")) {
					doIf->testCount += 1;
					if (!fxParseMath( parse, &ifOut ))
						return qfalse;
					if (!fxParseIfBlock( parse, &ifOut ))
						return qfalse;
				} else if (!Q_stricmp( token, "else" )) {
					doIf->elseStep = qtrue;
					if (!fxParseBlock( parse, &ifOut ))
						return qfalse;
					break;
				}
			}
			/* Calculate the total size for the final exit */
			doIf->size = ifOut.used;
			if (!fxParseOutCmd( parse, out, fxCmdIf, doIf, ifOut.used ))
				return qfalse;
		} else if (!Q_stricmp( token, "once" ) ) {
			fxRunOnce_t *once = (fxRunOnce_t *)temp;
			fxParseOut_t onceOut;

			onceOut.data = temp;
			onceOut.size = sizeof( temp );
			onceOut.used = sizeof( fxRunOnce_t );
			if (!fxParseBlock( parse, &onceOut ))
				return qfalse;
			once->size = onceOut.used;
			if (!fxParseOutCmd( parse, out, fxCmdOnce, once, onceOut.used ))
				return qfalse;
		} else if (!Q_stricmp( token, "pushParent" ) ) {
			fxRunPushParent_t push;
			if (!fxParseParentName( parse, &push.offset, &push.count ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdPushParent, &push, sizeof( push )))
				return qfalse;
			fxParseRead( parse, FX_MASK_PARENT );
		} else if (!Q_stricmp( token, "push" ) ) {
			int mask;
			fxRunPush_t push;
			if (!fxParseStackName( parse, &push.offset, &push.count, &mask ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdPush, &push, sizeof( push )))
				return qfalse;
			fxParseRead( parse, mask );
		} else if (!Q_stricmp( token, "pop" ) ) {
			int mask;
			fxRunPop_t pop;

			if (!fxParseStackName( parse, &pop.offset, &pop.count, &mask ))
				return qfalse;
			if (!fxParseOutCmd( parse, out, fxCmdPop, &pop, sizeof( pop )))
				return qfalse;
			fxParseWrite( parse, mask );
		} else if (!Q_stricmp( token, "script" ) ) {
			fxRunScript_t script;
			fxScript_t	 *loadScript;

			if (!fxParseWord( parse, "scriptname" ))
				return qfalse;
			loadScript = fxFindScript( token );
			if (!loadScript )
				return fxParseError( parse, "Can't load script %s", token );
			script.data = loadScript->data;
			fxParseRead( parse, loadScript->readMask );
			if (!fxParseOutCmd( parse, out, fxCmdScript, &script, sizeof( script )))
				return qfalse;
		} else if (!Q_stricmp( token, "}" ) ) {
			if (!fxParseOutCmd( parse, out, 0, 0, 0 ))
				return qfalse;
			return qtrue;
		} else {
			unsigned int mask;
			fxRunValue_t value;
			if (fxMatchFloat( token, &value.dst, &mask)) {
				if (!fxParseOutCmd( parse, out, fxCmdValue, &value, sizeof( value ) ))
					return qfalse;
				if (!fxParseMath( parse, out ))
					return qfalse;
				fxParseWrite( parse, mask );
			} else {
				return fxParseError( parse, "Unknown token %s", parse->token );
			}
		}
	}
	return qfalse;
}

static fxScript_t *fxParseScript( fxHashEntry_t *entry ) {
	fxParse_t parse;
	fxParseOut_t parseOut;
	fxScript_t *script;
	char parseData[ 32*1024];
	byte *allocBuf;
	int allocSize;

	/* Check if this one has already been loaded */
	if ( entry->script )
		return entry->script;

	if ( fx.scriptCount >= FX_MAX_SCRIPTS ) {
		Com_Printf( "fx script limit reached" );
		return 0;
	}

	/* Prepare variables for parsing the text */
	memset( &parse, 0, sizeof( parse ));
	parse.text = entry->text;
	parse.name = entry->name;

	parseOut.data = parseData;
	parseOut.used = 0;
	parseOut.size = sizeof( parseData );

	if (!fxParseBlock( &parse, &parseOut ))
		return 0;

	/* Allocate the script struct and set it up */
	allocSize = parseOut.used + sizeof( fxScript_t );
	allocBuf = fxParseAlloc( allocSize );
	script = (fxScript_t *) allocBuf;
	script->remap = 0;
	script->readMask = parse.readMask;
	Com_Memcpy( &script->data, parseOut.data, parseOut.used );
	entry->script = script;
	script->entry = entry;
	/* Register script in table an increase scriptcount */
	fx.scripts[ fx.scriptCount ] = script;
	script->handle = fx.scriptCount;
	fx.scriptCount++;
	return script;
}


#define SCAN_ENTRIES 256
typedef struct fxScanBlock_s {
	fxHashEntry_t entries[SCAN_ENTRIES];
	int entriesUsed;
	struct fxScanBlock_s *next;
} fxScanBlock_t;

static char *scanSkipSpaces( char *p ) {
	for (;*p;p++) 
		if ((*p != ' ') && (*p != '\t') && (*p != '\n')) 
			break;
	return p;
}
/* This will modify the original string */
static char *scanMakeWord( char **input ) {
	char *ret;
	char *p = *input;

	p = scanSkipSpaces( p );
	ret = p;
	for (;*p;p++) {
		if (*p == ' ' || *p == '\t' || *p == '\n') {
			*p++ = 0;		
			break;
		}
	}
	*input = p;
	return ret;
}

static char *scanMakeSection( char **input ) {
	char *ret;
	char *p = *input;
	int braceDepth = 1;

	p = scanSkipSpaces( p );
	if ( p[0] != '{')
		return 0;
	ret = p;
	p++;
	for (;*p;p++) {
		if ( p[0] == '{') {
			braceDepth++; 
		} else if ( p[0] == '}') {
			braceDepth--;
			if (!braceDepth) {
				if (p[1]) {
					p[1] = 0;
					p+=2;
				} else {
					p += 1;
				}
				break;
			}
		}
	}
	if ( braceDepth )
		return 0;
	*input = p;
	return ret;
}

static int scanCopyString( char *dst, const char *src, int left ) {
	int done = 0;
	if ( left <= 0)
		return 0;
	while (left > 1 && src[done]) {
		dst[done] = src[done];
		done++;
		left--;
	}
	dst[done] = 0;
	done++;
	return done;
}

static void fxScanDir( const char *dirName, void **buffers, int *numFiles) {
	int i, dirFiles;
	char **fxFiles;

	fxFiles = FS_ListFiles( "scripts", ".fx", &dirFiles );

	// load the files first time
	for ( i = 0; i < dirFiles; i++ ) {
		char filename[MAX_QPATH];
		if (*numFiles >= FX_MAX_SCRIPTS) {
			Com_Error( ERR_DROP, "Too many .fx files" );
		}
		Com_sprintf( filename, sizeof( filename ), "%s/%s", dirName, fxFiles[i] );
		FS_ReadFile( filename, &buffers[numFiles[0]] );
		numFiles[0]++;
	}
	FS_FreeFileList( fxFiles );
}

static void *emptyScript[2];
static void fxScanFiles( void ) {
	char *buffers[FX_MAX_SCRIPTS];
	int numFiles;
	int allocSize;
	int i;
	fxHashEntry_t *entryWrite;
	int entrySize, nameSize, textSize;
	char *allocBuf, *nameWrite, *textWrite;
	fxScanBlock_t *scanBlock = 0;
	fxHashEntry_t *tempHash[FX_HASH_SIZE];

	/* Clear some stuff just to be safe */
	fx.alloc.entries = 0;
	memset( fx.scripts, 0, sizeof( fx.scripts ));
	memset( fx.entryHash, 0, sizeof( fx.entryHash ));
	/* Init the default handle for an invalid/empty script*/
	fx.scripts[0] = (fxScript_t *)emptyScript;
	fx.scriptCount = 1;
	// scan for FX files
	Com_Memset( tempHash, 0, sizeof( tempHash ));
	numFiles = 0;
	fxScanDir( "scripts", buffers, &numFiles );
	if ( fx_Override->string[0] ) {
		int i;
		Cmd_TokenizeString( fx_Override->string );
		for( i=0; i<Cmd_Argc();i++) {
			const char *newDir = Cmd_Argv( i );
			fxScanDir( newDir, buffers, &numFiles );
		}
	}
	/* Scan the file and scan the braced scripts */
	for ( i = 0; i < numFiles; i++ )
	{
		char *p = buffers[i];
		COM_Compress( p );
		while (1) {
			char *name = scanMakeWord( &p );
			char *section = scanMakeSection( &p );
			unsigned int hashIndex;
			fxHashEntry_t *entry;
			if (!name || !name[0])
				break;
			if ( !section || !section[0] ) {
				Com_Printf( "Error reading section for %s\n", name);
				break;
			}
			/* Scan the hash for a name we've used before */
			hashIndex = fxGenerateHash( name );
			for ( entry = tempHash[hashIndex]; entry; entry = entry->next ) {
				if ( Q_stricmp( entry->name, name ))
					continue;
				/* Override the current entry */
				entry->name = name;
				entry->text = section;
				break;
			}
			/* Didn't find an entry, add a new one */
			if (!entry) {
				/* Get an entry from the block */
				if ( !scanBlock || scanBlock->entriesUsed == SCAN_ENTRIES) {
					fxScanBlock_t *newBlock = Hunk_AllocateTempMemory( sizeof( fxScanBlock_t ));
					newBlock->entriesUsed = 0;
					newBlock->next = scanBlock;
					scanBlock = newBlock;
				}
				entry = &scanBlock->entries[scanBlock->entriesUsed++];
				entry->name = name;
				entry->text = section;
				entry->next = tempHash[hashIndex];
				tempHash[hashIndex] = entry;
			}
		}
	}
	/* Scan through our hash and determine allocation sizes */
	nameSize = 0; textSize = 0; entrySize = 0;
	allocSize = 0;
	for ( i = 0; i < FX_HASH_SIZE; i++) {
		fxHashEntry_t *entry;
		for (entry = tempHash[i]; entry; entry = entry->next ) {
			entrySize += sizeof( fxHashEntry_t );
			nameSize += strlen( entry->name ) + 1;
			textSize += strlen( entry->text ) + 1;
		}
	}
	/* List of entries in front, then all the names, then all the text */
	allocBuf = (char*) fxParseAlloc( entrySize + nameSize + textSize );
	fx.alloc.entries = (fxHashEntry_t*)allocBuf;
	entryWrite = fx.alloc.entries;
	nameWrite = allocBuf + entrySize;
	textWrite = allocBuf + entrySize + nameSize;
	/* Silly overkill to ensure that names are read linearly, oh well */
	for ( i = 0; i < FX_HASH_SIZE; i++) {
		fxHashEntry_t *entry, *newEntry, *startEntry;
		newEntry = 0;
		startEntry = entryWrite;
		for (entry = tempHash[i]; entry; entry = entry->next ) {
			int len;
			newEntry = entryWrite++;

			memset( newEntry, 0, sizeof (*newEntry));
			newEntry->next = entryWrite;
			
			newEntry->name = nameWrite;
			len = scanCopyString( nameWrite, entry->name, nameSize );
			nameSize -= len; nameWrite += len;

			newEntry->text = textWrite;
			len = scanCopyString( textWrite, entry->text, textSize );
			textSize -= len; textWrite += len;
		}
		if ( newEntry ) {
			fx.entryHash[i] = startEntry;
			newEntry->next = 0;
		}
	}
	/* Free all allocated memory in the good order hopefully */
	while (scanBlock) {
		fxScanBlock_t *next = scanBlock->next;
		Hunk_FreeTempMemory( scanBlock );
		scanBlock = next;
	}
	for ( i = numFiles-1; i >=0; i-- )
		FS_FreeFile( buffers[i] );
}

static fxScript_t *fxFindScript( const char *name ) {
	unsigned int hashIndex;
	fxHashEntry_t *entry;
	if (!name)
		return 0;

	hashIndex = fxGenerateHash( name );
	for ( entry = fx.entryHash[ hashIndex ]; entry; entry = entry->next ) {
		if (!Q_stricmp( entry->name, name )) {
			return fxParseScript( entry );
		}
	}
	return 0;
}

fxHandle_t FX_Register( const char *name ) {
	const fxScript_t *script;
	script = fxFindScript( name );
	if ( fx_Debug->integer ) {
		Com_Printf( "FX_Debug:Registering script %s, %s\n", name, script ? "success" : "failed" );
	}
	if ( script )
		return script->handle;
	return 0;
}

static void fxRemap( void ) {
	const char *map1, *map2;
	fxScript_t *script1, *script2;
	
	if ( Cmd_Argc() < 1 ) {
		Com_Printf("fxRemap oldScript newScript, will replace oldscript with new one\n" );
		Com_Printf("fxRemap oldScript, will reset the remap\n" );
		return;
	}
	map1 = Cmd_Argv( 1 );
	script1 = fxFindScript( map1 );
	if (!script1) {
		Com_Printf( "fxScript %s not found\n", map1 );
		return;
	}
	if ( Cmd_Argc() > 2) {
		map2 = Cmd_Argv( 2 );
		script2 = fxFindScript( map2 );
		if (!script2) {
			Com_Printf( "fxScript %s not found\n", map2 );
			return;
		}
		script1->remap = script2;
		Com_Printf("Remapped %s to %s", script1->entry->name, script2->entry->name );
	} else {
		Com_Printf("Cleared remapping on script %s", script1->entry->name );
		script1->remap = 0;
	}
}

static void fxReload( void ) {
	fxScript_t *scripts[FX_MAX_SCRIPTS];
	fxHashEntry_t *entries = fx.alloc.entries;
	fxHashEntry_t *entryHash[FX_HASH_SIZE];
	int scriptCount;
	int i;

	FX_Reset();

	scriptCount = fx.scriptCount;
	for ( i = 1; i < scriptCount;i++ ) {
		scripts[i] = fx.scripts[i];
		fx.scripts[i] = 0;
	}
	for ( i = 0; i < FX_HASH_SIZE;i++ ) {
		entryHash[i] = fx.entryHash[i];
		fx.entryHash[i] = 0;
	}
	fx.alloc.entries = 0;
	/* Rescan all the files */
	fxScanFiles();
	/* Reload all the scripts in the same order */
	for (i = 1; i < scriptCount;i++ ) {
		int c;
		const char *name = scripts[i]->entry->name;
		fxScript_t *load = fxFindScript( name );
		if ( load && load->handle == i )
			continue;
		if (!load)
			Com_Printf( "Failed to reload scripts %s\n", name );
		else
			Com_Printf( "Script %s doesn't reload in the right handle\n", name );

		/* Restore the old stuff and free the new stuff */
		for ( c = 1; c < fx.scriptCount; c++ )
			fxParseFree( fx.scripts[c] );
		fxParseFree( fx.alloc.entries );
		fx.scriptCount = scriptCount;
		for ( c = 1; c < scriptCount; c++ )
			fx.scripts[c] = scripts[c];
		for ( c = 0; c< FX_HASH_SIZE; c++ )
			fx.entryHash[c] = entryHash[c];
		fx.alloc.entries = entries;
		return;
	}
	/* Free the old stuff */
	for ( i = 1; i < scriptCount; i++ )
		fxParseFree( scripts[i] );
	fxParseFree( entries );
}


static void fxMathTest( void ) {
	char line[1024];
	char *lineP = line;
	fxMathOut_t mathOut;
	fxParse_t parse;
	fxRun_t	run;

	memset( &parse, 0, sizeof( parse ));
	memset( &mathOut, 0, sizeof( mathOut ));
	memset( &run, 0, sizeof( run ));

	Q_strncpyz( line, Cmd_ArgsFrom( 1 ), sizeof( line ) );
	if ( fxMathParse( &parse, mathParseLine, &lineP, 0, &mathOut ) ) {
		fxMathInfo_t info;
		info.data = (int *)mathOut.data;
		info.run = &run;
		Com_Printf( "result %f\n", fxMath( &info) );
	}
}

void fxFreeMemory( void ) {
	int i;
	for ( i = 1; i < fx.scriptCount; i++ ) {
		fxParseFree( fx.scripts[i] );
		fx.scripts[i] = 0;
	}
	fx.scriptCount = 0;
	if ( fx.alloc.entries )
		fxParseFree( fx.alloc.entries );
	fx.alloc.entries = 0;
}

void fxInitParser( void ) {
	fxScanFiles();
	Cmd_AddCommand( "fxReload", fxReload );
	Cmd_AddCommand( "fxRemap", fxRemap );
	Cmd_AddCommand( "fxMath", fxMathTest );
}

