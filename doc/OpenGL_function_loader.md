When do I need to use an OpenGL function loader?



OpenGL functions (core or extension) must be loaded at runtime, dynamically, whenever the function in question is not part of the platforms original OpenGL ABI (application binary interface).

    For Windows the ABI covers is OpenGL-1.1
    For Linux the covers OpenGL-1.2 (there's no official OpenGL ABI for other *nixes, but they usually require OpenGL-1.2 as well)
    For MacOS X the OpenGL version available and with it the ABI is defined by the OS version.

This leads to the following rules:

    In Windows you're going to need a function loader for pretty much everything, except single textured, shaderless, fixed function drawing; it may be possible to load further functionality, but this is not a given.
    In Linux you're going to need a function loader for pretty much everything, except basic multitextured with just the basic texenv modes, shaderless, fixed function drawing; it may be possible to load further functionality, but this is not a given.
    In MacOS X you don't need a function loader at all, but the OpenGL features you can use are strictly determined by the OS version, either you have it, or you don't.

The difference between core OpenGL functions and extensions is, that core functions are found in the OpenGL specification, while extensions are functionality that may or may be not available in addition to what the OpenGL version available provides.

Both extensions and newer version core functions are loaded through the same mechanism.


Core and extension status is not platform-dependent or even mutually exclusive.

Core means that some feature was introduced in a certain version of OpenGL. There are core functions, which are things that are guaranteed to exist in version X.Y and there are even core extensions, which are extensions that were introduced alongside version X.Y. Core extensions provide the same functions, types, enums, etc. as the core feature only in an extension form that does not require a specific version.

Framebuffer Objects went core in OpenGL 3.0, and are slightly less restrictive than the EXT extension (GL_EXT_framebuffer_object) that predates OpenGL 3.0. However, it is not necessary to have an OpenGL 3.0 implementation to have access to the core version of FBOs - an OpenGL 2.1 implementation might offer the core functionality.


In the extension specification for GL_ARB_framebuffer_object, you will find:

Why don't the new tokens and entry points in this extension have "ARB" suffixes like other ARB extensions?

    RESOLVED: Unlike most ARB extensions, this is a strict subset of functionality already approved in OpenGL 3.0. This extension exists only to support that functionality on older hardware that cannot implement a full OpenGL 3.0 driver. Since there are no possible behavior changes between the ARB extension and core features, source code compatibility is improved by not using suffixes on the extension.


Mac OS

Mac OS supports two main OpenGL feature sets:

    OpenGL 2.1 with legacy features. This is used by including <OpenGL/gl.h>.
    OpenGL 3.x and higher, Core Profile only. Used by including <OpenGL/gl3.h>.

In both cases, you don't need any dynamic function loading. The header files contain all the declarations/definitions for the maximum version that can be supported, and the framework you link against (using -framework OpenGL) resolves the function names.

The maximum version you can use at build time is determined by the platform SDK you build against. By default, this is he platform SDK that matches the OS of your build machine. But you can change it by using the -isysroot build option.

At runtime, the machine has to run at least the OS matching the platform SDK used at build time, and you can only use features up to the version supported by the GPU. You can get an overview of what version is supported on which hardware on:

Android, NDK

With native code on Android, you choose the OpenGL version while setting up the context and surface. Your code then includes the desired header (like <GLES2/gl2.h> or <GLES3/gl3.h>) and links against the matching libraries. There is no dynamic function loading needed.

If the target device does not support the version you are trying to use, the context creation will fail. You can have an entry in the manifest that prevents the app from being installed on devices that will not support the required ES version.

Android, Java

This is very similar to the NDK case. The desired version is specified during setup, e.g. while creating a GLSurfaceView.

The GLES20 class contains the definitions for ES 2.0. GLES30 derives from GLES20, and adds the additional definitions for ES 3.0.


iOS

Not surprisingly, this is very similar to Mac OS. You include the header file that matches the desired OpenGL ES version (e.g. <OpenGLES/ES3/gl.h>), link against the framework, and you're all done.

Also matching Mac OS, the maximum version you can build against is determined by the platform SDK version you choose. Devices you want to run on then have to use at least the OS version that matches this platform SDK version, and support the OpenGL ES version you are using.

One main difference is obviously that you cross compile the app on a Mac. iOS uses a different set of platform SDKs with different headers and frameworks, but the overall process is pretty much the same as building for Mac OS.
