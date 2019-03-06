## The Virtual Machine Architecture

If previous engines delegated only the gameplay to the Virtual Machine,
idtech3 heavily rely on them for essential tasks. Among other things: 

* Rendition is triggered from the Client VM.
* The lag compensation mechanism is entirely in the Client VM.

Moreover their design is much more elaborated: 
They combine the security and portability of Quake1 Virtual Machine 
with the high performances of Quake2's native DLLs.
This is achieved by compiling the bytecode to x86 instruction on the fly.


The virtual machine was initially supposed to be a plain bytecode interpreter 
but performances were disappointing so the development team wrote a runtime x86 compiler.


According to the John Carmark's .plan from Nov 03, 1998:

<blockquote>
This was the most significant thing I talked about at The Frag, 
so here it is for everyone else. The way the QA game architecture
has been developed so far has been as two seperate binary dll¡¯s: 
one for the server side game logic, and one for the client side
presentation logic. While it was easiest to begin development like that, 
there are two crucial problems with shipping the game that way: security and portability.


It¡¯s one thing to ask the people who run dedicated servers to make
informed decisions about the safety of a given mod, but its a 
completely different matter to auto-download a binary image to a 
first time user connecting to a server they found. The quake 2 server
crashing attacks have certainly proven that there are hackers that 
enjoy attacking games, and shipping around binary code would be a 
very tempting opening for them to do some very nasty things.


With quake and Quake 2, all game modifications were strictly server side,
so any port of the game could connect to any server without problems.
With Quake 2¡¯s binary server dll¡¯s not all ports could necessarily run a
server, but they could all play. With significant chunks of code now 
running on the client side, if we stuck with binary dll¡¯s then the less
popular systems would find that they could not connect to new servers 
because the mod code hadn¡¯t been ported. I considered having things set
up in such a way that client game dll¡¯s could be sort of forwards-compatable, 
where they could always connect and play, but new commands and entity types
just might now show up. We could also GPL the game code to force mod authors
to release source with the binaries, but that would still be inconvenient to
deal with all the porting.


Related both issues is client side cheating. Certain cheats are easy to do
if you can hack the code, so the server will need to verify which code the
client is running. With multiple ported versions, it wouldn¡¯t be possible
to do any binary verification.
If we were willing to wed ourselves completely to the windows platform,
we might have pushed ahead with some attempt at binary verification
of dlls, but I ruled that option out. I want QuakeArena running on every
platform that has hardware accelerated OpenGL and an internet connection.


The only real solution to these problems is to use an interpreted language
like Quake 1 did. I have reached the conclusion that the benefits of a
standard language outweigh the benefits of a custom language for our
purposes. I would not go back and extend QC, because that stretches
the effort from simply system and interpreter design to include language
design, and there is already plenty to do.


I had been working under the assumption that Java was the right way to
go, but recently I reached a better conclusion. The programming language
for QuakeArena mods is interpreted ANSI C.  (well, I am dropping the double
data type, but otherwise it should be pretty conformant)


The game will have an interpreter for a virtual RISC-like CPU. This should
have a minor speed benefit over a byte-coded, stack based java interpreter.
Loads and stores are confined to a preset block of memory, and access to 
all external system facilities is done with system traps to the main game
code, so it is completely secure. The tools necessary for building mods 
will all be freely available: a modified version of LCC and a new program
called q3asm. LCC is a wonderful project - a cross platform, cross 
compiling ANSI C compiler done in under 20K lines of code. Anyone interested
in compilers should pick up a copy of ¡±A retargetable C compiler: design and
implementation¡± by Fraser and Hanson.


You can¡¯t link against any libraries, so every function must be resolved.
Things like strcmp, memcpy, rand, etc. must all be implemented directly.
I have code for all the ones I use, but some people may have to modify
their coding styles or provide implementations for other functions.
It is a fair amount of work to restructure all the interfaces to not share
pointers between the system and the games, but it is a whole lot easier
than porting everything to a new language. The client game code is about
10k lines, and the server game code is about 20k lines.


The drawback is performance. It will probably perform somewhat like
QC. Most of the heavy lifting is still done in the builtin functions for path
tracing and world sampling, but you could still hurt yourself by loop-
ing over tons of objects every frame. Yes, this does mean more load on
servers, but I am making some improvements in other parts that I hope
will balance things to about the way Q2 was on previous generation hardware.

There is also the amusing avenue of writing hand tuned virtual assembly
assembly language for critical functions..

I think this is The Right Thing.
</blockquote>

According to the .plan from Aug 16, 1999: 

<blockquote>
As I mentioned at quakecon, I decided to go ahead and try a dynamic
code generator to speed up the game interpreters. I was uneasy about it,
but the current performance was far enough off of my targets that I didn¡¯t
see any other way. At first, I was surprised at how quickly it was going. 
The first day, I worked out my system calling conventions and execution environment and
implemented enough opcode translations to get ¡±hello world¡± executing.
The second day I just plowed through opcode translations, tediously generating a lot of code like this:
</blockquote>


```
case OP_NEGI:
    EmitString( "F7 1F" );      // neg dword ptr [edi]
    break;
case OP_ADD:
    EmitString( "8B 07" );     // mov eax, dword ptr [edi]
    EmitString( "01 47 FC" );  // add dword ptr [edi-4],eax
    EmitString( "83 EF 04" );  // sub edi,4
    break;
case OP_SUB:
    EmitString( "8B 07" );     // mov eax, dword ptr [edi]
    EmitString( "29 47 FC" );  // sub dword ptr [edi-4],eax
    EmitString( "83 EF 04" );  // sub edi,4
    break;
case OP_DIVI:
    EmitString( "8B 47 FC" );  // mov eax,dword ptr [edi-4]
    EmitString( "99" );        // cdq
    EmitString( "F7 3F" );     // idiv dword ptr [edi]
    EmitString( "89 47 FC" );  // mov dword ptr [edi-4],eax
    EmitString( "83 EF 04" );  // sub edi,4
    break;
```




In Quake III a virtual machine is called a QVM: Three of them are loaded at any time:

* Client Side: Two virtual machine are loaded. Message are sent to one or the other depending on the gamestate:
        cgame : Receive messages during Battle phases. Performs entity culling, predictions and trigger renderer.lib.
        q3_ui : Receive messages during Menu phases. Uses system calls to draw the menus.

* Server Side:
        game : Always receive message: Perform gamelogic and hit bot.lib to perform A.I .


### QVM Internals

Before describing how the QVMs are used, let's check how the bytecode is generated.
As usual I prefer drawing with a little bit of complementary text: 

![](https://github.com/suijingfeng/engine/blob/master/doc/vm_chain_prod.png)

quake3.exe and its bytecode interpreter are generated via Visual Studio but the VM bytecode takes a very different path: 


1. Each .c file (translation unit) is compiled individually via LCC.

2. LCC is used with a special parameter so it does not output a PE (Windows Portable Executable) 
but rather its Intermediate Representation which is text based stack machine assembly. 
Each file produced features a text, data and bss section with symbols exports and imports.

3. A special tool from id Software q3asm.exe takes all text assembly files and assembles them together in one .qvm file. 
It also transform everything from text to binary (for speed in case the native converted cannot kick in). 
q3asm.exe also recognize which methods are system calls and give those a negative symbol number. 

4. Upon loading the binary bytecode, quake3.exe converts it to x86 instructions (not mandatory).


### LCC Internals

Here is a concrete example starting with a function that we want to run in the Virtual Machine: 

```
    extern int variableA;
    int variableB;
    int variableC=0;
    
    int fooFunction(char* string)
    {    
        return variableA + strlen(string);   
    }
```

Saved in module.c translation unit, lcc.exe is called with a special flag in order to 
avoid generating a Windows PE object but rather output the Intermediate Representation.
This is the LCC .obj output matching the C function above:

```
    data
    export variableC
    align 4
    LABELV variableC
    byte 4 0
    export fooFunction
    
    code
    proc fooFunction 4 4
    ADDRFP4 0
    INDIRP4
    ARGP4
    ADDRLP4 0
    ADDRGP4 strlen
    CALLI4
    ASGNI4
    ARGP4 variableA
    INDIRI4
    ADDRLP4 0
    INDIRI4
    ADDI4
    RETI4
    LABELV $1
    endproc fooFunction 4 4
    import strlen
    
    bss
    export variableB
    align 4
    LABELV variableB
    skip 4
    import variableA

```

A few observations:

* The bytecode is organized in sections (marked in red): 
    We can clearly see the bss (uninitialized variables), 
    data (initialized variables) and 
    code (usually called text but whatever..)
* Functions are defined via proc, endproc sandwich (marked in blue).
* The Intermediate Representation of LCC is a stack machine: 
    All operations are done on the stack with no assumptions made about CPU registers.
* At the end of the LCC phrase we have a bunch of files importing/exporting variables/functions.
* Each statement starts with the operation type (i.e: ARGP4, ADDRGP4, CALLI4...). Every parameter and result will be passed on the stack.
* Import and Export are here so the assembler can "link" translation units" together. 
    Notice import strlen, since neither q3asm.exe nor the VM Interpreter actually likes to the C Standard library, 
    strlen is considered a system call and must be provided by the Virtual Machine.

Such a text file is generated for each .c in the VM module. 


## q3asm.exe Internals
q3asm.exe takes the LCC Intermediate representation text files and assembles them together in a .qvm file:
![](https://github.com/suijingfeng/engine/blob/master/doc/vm_q3asm.png)


Several things to notice:

* q3asm makes sense of each import/export symbols across text files.
* Some methods are predefined via a system call text file. 
    You can see the syscall for the client VM and for the Server VMs.
    System calls symbols are attributed a negative integer value so they can be identified by the interpreter.
* q3asm change representation from text to binary in order to gain space and speed but that is pretty much it, no optimizations are performed here.
* The first method to be assembled MUST be vmMain since it is the input message dispatcher. 
    Moreover it MUST be located at 0x2D in the text segment of the bytecode.

## QVM: How it works

Again a drawing first illustrating the unique entry point and unique exit point that act as dispatch:
![](https://github.com/suijingfeng/engine/blob/master/doc/vm_bb.png)

A few details:

Messages (Quake3 -> VM) are send to the Virtual Machine as follow:

* Any part of Quake3 can call VM_Call( vm_t *vm, int callnum, ... ).
* VMCall takes up to 11 parameters and write each 4 bytes value in the VM bytecode (vm_t *vm) from 0x00 up to 0x26.
* VMCall writes the message id at 0x2A.
* The interpreter starts interpreting opcodes at 0x2D (where vmMain was placed by q3asm.exe).
* vmMain act as a dispatch and route the message to the appropriate bytecode method.

You can find the list of Message that can be sent to the [Client VM](https://github.com/id-Software/Quake-III-Arena/blob/master/code/cgame/cg_public.h)
and [Server VM](https://github.com/id-Software/Quake-III-Arena/blob/master/code/game/g_public.h) (at the bottom of each file).

System calls (VM -> Quake3) go out this way:

1. The interpreter execute the VM opcodes one after an other (`VM_CallInterpreted`).
2. When it encounters a CALLI4 opcode it checks the int method index.
3. If the value is negative then it is a system call.
4. The "system call function pointer" (`int (*systemCall)( int *parms )`) is called with the parameters.
5. The function pointed to by systemCall acts as a dispatch and route the system call to the right part of quake3.exe

* Parameters are always very simple types: Either primitives types (char,int,float) or pointer to primitive types `(char* , int[])`. 
I suspect this was done to minimize issues due to struct alignment between Visual Studio and LCC.

* Quake3 VM does not perform dynamic linking so a developer of a QVM mod had no access to any library, 
not even the C Standard Library (strlen, memset functions are here...but they are actually system calls). 
Some people still managed to fake it with preallocated buffer: [Malloc in QVM](http://icculus.org/homepages/phaethon/q3/malloc/malloc.html) !! 


## Productivity issue and solution

With such a long toolchain, developing VM code was difficult:

* The toolchain was slow.
* The toolchain was not integrated to Visual Studio.
* Building a QVM involved using commandline tools. It was cumbersome and interrupted the workflow.
* With so many elements in the toolchain it was hard to identify which part was at fault in case of bugs.

So idTech3 also have the ability to load a native DLL for the VM parts and it solved everything:
![](https://github.com/suijingfeng/engine/blob/master/doc/vm_chain_dev.png)

Overall the VM system is very versatile since a Virtual Machine is capable of running:

* Interpreted bytecode
* Bytecode compiled to x86 instructions
* Code compiled as a Windows DLL


##  opStack

* 32bit addressing, little endian
* 32bit floats
* about 60 instructions
* separate memory addressing for code and data
* no dynamic memory allocations
* no GP registers, load/store on opstack

* arguments for instructions, results
```
OP CONST 3
OP CONST 4
OP ADD
```
* function return values

## Calling Convention
    parameters and program counter pushed to stack
    callee responsible for new stack frame
    negative address means syscall
    return value on opstack

## VM Memory Layout
![](https://github.com/suijingfeng/engine/blob/master/doc/VM_Memory_Layout.png)

    vm memory allocated as one block
    64k stack at end of image
    code and opstack separate
    memory size rounded to power of two
    simple access violation checks via mask
    cannot address memory outside that block
    can't pass pointers

## Calling into the VM
```
intptr_t vmMain ( int command, int arg0, int arg1, ..., int arg11 )
{
    switch( command )
    {
        case GAME INIT:
            G_InitGame(arg0, arg1, arg2);
            return 0;
        case GAME SHUTDOWN:
            G_ShutdownGame(arg0);
            return 0;
        ...
    }
}    
```
    code is one big block, no symbols
    can only pass int parameters
    start of code has to be dispatcher function
        first parameter is a command 
        called vmMain() for dlopen()

## Calling into the Engine

* requests into engine
    print something
    open a file
    play s sound
* expensive operations
    memset, memcpy, sinus, cosinus, vector operation
* pass pointers but not return!

## More Speed With Native Code
* Byte code interpreter too slow for complex mods
* Translate byte code to native instructions on load
* CPU/OS specific compilers needed
    currently: x86, x86_64, ppc, sparc
* Multiple passes to calculate offsets
* Needs extra memory as native code is bigger
* Need to be careful when optimizing jump targets

## Reference Project
* https://github.com/jnz/q3vm
