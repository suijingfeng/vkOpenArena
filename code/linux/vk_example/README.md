### Vulkan
Vulkan is an explicit API, enabling direct control over how GPUs actually work.
The Vulkan API is a low overhead, explicit, cross-platform graphics API that provides applications with direct control over the GPU, maximizing application performance.

This Vulkan SDK does NOT include a Vulkan driver. The SDK allow you to build Vulkan
applications but you will need a Vulkan driver (ICD) to execute them. 
That is to say we have to install Nvidia, Amd, Intel's graphic drivers to run our application.
The ICD loader is a library that is placed between a Vulkan application and any number of Vulkan drivers

The SDK also includes certain Vulkan extensions for window system integration and debug extensions

### About version

A 1.1 SDK can still be used to develop Vulkan 1.0 applications,
but cannot be used to develop applications for future versions of Vulkan.
Furthermore, the presence of a 1.1 SDK does not necessarily indicate that a system can actually run Vulkan 1.1.


### Layer
A library designed to work as a plug-in for the loader. It usually serves to provide validation and debugging functionality to applications.

### SPIR-V
Standard Portable Intermediate Representation ¡ª A cross-API intermediate language that natively represents parallel compute and graphics programs


### compile shader file
run compile.sh to cimpile cube.vert cube.frag to cube\_frag.c cube\_vert.c

### SIGTRAP

With processors that support instruction breakpoints or data watchpoints, the debugger will ask the CPU to watch for instruction accesses to a specific address, or data reads/writes to a specific address, and then run full-speed.

When the processor detects the event, it will trap into the kernel, and the kernel will send SIGTRAP to the process being debugged. Normally, SIGTRAP would kill the process, but because it is being debugged, the debugger will be notified of the signal and handle it, mostly by letting you inspect the state of the process before continuing execution.

With processors that don't support breakpoints or watchpoints, the entire debugging environment is probably done through code interpretation and memory emulation, which is immensely slower. (I imagine clever tricks could be done by setting pagetable flags to forbid reading or writing, whichever needs to be trapped, and letting the kernel fix up the pagetables, signaling the debugger, and then restricting the page flags again. This could probably support near-arbitrary number of watchpoints and breakpoints, and run only marginally slower for cases when the watchpoint or breakpoint aren't frequently accessed.)

The question I placed into the comment field looks apropos here, only because Windows isn't actually sending a SIGTRAP, but rather signaling a breakpoint in its own native way. I assume when you're debugging programs, that debug versions of system libraries are used, and ensure that memory accesses appear to make sense. You might have a bug in your program that is papered-over at runtime, but may in fact be causing further problems elsewhere.
