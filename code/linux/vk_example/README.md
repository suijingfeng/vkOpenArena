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
