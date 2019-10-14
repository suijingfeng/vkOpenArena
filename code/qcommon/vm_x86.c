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
// vm_x86.c -- load time compiler and execution environment for x86


#include "vm_local.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #ifdef __FreeBSD__
  #include <sys/types.h>
  #endif

  #include <sys/mman.h> // for PROT_ stuff

  /* need this on NX enabled systems (i386 with PAE kernel or noexec32=on x86_64) */
  #define VM_X86_MMAP
  
  // workaround for systems that use the old MAP_ANON macro
	#ifndef MAP_ANONYMOUS
		#define MAP_ANONYMOUS MAP_ANON
	#endif
#endif



#if defined(_MSC_VER)

#if (idx64) 
extern uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, unsigned char *dataBase);
#endif
// work around with MSVC compiler, suijingfeng

extern int qvmftolsse( void );

int(*Q_VMftol)(void) = qvmftolsse;

#else
// GCC
#if idx64
#define EAX "%%rax"
#define EBX "%%rbx"
#define ESP "%%rsp"
#define EDI "%%rdi"
#else
#define EAX "%%eax"
#define EBX "%%ebx"
#define ESP "%%esp"
#define EDI "%%edi"
#endif

static int qftolsse_linux(void)
{
	int retval;

	__asm__ volatile
		(
			"movss (" EDI ", " EBX ", 4), %%xmm0\n"
			"cvttss2si %%xmm0, %0\n"
			: "=r" (retval)
			:
			: "%xmm0"
			);

	return retval;
}

int( * Q_VMftol )(void) = qftolsse_linux;

#endif


static void VM_Destroy_Compiled(vm_t* self);

/*
  eax		scratch
  ebx/bl	opStack offset
  ecx		scratch (required for shifts)
  edx		scratch (required for divisions)
  esi		program stack
  edi   	opStack base
x86_64:
  r8		vm->instructionPointers
  r9		vm->dataBase
*/



static int pc = 0;
static int compiledOfs = 0;

#define FTOL_PTR

static	int	instruction;
static	int	lastConst = 0;
static	int	oc0, oc1, pop0, pop1;
static	int jlabel;

typedef enum 
{
	LAST_COMMAND_NONE	= 0,
	LAST_COMMAND_MOV_STACK_EAX,
	LAST_COMMAND_SUB_BL_1,
	LAST_COMMAND_SUB_BL_2,
} ELastCommand;

typedef enum
{
	VM_JMP_VIOLATION = 0,
	VM_BLOCK_COPY = 1
} ESysCallType;

static ELastCommand	LastCommand;

static int iss8(int32_t v)
{
	return ( (v >= SCHAR_MIN) && (v <= SCHAR_MAX) );
}


inline static int Constant4( unsigned char * code)
{
	int	v = (code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24));
	pc += 4;
	return v;
}



inline static void Emit1( unsigned char* buf, int v )
{
	buf[ compiledOfs++ ] = v;
	LastCommand = LAST_COMMAND_NONE;
}

inline static void Emit2( unsigned char* buf, int v )
{
    buf[ compiledOfs++ ] = v & 0xFF;
    buf[ compiledOfs++ ] = (v >> 8) & 0xFF;
	LastCommand = LAST_COMMAND_NONE;
}


inline static void Emit4( unsigned char* buf, int v )
{
    buf[ compiledOfs++ ] = v & 0xFF;
    buf[ compiledOfs++ ] = (v >> 8) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 16) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 24) & 0xFF;
	LastCommand = LAST_COMMAND_NONE;
}

inline static void EmitPtr( unsigned char* buf, void *ptr )
{
	intptr_t v = (intptr_t) ptr;
	
    buf[ compiledOfs++ ] = v & 0xFF;
    buf[ compiledOfs++ ] = (v >> 8) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 16) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 24) & 0xFF;

#if idx64
    buf[ compiledOfs++ ] = (v >> 32) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 40) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 48) & 0xFF;
    buf[ compiledOfs++ ] = (v >> 56) & 0xFF;
#endif

    LastCommand = LAST_COMMAND_NONE;
}


static int ch2Hex( unsigned char c )
{
	if( (c >= '0') && (c <= '9') )
    {
		return c - '0';
	}
	if( (c >= 'A') && (c <= 'F') )
    {
		return 10 + c - 'A';
	}
	if( (c >= 'a') && (c <= 'f') )
    {
		return 10 + c - 'a';
	}
	return 0;
}

static void Hes2char(unsigned char hex, unsigned char code[2])
{
	unsigned char c1 = (hex >> 4);
	unsigned char c2 = (hex & 0x0f);

	if ((c1 >= 0) && (c1 <= 9))
	{
		code[0] = c1 - 0 + '0';
	}
	else if ((c1 >= 10) && (c1 <= 15))
	{
		code[0] = c1 - 10 + 'A';
	}
	
	if ((c2 >= 0) && (c2 <= 9))
	{
		code[1] = c2 - 0 + '0';
	}
	else if ((c2 >= 10) && (c2 <= 15))
	{
		code[1] = c2 - 10 + 'A';
	}
}

inline static void EmitString(unsigned char* buf, const char *string)
{

	while ( 1 )
    {
		buf[ compiledOfs++ ] = (ch2Hex(string[0]) << 4) | ch2Hex(string[1]);

		if ( 0 == string[2] )
			break;
        
		string += 3;
	}

    LastCommand = LAST_COMMAND_NONE;
}



inline static void EmitRexString(unsigned char* buf, unsigned char rex, const char *string)
{
#if idx64
	if(rex)
    	buf[ compiledOfs++ ] = rex;
#endif

	while ( 1 )
    {
		buf[ compiledOfs++ ] = (ch2Hex(string[0]) << 4) | ch2Hex(string[1]);

		if ( 0 == string[2] )
			break;
        
		string += 3;
	}

    LastCommand = LAST_COMMAND_NONE;
}


static void EmitMaskReg( unsigned char * buf, unsigned char * modrm, int mask)
{ 
	EmitString( buf, "81" );
	EmitString( buf, modrm );
	Emit4( buf, mask );
}


inline static void EmitCommand(unsigned char* buf, ELastCommand command)
{
	switch(command)
	{
		case LAST_COMMAND_MOV_STACK_EAX:
			// EmitString("89 04 9F");		// mov dword ptr [edi + ebx * 4], eax
            
            buf[ compiledOfs++ ] = 0x89;
            buf[ compiledOfs++ ] = 0x04;
            buf[ compiledOfs++ ] = 0x9F;
			break;

		case LAST_COMMAND_SUB_BL_1:
			//EmitString("80 EB");
		    buf[ compiledOfs++ ] = 0x80;
            buf[ compiledOfs++ ] = 0xEB;
            //Emit1(1);			// sub bl, 1
            buf[ compiledOfs++ ] = 0x01;
			break;

		case LAST_COMMAND_SUB_BL_2:
			//EmitString("80 EB");
		    buf[ compiledOfs++ ] = 0x80;
            buf[ compiledOfs++ ] = 0xEB;
            //Emit1(2);			// sub bl, 2
            buf[ compiledOfs++ ] = 0x02;
			break;
		default:
			break;
	}
    LastCommand = command;
}

static void EmitPushStack(unsigned char* buf, vm_t *vm)
{
	if (!jlabel)
	{
		if(LastCommand == LAST_COMMAND_SUB_BL_1)
		{	// sub bl, 1
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			return;
		}
		if(LastCommand == LAST_COMMAND_SUB_BL_2)
		{	// sub bl, 2
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			//EmitString("80 EB");
		    buf[ compiledOfs++ ] = 0x80;
            buf[ compiledOfs++ ] = 0xEB;
            //Emit1(1);			// sub bl, 1
            buf[ compiledOfs++ ] = 0x01;
	        LastCommand = LAST_COMMAND_NONE;
			return;
		}
	}

	// add bl, 1
    //EmitString("80 C3");
	//Emit1(1);

	buf[ compiledOfs++ ] = 0x80;
    buf[ compiledOfs++ ] = 0xC3;
    buf[ compiledOfs++ ] = 0x01;
	LastCommand = LAST_COMMAND_NONE;
}

static void EmitMovEAXStack(unsigned char* buf, vm_t *vm, int andit)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX) 
		{	// mov [edi + ebx * 4], eax
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
		}
		else if(pop1 == OP_CONST && buf[compiledOfs-7] == 0xC7 && buf[compiledOfs-6] == 0x04 && buf[compiledOfs - 5] == 0x9F)
		{	// mov [edi + ebx * 4], 0x12345678
			compiledOfs -= 7;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString(buf, "B8");	// mov	eax, 0x12345678

			if(andit)
				Emit4(buf, lastConst & andit);
			else
				Emit4(buf, lastConst);
			
			return;
		}
		else if(pop1 != OP_DIVI && pop1 != OP_DIVU && pop1 != OP_MULI && pop1 != OP_MULU &&
			pop1 != OP_STORE4 && pop1 != OP_STORE2 && pop1 != OP_STORE1)
		{	
			EmitString(buf, "8B 04 9F");	// mov eax, dword ptr [edi + ebx * 4]
		}
	}
	else
		EmitString(buf, "8B 04 9F");		// mov eax, dword ptr [edi + ebx * 4]

	if(andit)
	{
		EmitString(buf, "25");		// and eax, 0x12345678
		Emit4(buf, andit);
	}
}

void EmitMovECXStack(unsigned char* buf, vm_t *vm)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX) // mov [edi + ebx * 4], eax
		{
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString(buf, "89 C1");		// mov ecx, eax
			return;
		}
		if(pop1 == OP_DIVI || pop1 == OP_DIVU || pop1 == OP_MULI || pop1 == OP_MULU ||
			pop1 == OP_STORE4 || pop1 == OP_STORE2 || pop1 == OP_STORE1) 
		{	
			EmitString(buf, "89 C1");		// mov ecx, eax
			return;
		}
	}

	EmitString(buf, "8B 0C 9F");		// mov ecx, dword ptr [edi + ebx * 4]
}


void EmitMovEDXStack(unsigned char* buf, vm_t *vm, int andit)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX)
		{	// mov dword ptr [edi + ebx * 4], eax
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;

			EmitString(buf, "8B D0");	// mov edx, eax
		}
		else if(pop1 == OP_DIVI || pop1 == OP_DIVU || pop1 == OP_MULI || pop1 == OP_MULU ||
			pop1 == OP_STORE4 || pop1 == OP_STORE2 || pop1 == OP_STORE1)
		{	
			EmitString(buf, "8B D0");	// mov edx, eax
		}
		else if(pop1 == OP_CONST && buf[compiledOfs-7] == 0xC7 && buf[compiledOfs-6] == 0x07 && buf[compiledOfs - 5] == 0x9F)
		{	// mov dword ptr [edi + ebx * 4], 0x12345678
			compiledOfs -= 7;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString(buf, "BA");		// mov edx, 0x12345678

			if(andit)
				Emit4(buf, lastConst & andit);
			else
				Emit4(buf, lastConst);
			
			return;
		}
		else
			EmitString(buf, "8B 14 9F");	// mov edx, dword ptr [edi + ebx * 4]
		
	}
	else
		EmitString(buf, "8B 14 9F");		// mov edx, dword ptr [edi + ebx * 4]
	
	if(andit)
		EmitMaskReg(buf, "E2", andit);		// and edx, 0x12345678
}

static void Set_JUsed(unsigned char * buf, int x, unsigned char * jused, vm_t * vm)
{
	if (x < 0 || x >= vm->instructionCount)
	{
		Z_Free(buf); 
		Z_Free(jused);
		Com_Error( ERR_DROP,
				"VM_CompileX86: jump target out of range at offset %d", pc ); \
	} 
	jused[x] = 1;
}

static void SET_JMPOFS(unsigned char *buf, int x)
{
	buf[(x)] = compiledOfs - ((x) + 1);
}


/*
=================
ErrJump
Error handler for jump/call to invalid instruction number
=================
*/

static void __attribute__((__noreturn__)) ErrJump(void)
{ 
	Com_Error(ERR_DROP, "program tried to execute code outside VM");
}

/*
=================
DoSyscall

Assembler helper routines will write its arguments directly to global variables so as to
work around different calling conventions
=================
*/

int vm_syscallNum;
int vm_programStack;
int *vm_opStackBase;
uint8_t vm_opStackOfs;
intptr_t vm_arg;

static void DoSyscall(void)
{
	vm_t *savedVM;

	// save currentVM so as to allow for recursive VM entry
	savedVM = currentVM;
	// modify VM stack pointer for recursive VM entry
	currentVM->programStack = vm_programStack - 4;

	if(vm_syscallNum < 0)
	{
		int *data, *ret;
#if idx64
		int index;
		intptr_t args[MAX_VMSYSCALL_ARGS];
#endif
		
		data = (int *) (savedVM->dataBase + vm_programStack + 4);
		ret = &vm_opStackBase[vm_opStackOfs + 1];

#if idx64
		args[0] = ~vm_syscallNum;
		for(index = 1; index < ARRAY_LEN(args); index++)
			args[index] = data[index];
			
		*ret = savedVM->systemCall(args);
#else
		data[0] = ~vm_syscallNum;
		*ret = savedVM->systemCall((intptr_t *) data);
#endif
	}
	else
	{
		switch(vm_syscallNum)
		{
		case VM_JMP_VIOLATION:
			ErrJump();
		break;
		case VM_BLOCK_COPY: 
			if(vm_opStackOfs < 1)
				Com_Error(ERR_DROP, "VM_BLOCK_COPY failed due to corrupted opStack");
			
			VM_BlockCopy(vm_opStackBase[(vm_opStackOfs - 1)], vm_opStackBase[vm_opStackOfs], vm_arg);
		break;
		default:
			Com_Error(ERR_DROP, "Unknown VM operation %d", vm_syscallNum);
		break;
		}
	}

	currentVM = savedVM;
}

/*
=================
EmitCallRel
Relative call to vm->codeBase + callOfs
=================
*/

static void EmitCallRel(unsigned char * buf, vm_t *vm, int callOfs)
{
	EmitString(buf, "E8");			// call 0x12345678
	Emit4(buf, callOfs - compiledOfs - 4);
}

/*
=================
Call to DoSyscall()
=================
*/

static int EmitCallDoSyscall(unsigned char * buf, vm_t *vm)
{
	// use edx register to store DoSyscall address
	EmitRexString(buf, 0x48, "BA");		// mov edx, DoSyscall
	EmitPtr(buf, DoSyscall);

	// Push important registers to stack as we can't really make
	// any assumptions about calling conventions.
	EmitString(buf, "51");			// push ebx
	EmitString(buf, "56");			// push esi
	EmitString(buf, "57");			// push edi
#if idx64
	EmitRexString(buf, 0x41, "50");		// push r8
	EmitRexString(buf, 0x41, "51");		// push r9
#endif

	// write arguments to global vars
	// syscall number
	EmitString(buf, "A3");			// mov [0x12345678], eax
	EmitPtr(buf, &vm_syscallNum);
	// vm_programStack value
	EmitString(buf, "89 F0");		// mov eax, esi
	EmitString(buf, "A3");			// mov [0x12345678], eax
	EmitPtr(buf, &vm_programStack);
	// vm_opStackOfs 
	EmitString(buf, "88 D8");			// mov al, bl
	EmitString(buf, "A2");			// mov [0x12345678], al
	EmitPtr(buf, &vm_opStackOfs);
	// vm_opStackBase
	EmitRexString(buf, 0x48, "89 F8");		// mov eax, edi
	EmitRexString(buf, 0x48, "A3");		// mov [0x12345678], eax
	EmitPtr(buf, &vm_opStackBase);
	// vm_arg
	EmitString(buf, "89 C8");			// mov eax, ecx
	EmitString(buf, "A3");			// mov [0x12345678], eax
	EmitPtr(buf, &vm_arg);
	
	// align the stack pointer to a 16-byte-boundary
	EmitString(buf, "55");			// push ebp
	EmitRexString(buf, 0x48, "89 E5");		// mov ebp, esp
	EmitRexString(buf, 0x48, "83 E4 F0");	// and esp, 0xFFFFFFF0
			
	// call the syscall wrapper function DoSyscall()

	EmitString(buf, "FF D2");			// call edx

	// reset the stack pointer to its previous value
	EmitRexString(buf, 0x48, "89 EC");		// mov esp, ebp
	EmitString(buf, "5D");			// pop ebp

#if idx64
	EmitRexString(buf, 0x41, "59");		// pop r9
	EmitRexString(buf, 0x41, "58");		// pop r8
#endif
	EmitString(buf, "5F");			// pop edi
	EmitString(buf, "5E");			// pop esi
	EmitString(buf, "59");			// pop ebx

	EmitString(buf, "C3");			// ret

	return compiledOfs;
}

/*
=================
EmitCallErrJump
Emit the code that triggers execution of the jump violation handler
=================
*/

static void EmitCallErrJump(unsigned char * buf, vm_t *vm, int sysCallOfs)
{
	EmitString(buf, "B8");			// mov eax, 0x12345678
	Emit4(buf, VM_JMP_VIOLATION);

	EmitCallRel(buf, vm, sysCallOfs);
}


/*
=================
EmitCallProcedure
VM OP_CALL procedure for call destinations obtained at runtime
=================
*/
static int EmitCallProcedure(unsigned char * buf, vm_t *vm, int sysCallOfs)
{
	int jmpSystemCall, jmpBadAddr;
	int retval;

	EmitString(buf, "8B 04 9F");		// mov eax, dword ptr [edi + ebx * 4]
	EmitString(buf, "80 EB");
	Emit1(buf, 1);			// sub bl, 1
	EmitString(buf, "85 C0");		// test eax, eax

	// Jump to syscall code, 1 byte offset should suffice
	EmitString(buf, "7C");		// jl systemCall
	jmpSystemCall = compiledOfs++;
		
	/************ Call inside VM ************/
	
	EmitString(buf, "81 F8");		// cmp eax, vm->instructionCount
	Emit4(buf, vm->instructionCount);
		
	// Error jump if invalid jump target
	EmitString(buf, "73");		// jae badAddr
	jmpBadAddr = compiledOfs++;

#if idx64
	EmitRexString(buf, 0x49, "FF 14 C0");	// call qword ptr [r8 + eax * 8]
#else
	EmitString(buf, "FF 14 85");			// call dword ptr [vm->instructionPointers + eax * 4]
	Emit4(buf, (intptr_t) vm->instructionPointers);
#endif
	EmitString(buf, "8B 04 9F");			// mov eax, dword ptr [edi + ebx * 4]
	EmitString(buf, "C3");			// ret
		
	// badAddr:
	SET_JMPOFS(buf, jmpBadAddr);
	EmitCallErrJump(buf, vm, sysCallOfs);

	/************ System Call ************/
	
	// systemCall:
	SET_JMPOFS(buf, jmpSystemCall);
	retval = compiledOfs;

	EmitCallRel(buf, vm, sysCallOfs);

	// have opStack reg point at return value
	// add bl, 1
    EmitString(buf, "80 C3");
	Emit1(buf, 1);
	EmitString(buf, "C3");		// ret

	return retval;
}

/*
=================
EmitJumpIns
Jump to constant instruction number
=================
*/
static void EmitJumpIns(unsigned char * buf, vm_t *vm, const char *jmpop, int cdest, int pass, unsigned char * jused)
{
	Set_JUsed(buf, cdest, jused, vm);

	EmitString(buf, jmpop);	// j??? 0x12345678

	// we only know all the jump addresses in the third pass
	if(pass == 2)
		Emit4(buf, vm->instructionPointers[cdest] - compiledOfs - 4);
	else
		compiledOfs += 4;
}

/*
=================
EmitCallIns
Call to constant instruction number
=================
*/

static void EmitCallIns(unsigned char * buf, vm_t *vm, int cdest, int pass, unsigned char *jused)
{
	Set_JUsed(buf, cdest, jused, vm);

	EmitString(buf, "E8");	// call 0x12345678

	// we only know all the jump addresses in the third pass
	if(pass == 2)
		Emit4(buf, vm->instructionPointers[cdest] - compiledOfs - 4);
	else
		compiledOfs += 4;
}

/*
=================
EmitCallConst
Call to constant instruction number or syscall
=================
*/

static void EmitCallConst(unsigned char * buf, vm_t *vm, int cdest, int callProcOfsSyscall, int pass, unsigned char * jused)
{
	if(cdest < 0)
	{
		EmitString(buf, "B8");	// mov eax, cdest
		Emit4(buf, cdest);

		EmitCallRel(buf, vm, callProcOfsSyscall);
	}
	else
		EmitCallIns(buf, vm, cdest, pass, jused);
}

/*
=================
EmitBranchConditions
Emits x86 branch condition as given in op
=================
*/
static void EmitBranchConditions(unsigned char * buf, vm_t *vm, int op, int pass, unsigned char * jused, unsigned char * code)
{
	switch(op)
	{
	case OP_EQ:
		EmitJumpIns(buf, vm, "0F 84", Constant4(code), pass, jused);	// je 0x12345678
	break;
	case OP_NE:
		EmitJumpIns(buf, vm, "0F 85", Constant4(code), pass, jused);	// jne 0x12345678
	break;
	case OP_LTI:
		EmitJumpIns(buf, vm, "0F 8C", Constant4(code), pass, jused);	// jl 0x12345678
	break;
	case OP_LEI:
		EmitJumpIns(buf, vm, "0F 8E", Constant4(code), pass, jused);	// jle 0x12345678
	break;
	case OP_GTI:
		EmitJumpIns(buf, vm, "0F 8F", Constant4(code), pass, jused);	// jg 0x12345678
	break;
	case OP_GEI:
		EmitJumpIns(buf, vm, "0F 8D", Constant4(code), pass, jused);	// jge 0x12345678
	break;
	case OP_LTU:
		EmitJumpIns(buf, vm, "0F 82", Constant4(code), pass, jused);	// jb 0x12345678
	break;
	case OP_LEU:
		EmitJumpIns(buf, vm, "0F 86", Constant4(code), pass, jused);	// jbe 0x12345678
	break;
	case OP_GTU:
		EmitJumpIns(buf, vm, "0F 87", Constant4(code), pass, jused);	// ja 0x12345678
	break;
	case OP_GEU:
		EmitJumpIns(buf, vm, "0F 83", Constant4(code), pass, jused);	// jae 0x12345678
	break;
	}
}


/*
=================
ConstOptimize
Constant values for immediately following instructions may be translated to immediate values
instead of opStack operations, which will save expensive operations on memory
=================
*/

static qboolean ConstOptimize(unsigned char * buf, vm_t *vm, int callProcOfsSyscall, int pass, unsigned char * jused, unsigned char * code)
{
	int v;
	int op1;

	// we can safely perform optimizations only in case if 
	// we are 100% sure that next instruction is not a jump label
	if (vm->jumpTableTargets && !jused[instruction])
		op1 = code[pc+4];
	else
		return qfalse;

	switch ( op1 ) {

	case OP_LOAD4:
		EmitPushStack(buf, vm);
#if idx64
		EmitRexString(buf, 0x41, "8B 81");			// mov eax, dword ptr [r9 + 0x12345678]
		Emit4(buf, Constant4(code) & vm->dataMask);
#else
		EmitString(buf, "B8");				// mov eax, 0x12345678
		EmitPtr(buf, vm->dataBase + (Constant4(code) & vm->dataMask));
		EmitString(buf, "8B 00");				// mov eax, dword ptr [eax]
#endif
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		++pc;						// OP_LOAD4
		++instruction;
		return qtrue;

	case OP_LOAD2:
		EmitPushStack(buf, vm);
#if idx64
		EmitRexString(buf, 0x41, "0F B7 81");		// movzx eax, word ptr [r9 + 0x12345678]
		Emit4(buf, Constant4(code) & vm->dataMask);
#else
		EmitString(buf, "B8");				// mov eax, 0x12345678
		EmitPtr(buf, vm->dataBase + (Constant4(code) & vm->dataMask));
		EmitString(buf, "0F B7 00");				// movzx eax, word ptr [eax]
#endif
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		pc++;						// OP_LOAD2
		instruction += 1;
		return qtrue;

	case OP_LOAD1:
		EmitPushStack(buf, vm);
#if idx64
		EmitRexString(buf, 0x41, "0F B6 81");		// movzx eax, byte ptr [r9 + 0x12345678]
		Emit4(buf, Constant4(code) & vm->dataMask);
#else
		EmitString(buf, "B8");				// mov eax, 0x12345678
		EmitPtr(buf, vm->dataBase + (Constant4(code) & vm->dataMask));
		EmitString(buf, "0F B6 00");				// movzx eax, byte ptr [eax]
#endif
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		pc++;						// OP_LOAD1
		instruction += 1;
		return qtrue;

	case OP_STORE4:
		EmitMovEAXStack(buf, vm, (vm->dataMask & ~3));
#if idx64
		EmitRexString(buf, 0x41, "C7 04 01");		// mov dword ptr [r9 + eax], 0x12345678
		Emit4(buf, Constant4(code));
#else
		EmitString(buf, "C7 80");				// mov dword ptr [eax + 0x12345678], 0x12345678
		Emit4(buf, (intptr_t) vm->dataBase);
		Emit4(buf, Constant4(code));
#endif
		EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
		pc++;						// OP_STORE4
		instruction += 1;
		return qtrue;

	case OP_STORE2:
		EmitMovEAXStack(buf, vm, (vm->dataMask & ~1));
#if idx64
		Emit1(buf, 0x66);					// mov word ptr [r9 + eax], 0x1234
		EmitRexString(buf, 0x41, "C7 04 01");
		Emit2(buf, Constant4(code));
#else
		EmitString(buf, "66 C7 80");				// mov word ptr [eax + 0x12345678], 0x1234
		Emit4(buf, (intptr_t) vm->dataBase);
		Emit2(buf, Constant4(code));
#endif
		EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1

		pc++;						// OP_STORE2
		instruction += 1;
		return qtrue;

	case OP_STORE1:
		EmitMovEAXStack(buf, vm, vm->dataMask);
#if idx64
		EmitRexString(buf, 0x41, "C6 04 01");		// mov byte [r9 + eax], 0x12
		Emit1(buf, Constant4(code));
#else
		EmitString(buf, "C6 80");				// mov byte ptr [eax + 0x12345678], 0x12
		Emit4(buf, (intptr_t) vm->dataBase);
		Emit1(buf, Constant4(code));
#endif
		EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1

		pc++;						// OP_STORE1
		instruction += 1;
		return qtrue;

	case OP_ADD:
		v = Constant4(code);

		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "83 C0");			// add eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "05");			// add eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);

		pc++;						// OP_ADD
		instruction += 1;
		return qtrue;

	case OP_SUB:
		v = Constant4(code);

		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "83 E8");			// sub eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "2D");			// sub eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);

		pc++;						// OP_SUB
		instruction += 1;
		return qtrue;

	case OP_MULI:
		v = Constant4(code);

		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "6B C0");			// imul eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "69 C0");			// imul eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);
		pc++;						// OP_MULI
		instruction += 1;

		return qtrue;

	case OP_LSH:
		v = (code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24));
		if(v < 0 || v > 31)
			break;

		EmitMovEAXStack(buf, vm, 0);
		EmitString(buf, "C1 E0");				// shl eax, 0x12
		Emit1(buf, v);
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_LSH
		instruction += 1;
		return qtrue;

	case OP_RSHI:
		v = (code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24));
		if(v < 0 || v > 31)
			break;
			
		EmitMovEAXStack(buf, vm, 0);
		EmitString(buf, "C1 F8");				// sar eax, 0x12
		Emit1(buf, v);
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_RSHI
		instruction += 1;
		return qtrue;

	case OP_RSHU:
		v = (code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24));
		if(v < 0 || v > 31)
			break;
			
		EmitMovEAXStack(buf, vm, 0);
		EmitString(buf, "C1 E8");				// shr eax, 0x12
		Emit1(buf, v);
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_RSHU
		instruction += 1;
		return qtrue;
	
	case OP_BAND:
		v = Constant4(code);

		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "83 E0");			// and eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "25");			// and eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;					// OP_BAND
		instruction += 1;
		return qtrue;

	case OP_BOR:
		v = Constant4(code);

		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "83 C8");			// or eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "0D");			// or eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;				 	// OP_BOR
		instruction += 1;
		return qtrue;

	case OP_BXOR:
		v = Constant4(code);
		
		EmitMovEAXStack(buf, vm, 0);
		if(iss8(v))
		{
			EmitString(buf, "83 F0");			// xor eax, 0x7F
			Emit1(buf, v);
		}
		else
		{
			EmitString(buf, "35");			// xor eax, 0x12345678
			Emit4(buf, v);
		}
		EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;					// OP_BXOR
		instruction += 1;
		return qtrue;

	case OP_EQ:
	case OP_NE:
	case OP_LTI:
	case OP_LEI:
	case OP_GTI:
	case OP_GEI:
	case OP_LTU:
	case OP_LEU:
	case OP_GTU:
	case OP_GEU:
		EmitMovEAXStack(buf, vm, 0);
		EmitCommand(buf, LAST_COMMAND_SUB_BL_1);
		EmitString(buf, "3D");				// cmp eax, 0x12345678
		Emit4(buf, Constant4(code));

		pc++;						// OP_*
		EmitBranchConditions(buf, vm, op1, pass, jused, code);
		instruction++;

		return qtrue;

	case OP_EQF:
	case OP_NEF:
		if(code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24))
			break;
		pc += 5;					// CONST + OP_EQF|OP_NEF

		EmitMovEAXStack(buf, vm, 0);
		EmitCommand(buf, LAST_COMMAND_SUB_BL_1);
		// floating point hack :)
		EmitString(buf, "25");				// and eax, 0x7FFFFFFF
		Emit4(buf, 0x7FFFFFFF);
		if(op1 == OP_EQF)
			EmitJumpIns(buf, vm, "0F 84", Constant4(code), pass, jused);	// jz 0x12345678
		else
			EmitJumpIns(buf, vm, "0F 85", Constant4(code), pass, jused);	// jnz 0x12345678
		
		instruction += 1;
		return qtrue;


	case OP_JUMP:
		EmitJumpIns(buf, vm, "E9", Constant4(code), pass, jused);		// jmp 0x12345678

		pc += 1;                  // OP_JUMP
		instruction += 1;
		return qtrue;

	case OP_CALL:
		v = Constant4(code);
		EmitCallConst(buf, vm, v, callProcOfsSyscall, pass, jused);

		pc += 1;                  // OP_CALL
		instruction += 1;
		return qtrue;

	default:
		break;
	}

	return qfalse;
}

///////////////////////////////////////////////////////////////////////
// debug

void FileSys_PrintfHexToFile(const char * name, unsigned char * pBuf, unsigned int len)
{
	char local_name[128] = { 0 };
#ifndef NDEBUG
	snprintf(local_name, 127, "compiled_%s_debug.txt", name);
#else
	snprintf(local_name, 127, "compiled_%s_release.txt", name);
#endif

	unsigned char * pOpcode = (unsigned char *) malloc ( len * 3 + len / 20 + 10);
	unsigned int j = 0;
	for (unsigned int i = 0; i < len; ++i)
	{
		Hes2char(pBuf[i], pOpcode+j);
		j += 2;
		pOpcode[j++] = ' ';
		if ( (i != 0 ) && (i % 20) == 0)
			pOpcode[j++] = '\n';
	}

	pOpcode[j] = '\0';

	FILE* log_fp = fopen(local_name, "wt");

	if (log_fp)
	{
		fprintf(log_fp, "%s", pOpcode);
	}
	else
	{
		fprintf(stderr, "Error open %s\n", name);
	}

	fclose(log_fp);

	free(pOpcode);
}


////////////////////////////////////////////////////////////////////

void VM_Compile(vm_t * const vm, vmHeader_t *header)
{
	int	op, v;
    int	callProcOfsSyscall, callProcOfs, callDoSyscallOfs;

	// allocate a very large temp buffer, we will shrink it later
	int maxLength = header->codeLength * 8 + 64;
	unsigned char* buf = Z_Malloc(maxLength);
    memset(buf, 0, maxLength);

	int jusedSize = header->instructionCount + 2;
	unsigned char* jused = Z_Malloc(jusedSize);
	memset(jused, 0, jusedSize);

	unsigned char* code = Z_Malloc(header->codeLength+32);
	memset(code, 0, header->codeLength+32);
	
	Com_Printf(" Starting VM_Compile ... \n");

	// copy code in larger buffer and put some zeros at the end
	// so we can safely look ahead for a few instructions in it
	// without a chance to get false-positive because of some garbage bytes
	memcpy(code, (byte *)header + header->codeOffset, header->codeLength );

	// ensure that the optimisation pass knows about all the jump
	// table targets
	pc = -1; // a bogus value to be printed in out-of-bounds error messages
	for(int i = 0; i < vm->numJumpTableTargets; ++i )
	{
		Set_JUsed(buf, *(int *)(vm->jumpTableTargets + ( i * sizeof( int ) ) ), jused, vm);
	}

	// Start buffer with x86-VM specific procedures
	compiledOfs = 0;

	callDoSyscallOfs = compiledOfs;
	callProcOfs = EmitCallDoSyscall(buf, vm);
	callProcOfsSyscall = EmitCallProcedure(buf, vm, callDoSyscallOfs);
	vm->entryOfs = compiledOfs;

	for(unsigned int pass=0; pass < 3; ++pass)
	{
	oc0 = -23423;
	oc1 = -234354;
	pop0 = -43435;
	pop1 = -545455;

	// translate all instructions
	pc = 0;
	instruction = 0;
	//code = (byte *)header + header->codeOffset;
	compiledOfs = vm->entryOfs;

	LastCommand = LAST_COMMAND_NONE;

	while(instruction < header->instructionCount)
	{
		if(compiledOfs > maxLength - 16)
		{
			Z_Free(buf);
			Z_Free(jused);
			Com_Error(ERR_DROP, "VM_CompileX86: maxLength exceeded");
		}

		vm->instructionPointers[ instruction ] = compiledOfs;

		if ( !vm->jumpTableTargets )
			jlabel = 1;
		else 
			jlabel = jused[ instruction ];

		instruction++;

		if(pc > header->codeLength)
		{
			Z_Free(buf);
			Z_Free(jused);
			Com_Error(ERR_DROP, "VM_CompileX86: pc > header->codeLength");
		}

		op = code[ pc++ ];
	
		switch ( op )
        {
		case 0:
			break;
		case OP_BREAK:
			EmitString(buf, "CC");				// int 3
			break;
		case OP_ENTER:
			EmitString(buf, "81 EE");				// sub esi, 0x12345678
			Emit4(buf, Constant4(code));
			break;
		case OP_CONST:
			if( ConstOptimize(buf, vm, callProcOfsSyscall, pass, jused, code) )
				break;

			EmitPushStack(buf, vm);
			EmitString(buf, "C7 04 9F");				// mov dword ptr [edi + ebx * 4], 0x12345678
			lastConst = Constant4(code);

			Emit4(buf, lastConst);
			if(code[pc] == OP_JUMP)
				Set_JUsed(buf, lastConst, jused, vm);

			break;
		case OP_LOCAL:
			EmitPushStack(buf, vm);
			EmitString(buf, "8D 86");				// lea eax, [0x12345678 + esi]
			oc0 = oc1;
			oc1 = Constant4(code);
			Emit4(buf, oc1);
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_ARG:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "8B D6");				// mov edx, esi
			EmitString(buf, "81 C2");				// add edx, 0x12345678
			Emit4(buf, code[pc++]);
			EmitMaskReg(buf, "E2", vm->dataMask);			// and edx, 0x12345678
#if idx64
			EmitRexString(buf, 0x41, "89 04 11");		// mov dword ptr [r9 + edx], eax
#else
			EmitString(buf, "89 82");				// mov dword ptr [edx + 0x12345678], eax
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_CALL:
			EmitCallRel(buf, vm, callProcOfs);
			break;
		case OP_PUSH:
			EmitPushStack(buf, vm);
			break;
		case OP_POP:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_LEAVE:
			v = Constant4(code);
			EmitString(buf, "81 C6");				// add	esi, 0x12345678
			Emit4(buf, v);
			EmitString(buf, "C3");				// ret
			break;
		case OP_LOAD4:
			if (code[pc] == OP_CONST && code[pc+5] == OP_ADD && code[pc+6] == OP_STORE4)
			{
				if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
					compiledOfs -= 12;
					vm->instructionPointers[instruction - 1] = compiledOfs;
				}

				pc++;				// OP_CONST
				v = Constant4(code);

				EmitMovEDXStack(buf, vm, vm->dataMask);
				if(v == 1 && oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
#if idx64
					EmitRexString(buf, 0x41, "FF 04 11");	// inc dword ptr [r9 + edx]
#else
					EmitString(buf, "FF 82");			// inc dword ptr [edx + 0x12345678]
					Emit4(buf, (intptr_t) vm->dataBase);
#endif
				}
				else
				{
#if idx64
					EmitRexString(buf, 0x41, "8B 04 11");	// mov eax, dword ptr [r9 + edx]
#else
					EmitString(buf, "8B 82");			// mov eax, dword ptr [edx + 0x12345678]
					Emit4(buf, (intptr_t) vm->dataBase);
#endif
					EmitString(buf, "05");			// add eax, v
					Emit4(buf, v);
					
					if (oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
					{
#if idx64
						EmitRexString(buf, 0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString(buf, "89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4(buf, (intptr_t) vm->dataBase);
#endif
					}
					else
					{
						EmitCommand(buf, LAST_COMMAND_SUB_BL_1);	// sub bl, 1
						EmitString(buf, "8B 14 9F");			// mov edx, dword ptr [edi + ebx * 4]
						EmitMaskReg(buf, "E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
						EmitRexString(buf, 0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString(buf, "89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4(buf, (intptr_t) vm->dataBase);
#endif
					}
				}

				EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
				pc++;						// OP_ADD
				pc++;						// OP_STORE
				instruction += 3;
				break;
			}

			if(code[pc] == OP_CONST && code[pc+5] == OP_SUB && code[pc+6] == OP_STORE4)
			{
				if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
					compiledOfs -= 12;
					vm->instructionPointers[instruction - 1] = compiledOfs;
				}
				
				pc++;					// OP_CONST
				v = Constant4(code);

				EmitMovEDXStack(buf, vm, vm->dataMask);
				if(v == 1 && oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
#if idx64
					EmitRexString(buf, 0x41, "FF 0C 11");	// dec dword ptr [r9 + edx]
#else
					EmitString(buf, "FF 8A");			// dec dword ptr [edx + 0x12345678]
					Emit4(buf, (intptr_t) vm->dataBase);
#endif
				}
				else
				{
#if idx64
					EmitRexString(buf, 0x41, "8B 04 11");	// mov eax, dword ptr [r9 + edx]
#else
					EmitString(buf, "8B 82");			// mov eax, dword ptr [edx + 0x12345678]
					Emit4(buf, (intptr_t) vm->dataBase);
#endif
					EmitString(buf, "2D");			// sub eax, v
					Emit4(buf, v);
					
					if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
					{
#if idx64
						EmitRexString(buf, 0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString("89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4((intptr_t) vm->dataBase);
#endif
					}
					else
					{
						EmitCommand(buf, LAST_COMMAND_SUB_BL_1);	// sub bl, 1
						EmitString(buf, "8B 14 9F");			// mov edx, dword ptr [edi + ebx * 4]
						EmitMaskReg(buf, "E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
						EmitRexString(buf, 0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString(buf, "89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4(buf, (intptr_t) vm->dataBase);
#endif
					}
				}
				EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
				pc++;						// OP_SUB
				pc++;						// OP_STORE
				instruction += 3;
				break;
			}

			if(buf[compiledOfs - 3] == 0x89 && buf[compiledOfs - 2] == 0x04 && buf[compiledOfs - 1] == 0x9F)
			{
				compiledOfs -= 3;
				vm->instructionPointers[instruction - 1] = compiledOfs;
				EmitMaskReg(buf, "E0", vm->dataMask);			// and eax, 0x12345678
#if idx64
				EmitRexString(buf, 0x41, "8B 04 01");		// mov eax, dword ptr [r9 + eax]
#else
				EmitString(buf, "8B 80");				// mov eax, dword ptr [eax + 0x1234567]
				Emit4(buf, (intptr_t) vm->dataBase);
#endif
				EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
				break;
			}
			
			EmitMovEAXStack(buf, vm, vm->dataMask);
#if idx64
			EmitRexString(buf, 0x41, "8B 04 01");		// mov eax, dword ptr [r9 + eax]
#else
			EmitString(buf, "8B 80");				// mov eax, dword ptr [eax + 0x12345678]
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_LOAD2:
			EmitMovEAXStack(buf, vm, vm->dataMask);
#if idx64
			EmitRexString(buf, 0x41, "0F B7 04 01");		// movzx eax, word ptr [r9 + eax]
#else
			EmitString(buf, "0F B7 80");				// movzx eax, word ptr [eax + 0x12345678]
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_LOAD1:
			EmitMovEAXStack(buf, vm, vm->dataMask);
#if idx64
			EmitRexString(buf, 0x41, "0F B6 04 01");		// movzx eax, byte ptr [r9 + eax]
#else
			EmitString(buf, "0F B6 80");				// movzx eax, byte ptr [eax + 0x12345678]
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_STORE4:
			EmitMovEAXStack(buf, vm, 0);
			EmitString(buf, "8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			EmitMaskReg(buf, "E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
			EmitRexString(buf, 0x41, "89 04 11");		// mov dword ptr [r9 + edx], eax
#else
			EmitString(buf, "89 82");				// mov dword ptr [edx + 0x12345678], eax
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;
		case OP_STORE2:
			EmitMovEAXStack(buf, vm, 0);
			EmitString(buf, "8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			EmitMaskReg(buf, "E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
			Emit1(buf, 0x66);					// mov word ptr [r9 + edx], eax
			EmitRexString(buf, 0x41, "89 04 11");
#else
			EmitString(buf, "66 89 82");				// mov word ptr [edx + 0x12345678], eax
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;
		case OP_STORE1:
			EmitMovEAXStack(buf, vm, 0);
			EmitString(buf, "8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			EmitMaskReg(buf, "E2", vm->dataMask);			// and edx, 0x12345678
#if idx64
			EmitRexString(buf, 0x41, "88 04 11");		// mov byte ptr [r9 + edx], eax
#else
			EmitString(buf, "88 82");				// mov byte ptr [edx + 0x12345678], eax
			Emit4(buf, (intptr_t) vm->dataBase);
#endif
			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;

		case OP_EQ:
		case OP_NE:
		case OP_LTI:
		case OP_LEI:
		case OP_GTI:
		case OP_GEI:
		case OP_LTU:
		case OP_LEU:
		case OP_GTU:
		case OP_GEU:
			EmitMovEAXStack(buf, vm, 0);
			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			EmitString(buf, "39 44 9F 04");			// cmp	eax, dword ptr 4[edi + ebx * 4]

			EmitBranchConditions(buf, vm, op, pass, jused, code);
		break;
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			EmitString(buf, "D9 44 9F 04");			// fld dword ptr 4[edi + ebx * 4]
			EmitString(buf, "D8 5C 9F 08");			// fcomp dword ptr 8[edi + ebx * 4]
			EmitString(buf, "DF E0");				// fnstsw ax

			switch(op)
			{
			case OP_EQF:
				EmitString(buf, "F6 C4 40");			// test	ah,0x40
				EmitJumpIns(buf, vm, "0F 85", Constant4(code), pass, jused);	// jne 0x12345678
			break;
			case OP_NEF:
				EmitString(buf, "F6 C4 40");			// test	ah,0x40
				EmitJumpIns(buf, vm, "0F 84", Constant4(code), pass, jused);	// je 0x12345678
			break;
			case OP_LTF:
				EmitString(buf, "F6 C4 01");			// test	ah,0x01
				EmitJumpIns(buf, vm, "0F 85", Constant4(code), pass, jused);	// jne 0x12345678
			break;
			case OP_LEF:
				EmitString(buf, "F6 C4 41");			// test	ah,0x41
				EmitJumpIns(buf, vm, "0F 85", Constant4(code), pass, jused);	// jne 0x12345678
			break;
			case OP_GTF:
				EmitString(buf, "F6 C4 41");			// test	ah,0x41
				EmitJumpIns(buf, vm, "0F 84", Constant4(code), pass, jused);	// je 0x12345678
			break;
			case OP_GEF:
				EmitString(buf, "F6 C4 01");			// test	ah,0x01
				EmitJumpIns(buf, vm, "0F 84", Constant4(code), pass, jused);	// je 0x12345678
			break;
			}
		break;			
		case OP_NEGI:
			EmitMovEAXStack(buf, vm, 0);
			EmitString(buf, "F7 D8");				// neg eax
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);
			break;
		case OP_ADD:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "01 44 9F FC");			// add dword ptr -4[edi + ebx * 4], eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_SUB:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "29 44 9F FC");			// sub dword ptr -4[edi + ebx * 4], eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_DIVI:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "99");				// cdq
			EmitString(buf, "F7 3C 9F");				// idiv dword ptr [edi + ebx * 4]
			EmitString(buf, "89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_DIVU:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "33 D2");				// xor edx, edx
			EmitString(buf, "F7 34 9F");				// div dword ptr [edi + ebx * 4]
			EmitString(buf, "89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MODI:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "99" );				// cdq
			EmitString(buf, "F7 3C 9F");				// idiv dword ptr [edi + ebx * 4]
			EmitString(buf, "89 54 9F FC");			// mov dword ptr -4[edi + ebx * 4],edx
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MODU:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "33 D2");				// xor edx, edx
			EmitString(buf, "F7 34 9F");				// div dword ptr [edi + ebx * 4]
			EmitString(buf, "89 54 9F FC");			// mov dword ptr -4[edi + ebx * 4],edx
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MULI:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "F7 2C 9F");				// imul dword ptr [edi + ebx * 4]
			EmitString(buf, "89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MULU:
			EmitString(buf, "8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString(buf, "F7 24 9F");				// mul dword ptr [edi + ebx * 4]
			EmitString(buf, "89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BAND:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "21 44 9F FC");			// and dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BOR:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "09 44 9F FC");			// or dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BXOR:
			EmitMovEAXStack(buf, vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString(buf, "31 44 9F FC");			// xor dword ptr -4[edi + ebx * 4],eax
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BCOM:
			EmitString(buf, "F7 14 9F");				// not dword ptr [edi + ebx * 4]
			break;
		case OP_LSH:
			EmitMovECXStack(buf, vm);
			EmitString(buf, "D3 64 9F FC");			// shl dword ptr -4[edi + ebx * 4], cl
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_RSHI:
			EmitMovECXStack(buf, vm);
			EmitString(buf, "D3 7C 9F FC");			// sar dword ptr -4[edi + ebx * 4], cl
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_RSHU:
			EmitMovECXStack(buf, vm);
			EmitString(buf, "D3 6C 9F FC");			// shr dword ptr -4[edi + ebx * 4], cl
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_NEGF:
			EmitString(buf, "D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString(buf, "D9 E0");				// fchs
			EmitString(buf, "D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_ADDF:
			EmitString(buf, "D9 44 9F FC");			// fld dword ptr -4[edi + ebx * 4]
			EmitString(buf, "D8 04 9F");				// fadd dword ptr [edi + ebx * 4]
			EmitString(buf, "D9 5C 9F FC");			// fstp dword ptr -4[edi + ebx * 4]
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_SUBF:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString(buf, "D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString(buf, "D8 64 9F 04");			// fsub dword ptr 4[edi + ebx * 4]
			EmitString(buf, "D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_DIVF:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString(buf, "D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString(buf, "D8 74 9F 04");			// fdiv dword ptr 4[edi + ebx * 4]
			EmitString(buf, "D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_MULF:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString(buf, "D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString(buf, "D8 4C 9F 04");			// fmul dword ptr 4[edi + ebx * 4]
			EmitString(buf, "D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_CVIF:
			EmitString(buf, "DB 04 9F");				// fild dword ptr [edi + ebx * 4]
			EmitString(buf, "D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_CVFI:
#ifndef FTOL_PTR // WHENHELLISFROZENOVER
			// not IEEE complient, but simple and fast
			EmitString(buf, "D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString(buf, "DB 1C 9F");				// fistp dword ptr [edi + ebx * 4]
#else // FTOL_PTR

// call the library conversion function

			EmitRexString(buf, 0x48, "BA");			// mov edx, Q_VMftol
			EmitPtr(buf, Q_VMftol);
			EmitRexString(buf, 0x48, "FF D2");			// call edx
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
#endif
			break;
		case OP_SEX8:
			EmitString(buf, "0F BE 04 9F");			// movsx eax, byte ptr [edi + ebx * 4]
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_SEX16:
			EmitString(buf, "0F BF 04 9F");			// movsx eax, word ptr [edi + ebx * 4]
			EmitCommand(buf, LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;

		case OP_BLOCK_COPY:
			EmitString(buf, "B8");				// mov eax, 0x12345678
			Emit4(buf, VM_BLOCK_COPY);
			EmitString(buf, "B9");				// mov ecx, 0x12345678
			Emit4(buf, Constant4(code));

			EmitCallRel(buf, vm, callDoSyscallOfs);

			EmitCommand(buf, LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;

		case OP_JUMP:
			EmitCommand(buf, LAST_COMMAND_SUB_BL_1);	// sub bl, 1
			EmitString(buf, "8B 44 9F 04");		// mov eax, dword ptr 4[edi + ebx * 4]
			EmitString(buf, "81 F8");			// cmp eax, vm->instructionCount
			Emit4(buf, vm->instructionCount);
#if idx64
			EmitString(buf, "73 04");			// jae +4
			EmitRexString(buf, 0x49, "FF 24 C0");        // jmp qword ptr [r8 + eax * 8]
#else
			EmitString(buf, "73 07");			// jae +7
			EmitString(buf, "FF 24 85");			// jmp dword ptr [instructionPointers + eax * 4]
			Emit4(buf, (intptr_t) vm->instructionPointers);
#endif
			EmitCallErrJump(buf, vm, callDoSyscallOfs);
			break;
		default:
			Z_Free(buf);
			Z_Free(jused);
			Com_Error(ERR_DROP, "VM_CompileX86: bad opcode %i at offset %i", op, pc);
		}
		pop0 = pop1;
		pop1 = op;
	}
	}

	// copy to an exact sized buffer with the appropriate permission bits
	vm->codeLength = compiledOfs;
#ifdef VM_X86_MMAP
	vm->codeBase = mmap(NULL, compiledOfs, PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(vm->codeBase == MAP_FAILED)
		Com_Error(ERR_FATAL, "VM_CompileX86: can't mmap memory");
#elif _WIN32
	// allocate memory with EXECUTE permissions under windows.
	// Reserves, commits, or changes the state of a region of pages
	// in the virtual address space of the calling process.
	// Memory allocated by this function is automatically initialized to zero.

	vm->codeBase = VirtualAlloc(NULL, compiledOfs, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!vm->codeBase)
		Com_Error(ERR_FATAL, "VM_CompileX86: VirtualAlloc failed");
#else
	vm->codeBase = malloc(compiledOfs);
	if(!vm->codeBase)
	        Com_Error(ERR_FATAL, "VM_CompileX86: malloc failed");
#endif

	memcpy( vm->codeBase, buf, compiledOfs );
	
	// if debug
	FileSys_PrintfHexToFile(vm->name, buf, compiledOfs);

#ifdef VM_X86_MMAP
	if(mprotect(vm->codeBase, compiledOfs, PROT_READ|PROT_EXEC))
		Com_Error(ERR_FATAL, "VM_CompileX86: mprotect failed");
#elif _WIN32
	{
		DWORD oldProtect = 0;
		
		// remove write permissions.
		if(!VirtualProtect(vm->codeBase, compiledOfs, PAGE_EXECUTE_READ, &oldProtect))
			Com_Error(ERR_FATAL, "VM_CompileX86: VirtualProtect failed");
	}
#endif

	Z_Free( code );
	Z_Free( buf );
	Z_Free( jused );
	Com_Printf( "VM file %s compiled to %i bytes of code\n", 
		vm->name, compiledOfs );

	vm->destroy = VM_Destroy_Compiled;

	// offset all the instruction pointers for the new location
	for (int i = 0 ; i < header->instructionCount ; ++i ) {
		vm->instructionPointers[i] += (intptr_t) vm->codeBase;
	}
}



void VM_Destroy_Compiled(vm_t* self)
{
#ifdef VM_X86_MMAP
	munmap(self->codeBase, self->codeLength);
#elif _WIN32
	VirtualFree(self->codeBase, 0, MEM_RELEASE);
#else
	free(self->codeBase);
#endif
}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/
intptr_t VM_CallCompiled(vm_t * const vm, int *args)
{
	unsigned char stack[OPSTACK_SIZE + 15];

	const unsigned int szStack = 8 + 4 * MAX_VMMAIN_ARGS;
	currentVM = vm;

	// we might be called recursively, so this might not be the very top
	const int stackOnEntry = vm->programStack;
	int programStack = vm->programStack - szStack;
    
	// set up the stack frame 
	int * image = (int *)( vm->dataBase + programStack);


	for (unsigned int arg = 0; arg < MAX_VMMAIN_ARGS; ++arg )
		image[ 2 + arg  ] = args[ arg ];

	image[ 1 ] = 0;	// return stack
	image[ 0 ] = -1;	// will terminate the loop on return



	// off we go into generated code...
	void* entryPoint = vm->codeBase + vm->entryOfs;
	int* opStack = PADP(stack, 16);
	*opStack = 0xDEADBEEF;
	int opStackOfs = 0;


#ifdef _MSC_VER
  #if idx64
	opStackOfs = qvmcall64(&programStack, opStack, vm->instructionPointers, vm->dataBase);
  #else
	__asm
	{
		pushad

		mov	esi, dword ptr programStack
		mov	edi, dword ptr opStack
		mov	ebx, dword ptr opStackOfs

		call	entryPoint

		mov	dword ptr opStackOfs, ebx
		mov	dword ptr opStack, edi
		mov	dword ptr programStack, esi
		
		popad
	}
  #endif

#elif idx64
	__asm__ volatile(
		"movq %5, %%rax\n"
		"movq %3, %%r8\n"
		"movq %4, %%r9\n"
		"push %%r15\n"
		"push %%r14\n"
		"push %%r13\n"
		"push %%r12\n"
		"callq *%%rax\n"
		"pop %%r12\n"
		"pop %%r13\n"
		"pop %%r14\n"
		"pop %%r15\n"
		: "+S" (programStack), "+D" (opStack), "+b" (opStackOfs)
		: "g" (vm->instructionPointers), "g" (vm->dataBase), "g" (entryPoint)
		: "cc", "memory", "%rax", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11"
	);
#else
	__asm__ volatile(
		"calll *%3\n"
		: "+S" (programStack), "+D" (opStack), "+b" (opStackOfs)
		: "g" (entryPoint)
		: "cc", "memory", "%eax", "%ecx", "%edx"
	);
#endif

	if(opStackOfs != 1 || *opStack != 0xDEADBEEF)
	{
		Com_Error(ERR_DROP, "opStack corrupted in compiled code");
	}

	if (programStack != stackOnEntry - szStack)
	{
		Com_Error(ERR_DROP, "programStack(%d != %d) corrupted in compiled code",
			programStack, stackOnEntry - szStack);
	}

	vm->programStack = stackOnEntry;

	return opStack[opStackOfs];
}
