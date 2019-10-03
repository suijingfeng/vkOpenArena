#include "sys_public.h"

static int CPU_Flags;
#define CPU_FCOM   0x01
#define CPU_MMX    0x02
#define CPU_SSE    0x04
#define CPU_SSE2   0x08
#define CPU_SSE3   0x10
#define CPU_SSE41  0x20


/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/
#if defined _MSC_VER
static void CPUID( int func, unsigned int *regs )
{
#if _MSC_VER >= 1400
	__cpuid( regs, func );
#else
	__asm {
		mov edi,regs
		mov eax,[edi]
		cpuid
		mov [edi], eax
		mov [edi+4], ebx
		mov [edi+8], ecx
		mov [edi+12], edx
	}
#endif
}
#else
static void CPUID( int func, unsigned int *regs )
{
	__asm__ __volatile__( "cpuid" :
		"=a"(regs[0]),
		"=b"(regs[1]),
		"=c"(regs[2]),
		"=d"(regs[3]) :
		"a"(func) );
}
#endif


int Sys_GetProcessorId( char *vendor )
{
	unsigned int regs[4];
	// setup initial features
#if idx64
	CPU_Flags |= CPU_SSE | CPU_SSE2 | CPU_FCOM;
#else
	CPU_Flags = 0;
#endif
	// get CPU feature bits
	CPUID( 1, regs );
	// bit 15 of EDX denotes CMOV/FCMOV/FCOMI existence
	if ( regs[3] & ( 1 << 15 ) )
		CPU_Flags |= CPU_FCOM;
	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		CPU_Flags |= CPU_MMX;
	// bit 25 of EDX denotes SSE existence
	if ( regs[3] & ( 1 << 25 ) )
		CPU_Flags |= CPU_SSE;
	// bit 26 of EDX denotes SSE2 existence
	if ( regs[3] & ( 1 << 26 ) )
		CPU_Flags |= CPU_SSE2;
	// bit 0 of ECX denotes SSE3 existence
	if ( regs[2] & ( 1 << 0 ) )
		CPU_Flags |= CPU_SSE3;

	// bit 19 of ECX denotes SSE41 existence
	if ( regs[ 2 ] & ( 1 << 19 ) )
		CPU_Flags |= CPU_SSE41;

	if ( vendor )
	{
#if idx64
		strcpy( vendor, "64-bit " );
		vendor += strlen( vendor );
#else
		vendor[0] = '\0';
#endif
		// get CPU vendor string
		CPUID( 0, regs );
		memcpy( vendor+0, (char*) &regs[1], 4 );
		memcpy( vendor+4, (char*) &regs[3], 4 );
		memcpy( vendor+8, (char*) &regs[2], 4 );
		vendor[12] = '\0'; vendor += 12;
		if ( CPU_Flags ) {
			// print features
#if !idx64	// do not print default 64-bit features in 32-bit mode
			strcat( vendor, " w/" );
			if ( CPU_Flags & CPU_FCOM )
				strcat( vendor, " CMOV" );
			if ( CPU_Flags & CPU_MMX )
				strcat( vendor, " MMX" );
			if ( CPU_Flags & CPU_SSE )
				strcat( vendor, " SSE" );
			if ( CPU_Flags & CPU_SSE2 )
				strcat( vendor, " SSE2" );
#endif
			//if ( CPU_Flags & CPU_SSE3 )
			//	strcat( vendor, " SSE3" );
			if ( CPU_Flags & CPU_SSE41 )
				strcat( vendor, " SSE4.1" );
		}
	}
	return 1;
}
