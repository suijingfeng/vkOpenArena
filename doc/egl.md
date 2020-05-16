

EGL provides mechanisms for creating rendering surfaces onto which client APIs like OpenGL ES and OpenVG can draw,
creates graphics contexts for client APIs, and synchronizes drawing by client APIs as well as native platform
rendering APIs. This enables seamless rendering using both OpenGL ES and OpenVG for high-performance, accelerated,
mixed-mode 2D and 3D rendering.


EGL Native Platform Graphics Interface is an interface portable layer for graphics resource management
and works between rendering APIs such as OpenGL ES or OpenVG and the underlying native platform window system


Portable Layer for Graphics Resource Management


EGL can be implemented on multiple operating systems (such as Android and Linux) and native window systems
(such as X and Microsoft Windows). Implementations may also choose to allow rendering into specific types
of EGL surfaces via other supported native rendering APIs, such as Xlib or GDI. EGL provides:


1) Mechanisms for creating rendering surfaces (windows, pbuffers, pixmaps) onto which client APIs can draw
   and share
2) Methods to create and manage graphics contexts for client APIs
3) Ways to synchronize drawing by client APIs as well as native platform rendering APIs.
