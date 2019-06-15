# OpenArena Engine 

This project is a fork of OpenArena with specific changes to its renderer module.
It is mainly for myself to study Vulkan/OpenGL API and Quake3's engine. 
I add a vulkan-based render module to this game, the vulkan-based render have
the nearly same visual effects with the default opengl1 render. ( Informing you in case
of you are disapointed, vulkan is just a 2D/3D render API, whether the rendered
image is beautiful or not depend more on the textures，shaders，material provided). 
Of course I will try to add some new features to it，so the project is under developing, 
so always check the lastest build.

The opengl2 render module ported from ioq3 is more beatiful and powerful, 
you can play with the cvars(r\_pbr, r\_cubemapping, r\_hdr, r\_ssao ...) to enable
corresponding feature. It also consume more compute power and need more memory,
set com\_hunkMegs to 256 instand 128, otherwise it will quit and inform you hunk  
allocate failed when there are more player playing or in the case of some large map.
See ioq3 rendergl2 readme.


In my test( See the benchmark blow), The vulkan-based backend run faster on newer hardware,
however it may run slower than opengl1 on older harware. For example my notebook(i5 5200U, Gforce 840M),
but this notebook have a 1366x768 resolutions with ubuntu 16.04 installed, other computer is 1920x1080.
I'm now curious about whether it is accurate or meaningful, there are many factors, such as the monitor's
resolutions, the driver version, operate system and library etc. which one is major factor? You are 
welcome to have test on your pc configuration and report the results.


Quake 3 arena released by ID 20 years ago, this code lead me to something deep. 
It's still too large to understand for a noob programmers likes me. I'm naive, 
low level code modifier. I will take more time to learn Vulkan/OpenGL, data structure
and algorithm, computer network, operate system and compiler technology seriously, 
reduce aimless changing the code which tend to introduce bugs. There may have bugs
instrinsic in the code. If i could find it, i will try to fixed it.
However, there may some I am not able to find and some I could not fix in a short time. 


For people who want to try the vulkan based renderer on quake3's map,
go to https://github.com/suijingfeng/vkQuake3. 

You can also compile the render module from this repository and copy the renderer 
binary( renderer\_vulkan\_x86\_64.so or .dll ) to where ioquake3's executable
binary directory. Select the renderer in pull down window with `cl_renderer vulkan`
followed by `vid_restart`.


![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/quake3.jpg)

## Building on Ubuntu or Debian Linux ##

Install the build dependencies.

```sh
$ sudo apt-get install libcurl4-openssl-dev libsdl2-dev libopenal-dev libvulkan-dev libgl1-mesa-dev
$ sudo apt-get install clang gcc make git
$ git clone https://github.com/suijingfeng/vkOpenArena.git
$ cd vkOpenArena
$ make -j4
```

Please note that vulkan renderer requires at least SDL 2.0.6. 
The precompiled versions in some of the distribute repositories 
do not ship with Vulkan support, you therefore may come cross the
following problem when you launch OA:

```
Vulkan support is either not configured in SDL or not available in video driver.
```
This problem can be solved easily by compiling the SDL 2.0.9 from source:

Get a copy of the source code from https://www.libsdl.org/, then extract it.

```
cd SDL
mkdir build
cd build
../configure
make -j4
sudo make install
```


## Building on Windows 7 or 10 ##

To build 64-bit binaries, follow these instructions:

1. Install msys2 from https://msys2.github.io/ , following the instructions there.
2. Start "MinGW 64-bit" from the Start Menu, NOTE: NOT MSYS2.
3. Install mingw-w64-x86\_64, make, git and necessary libs.
```sh
pacman -S mingw-w64-x86_64-gcc make git
```
4. Grab latest openarena source code from github and compile. Note that in msys2, your drives are linked as folders in the root directory: C:\ is /c/, D:\ is /d/, and so on.

```sh
git clone https://github.com/suijingfeng/vkOpenArena.git
cd vkOpenArena
make -j4
```
5. Find the executables and dlls in build/release-mingw64-x86\_64 . 



## RUN ##
First, download the map packages from http://openarena.ws/download.php
Second, extract the data files at ~/.OpenArena/ (on linux) 
C:\Users\youname\AppData\Roaming\OpenArena\ (on windows)


```sh
$ cd ./build/release-linux-x86_64/
$ ./openarena.x86_64
```

## Change Field of Vision (FOV)

```
# cg_fov default value is 90, but it's a not good value for widescreen
# I generally set cg_fov to 150, the max value is 160, some game mod
# restrict this value to 140.

$ \cg_fov 150
```

## Too dark ? How to turn the brightness of the drawing window ?

* r\_brightness and r\_gamma, r\_gamma nonlinearly calibrate the image
before upload it to GPU, r\_brightness do it linearly,
both value can be float. You must restart the game after change it.

```
# large number with make the image more brighter.
$ \r_gamma 1.2
$ \r_brightness 1.5 
$ \vid_restart

```

* Use r\_intensity to turn the intensity/brightness of the lightmap. this value
can be float, you must restart the game after change it.
```
# this don't influence UI brightness, as it intended only for lightmap.
# 1.5 ~ 2.5 give acceptable result
$ \r_intensity 2
$ \vid_restart
```

* r\_mapOverbrightBit, integer, defalut is 2, r\_overbrightBit get remove in vulkan render, misleading.

* The image/texture is multiply/add(depend on the shader) with the lightmap.

NOTE: original gamma setting program setting the gamma of the entire destop by use SDL's hardware.
which works on newer computer hardware but not works on some old machine. 
It is buggy and embarrasing when program abnormal quit or stall accidently, left the desktop entire washing write.
so i change it,
we use software gamma only, take effects before the image is uploading to GPU.
One cvar for one thing, don't mess it with r\_overbrightBit and r\_mapOverbrightBit.



## Switching renderers ##

Q: How to enable vulkan support from the pulldown console ?

```sh

\cl_renerer vulkan
\vid_restart

# then play

# Enable renderergl2 ported from ioq3
\cl_renerer opengl2
\vid_restart

# Enable renderergl1 ported from ioq3
\cl_renerer opengl1
\vid_restart

# Enable openarena
\cl_renerer openarena
\vid_restart
```

When you start OpenArena, you can switch witch dynamic library to load by passing its name. 

Example:

```sh
# New vulkan renderer backend, under developing, 
# work on ubuntu 18.04, ubuntu16.04, win10, win7.
# associate code located in code/renderer_vulkan, see readme there.
$ ./openarena.x86_64 +set cl_renderer vulkan

# Enable renderergl2( borrowed from ioq3 ):
$ ./openarena.x86_64 +set cl_renderer opengl2

# Enable the renderergl1( borrowed from ioq3 ):
$ ./openarena.x86_64 +set cl_renderer opengl1


# Enable the default OpenArena renderer:
# This renderer module is similiar to the renderergl1 code.
$ ./openarena.x86_64 +set cl_renderer openarena
```


Q: How to check that Vulkan backend is really active ? 

```sh
\vkinfo

Type \vkinfo in the console reports information about active rendering backend.
It will report something as following:

Active 3D API: Vulkan
Vk api version: 1.0.65
Vk driver version: 1637679104
Vk vendor id: 0x10DE (NVIDIA)
Vk device id: 0x1B80
Vk device type: DISCRETE_GPU
Vk device name: GeForce GTX 1080

Total Device Extension Supported:

...

Vk instance extensions:

...

Image chuck memory(device local) used: 8 M 
```

besides, the following new cmd only exist in vulkan renderer.

* gpuMem: image memmory allocated on GPU;
* printOR: print the value of backend.or;
* pipelineList: list the number of pipelines we have created for the shader stage. (about 80~100, seem too much?)
* displayResoList: list of the display resolution the render software provided.
* monitorInfo: you monitor actually supported resolution, need the vulkan driver support VK\_KHR\_display,
    only supported on newer haveware and linux platform. It actually not work properly as intended on my two old
    notebook(in this casem it will not print anything), maybe the driver side bug (nvidia vulkan driver 1.0.65).
* printDeviceExtensions: list the device extensions supported on you GPU
* printInstanceExtensions: list the instance extensions supported on you GPU
* printImgHashTable: print the image hash table usage, which also list the image created and its size info etc.
* printShaderTextHashTable: I am curious about the q3 handle out the shader name clash problem.

For example:

```
$ \monitorInfo
 Display 0: LG Electronics LG HDR WFHD (HDMI-0). 
 Resolution: 2560x1080, Dimension: 800x340. 
 -------- --------------------- --------
 Display Mode Propertie 0: 2560x1080 60.0 hz. 
 Display Mode Propertie 1: 1920x1080 60.0 hz. 
 Display Mode Propertie 2: 1920x1080 59.9 hz. 
 Display Mode Propertie 3: 1280x720 59.9 hz. 
 Display Mode Propertie 4: 720x480 59.9 hz. 
 Display Mode Propertie 5: 640x480 59.9 hz. 
 Display Mode Propertie 6: 1920x1080 50.0 hz. 
 Display Mode Propertie 7: 1280x720 50.0 hz. 
 Display Mode Propertie 8: 2560x1080 50.0 hz. 
 Display Mode Propertie 9: 2560x1080 59.9 hz. 
 Display Mode Propertie 10: 720x576 50.0 hz. 
 Display Mode Propertie 11: 3840x2160 24.0 hz. 
 Display Mode Propertie 12: 3840x2160 25.0 hz. 
 Display Mode Propertie 13: 3840x2160 30.0 hz. 
 Display Mode Propertie 14: 3840x2160 50.0 hz. 
 Display Mode Propertie 15: 3840x2160 59.9 hz. 
 Display Mode Propertie 16: 2560x1080 75.0 hz. 
 Display Mode Propertie 17: 2560x1440 60.0 hz. 
 Display Mode Propertie 18: 1152x864 60.0 hz. 
 Display Mode Propertie 19: 1280x1024 60.0 hz. 
 Display Mode Propertie 20: 1280x720 60.0 hz. 
 Display Mode Propertie 21: 1600x900 60.0 hz. 
 Display Mode Propertie 22: 1680x1050 60.0 hz. 
 Display Mode Propertie 23: 1920x1080 60.0 hz. 
 Display Mode Propertie 24: 1280x800 59.8 hz. 
 Display Mode Propertie 25: 1920x1080 75.0 hz. 
 Display Mode Propertie 26: 640x480 59.9 hz. 
 Display Mode Propertie 27: 640x480 75.0 hz. 
 Display Mode Propertie 28: 800x600 60.3 hz. 
 Display Mode Propertie 29: 800x600 75.0 hz. 
 Display Mode Propertie 30: 1024x768 60.0 hz. 
 Display Mode Propertie 31: 1024x768 75.0 hz. 
 Display Mode Propertie 32: 1280x1024 75.0 hz. 
 Display Mode Propertie 33: 3840x2160 30.0 hz. 
 Display Mode Propertie 34: 3840x2160 25.0 hz. 
 Display Mode Propertie 35: 3840x2160 24.0 hz. 


# software provided resolution
$ \displayResoList 

Mode  0: 320x240
Mode  1: 400x300
Mode  2: 512x384
Mode  3: 640x480 (480p)
Mode  4: 800x600
Mode  5: 960x720
Mode  6: 1024x768
Mode  7: 1152x864
Mode  8: 1280x1024
Mode  9: 1600x1200
Mode 10: 2048x1536
Mode 11: 856x480
Mode 12: 1280x720 (720p)
Mode 13: 1280x768
Mode 14: 1280x800
Mode 15: 1280x960
Mode 16: 1360x768
Mode 17: 1366x768
Mode 18: 1360x1024
Mode 19: 1400x1050
Mode 20: 1400x900
Mode 21: 1600x900
Mode 22: 1680x1050
Mode 23: 1920x1080 (1080p)
Mode 24: 1920x1200
Mode 25: 1920x1440
Mode 26: 2560x1080
Mode 27: 2560x1600
Mode 28: 3840x2160 (4K)

$ \r_mode 12
$ \vid_restart
```


Q: How to use check FPS or using it as a benchmarking tool?

```
\timedemo 1
\demo demo088-test1 
```

### AMD2700X CPU, GTX1080 GPU, 1920x1080 60 hz LG display ###
```
# Frames  TotalTime  averageFPS  minimum/average/maximum/std deviation

# tested on win10 OS:
render_vulkan: 3398 frames 4.3 seconds 793.6 fps 1.0/1.3/4.0/0.5 ms
render_gl2:    3398 frames 24.6 seconds 138.3 fps 1.0/7.2/18.0/1.8 ms
render_gl1:    3398 frames 8.1 seconds 417.2 fps 1.0/2.4/8.0/0.9 ms
render_oa:     3398 frames 8.0 seconds 423.7 fps 1.0/2.4/8.0/0.9 ms

# tested on Ubuntu18.04:
render_vulkan: 3398 frames 3.6 seconds 935.6 fps 0.0/1.1/2.0/0.3 ms
render_gl2:    3398 frames 6.8 seconds 503.2 fps 1.0/2.0/6.0/0.6 ms
render_gl1:    3398 frames 5.4 seconds 629.7 fps 1.0/1.6/4.0/0.6 ms
render_mydev:  3398 frames 5.0 seconds 686.2 fps 1.0/1.5/3.0/0.5 ms
render_oa:     3398 frames 5.3 seconds 646.9 fps 1.0/1.5/4.0/0.5 ms
```

### HardWare: Aspire E5-471G-51SP, i5-5200U, GeForce 840M, 1366x768 ###

```
\timedemo 1
\demo demo088-test1

# Testing on UBUNTU 16.04, driver: 384.130, vulkan 1.0.49, tested on 2019.05.29,
vulkan : 3398 frames 21.0 seconds 162.2 fps 3.0/6.2/11.0/1.1 ms
opengl2: 3398 frames 25.1 seconds 135.5 fps 4.0/7.4/14.0/1.5 ms
opengl1: 3398 frames 10.1 seconds 337.0 fps 1.0/3.0/6.0/0.8 ms


# Testing on WIN10 Pro 64-bit, Driver version 397.31 
vulkan : 3398 frames 14.5 seconds 233.6 fps 1.0/4.3/14.0/1.6 ms
opengl2: 3398 frames 83.4 seconds 40.8 fps 8.0/24.5/331.0/9.6 ms
opengl1: 3398 frames 22.2 seconds 152.8 fps 2.0/6.5/17.0/1.9 ms

# opengl2 renderer achieve best virsual result but also consume the compute power most
# which in other word is slow, but it is ok on modern GPU/CPU.
# it is NOT OK on i5-5200U, GeForce 840M, I think.

\com_speed 1
# opengl2 backend
frame:24071 all: 13 sv:  0 ev:  0 cl:  0 gm:  0 rf:  1 bk: 12
frame:24072 all: 19 sv:  0 ev:  0 cl:  0 gm:  0 rf:  1 bk: 18
frame:24073 all:  9 sv:  1 ev:  0 cl:  0 gm:  0 rf:  1 bk:  7
frame:24074 all: 20 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk: 19
frame:24075 all:  9 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  8
frame:24080 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24081 all:  8 sv:  0 ev:  1 cl:  0 gm:  0 rf:  1 bk:  7
frame:24083 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24084 all: 17 sv:  0 ev:  0 cl:  1 gm:  0 rf:  1 bk: 15
frame:24085 all:  9 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  8
frame:24086 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24090 all:  9 sv:  1 ev:  0 cl:  0 gm:  0 rf:  0 bk:  8
frame:24092 all:  8 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  7
frame:24093 all:  9 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  8
frame:24094 all:  8 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  7
frame:24095 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24096 all:  9 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  8
frame:24101 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24102 all: 10 sv:  0 ev:  0 cl:  2 gm:  0 rf:  0 bk:  8
frame:24104 all: 11 sv:  0 ev:  0 cl:  0 gm:  1 rf:  0 bk: 10
frame:24105 all: 11 sv:  0 ev:  1 cl:  1 gm:  0 rf:  0 bk:  9
frame:24106 all: 20 sv:  0 ev:  0 cl:  0 gm:  0 rf:  1 bk: 19
frame:24107 all: 13 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk: 12
frame:24108 all: 15 sv:  0 ev:  1 cl:  1 gm:  0 rf:  0 bk: 14
frame:24109 all: 10 sv:  1 ev:  0 cl:  0 gm:  0 rf:  0 bk:  9
frame:24110 all: 12 sv:  1 ev:  0 cl:  1 gm:  0 rf:  0 bk: 10
frame:24111 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24113 all: 11 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk: 10
frame:24114 all: 10 sv:  0 ev:  0 cl:  0 gm:  1 rf:  0 bk:  9
frame:24116 all: 11 sv:  0 ev:  1 cl:  0 gm:  0 rf:  0 bk: 11
frame:24117 all: 11 sv:  0 ev:  0 cl:  0 gm:  1 rf:  0 bk: 10
frame:24118 all: 10 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk:  9
frame:24119 all: 14 sv:  0 ev:  1 cl:  0 gm:  0 rf:  0 bk: 14
frame:24120 all: 12 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk: 11
frame:24121 all: 11 sv:  0 ev:  0 cl:  1 gm:  0 rf:  0 bk: 10
```

### Aspire v3-772G i7-4702MQ GeForce GTX 760M 1920x1080 refresh_rate: 60Hz###
```
# Testing on ubuntu 18.04 gnome, nvidia vulkan driver 1.0.65 
render_vulkan: 3398 frames 15.0 seconds 225.9 fps 2.0/4.4/9.0/0.7 ms
# old
vulkan: 3398 frames 13.1 seconds 260.2 fps 2.0/3.8/7.0/0.7 ms
opengl1: 3398 frames 7.9 seconds 429.2 fps 1.0/2.3/6.0/0.6 ms
opengl2: 3398 frames 18.1 seconds 188.1 fps 3.0/5.3/23.0/1.0 ms
mydev: 3398 frames 7.5 seconds 450.5 fps 1.0/2.2/5.0/0.6 ms
openarena: 3398 frames 7.7 seconds 444.2 fps 1.0/2.3/5.0/0.6 ms


# Testing on win7, Vk api version: 1.1.70
Vk driver version: 1669513216

vulkan: 3398 frames 17.4 seconds 195.1 fps 1.0/5.1/24.0/1.8 ms
opengl1: 3398 frames 21.1 seconds 160.9 fps 2.0/6.2/23.0/1.8 ms
opengl2: 3398 frames 47.6 seconds 71.4 fps 5.0/14.0/81.0/2.4 ms

```

# framerate dropped due to non-professional modify
# I am also frustrated that the vulkan renderer not faster than
# opengl1/openarena backend on this notebook ...


```

### GT72VR 6RD, i7-6700HQ, GTX 1060, ubuntu 18.04 GNOME 3.28.2

```
vulkan:  3398 frames 4.6 seconds 731.2 fps 1.0/1.4/3.0/0.5 ms
opengl1: 3398 frames 5.8 seconds 589.2 fps 1.0/1.7/6.0/0.6 ms
opengl2: 3398 frames 7.2 seconds 469.7 fps 1.0/2.1/21.0/0.7 ms
```



You can also get the information from the UI: SETUP >> SYSTEM >> DRIVER INFO

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/driver_info.jpg)


# OpenArena gamecode

## Description
It's non engine part of OA, includes game, cgame and ui.
In mod form it is referred as OAX. 

## Loading native dll(.so)

```
cd linux_scripts/
./supermake_native
```


## Links
Development documentation is located here: https://github.com/OpenArena/gamecode/wiki

The development board on the OpenArena forum: http://openarena.ws/board/index.php?board=30.0

In particular the Open Arena Expanded topic: http://openarena.ws/board/index.php?topic=1908.0



## Status

* Initial testing on Ubuntu16.04 and Ubuntu18.04, Win7 and Win10


## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

## bugs/issues

* About com\_hunkmegs

When i playing CTF on :F for stupid server with default com\_hunkmegs = 128 setting, the following errors occurs:
```
ERROR: Hunk_Alloc failed on 739360: code/renderergl2/tr_model.c, line: 535 (sizeof(*v) * (md3Surf->numVerts * md3Surf->numFrames)).
```
OpenGL2 renderer seems use more memory, Upping com\_hunkmegs to 256 will generally be OK.


* Different results compile OA without -fno-strict-aliasing 
I am using GCC7.2 and clang6.0 on ubuntu18.04.

Build OA using clang with -fno-strict-aliasing removed:
```
WARNING: light grid mismatch, l->filelen=103896, numGridPoints*8=95904
```
This is printed by renderergl2's R\_LoadLightGrid function.

    Problem solved with following line added in it.
```
ri.Printf( PRINT_WARNING, "s_worldData.lightGridBounds[i]=%d\n", s_worldData.lightGridBounds[i]);
```
    However this line do nothing but just printed the value of s_worldData.lightGridBounds[i]. I guess its a bug of clang.

    Build OA with GCC without this issue.

* E\_AddRefEntityToScene passed a refEntity which has an origin with a NaN component
* Unpure client detected. Invalid .PK3 files referenced!
* Shader rocketThrust has a stage with no image

## TODO
* merge rendergl1, rendereroa, renderer\_mydev to one module
* r\_gamma shader
* have issues with \minimize when use vulkan renderer in fullscreen. recreate the swapchain ?
* flare support
* Implement RB\_SurfaceAxis();
* Split the program with the data.
* Use gprof to examine the performance of the program
```
gprof openarena.x86_64 gmon.out > report.txt
```


### Developing notes

```
\printInstanceExtensions
 
----- Total 12 Instance Extension Supported -----
VK_EXT_debug_report
VK_EXT_display_surface_counter
VK_KHR_get_physical_device_properties2
VK_KHR_get_surface_capabilities2
VK_KHR_surface
VK_KHR_win32_surface
VK_KHR_device_group_creation
VK_KHR_external_fence_capabilities
VK_KHR_external_memory_capabilities
VK_KHR_external_semaphore_capabilities
VK_NV_external_memory_capabilities
VK_EXT_debug_utils
----- ------------------------------------- -----

\printDeviceExtensions

--------- Total 40 Device Extension Supported ---------
 VK_KHR_swapchain
 VK_KHR_16bit_storage
 VK_KHR_bind_memory2
 VK_KHR_dedicated_allocation
 VK_KHR_descriptor_update_template
 VK_KHR_device_group
 VK_KHR_get_memory_requirements2
 VK_KHR_image_format_list
 VK_KHR_maintenance1
 VK_KHR_maintenance2
 VK_KHR_maintenance3
 VK_KHR_multiview
 VK_KHR_push_descriptor
 VK_KHR_relaxed_block_layout
 VK_KHR_sampler_mirror_clamp_to_edge
 VK_KHR_sampler_ycbcr_conversion
 VK_KHR_shader_draw_parameters
 VK_KHR_storage_buffer_storage_class
 VK_KHR_external_memory
 VK_KHR_external_memory_win32
 VK_KHR_external_semaphore
 VK_KHR_external_semaphore_win32
 VK_KHR_win32_keyed_mutex
 VK_KHR_external_fence
 VK_KHR_external_fence_win32
 VK_KHR_variable_pointers
 VK_KHX_device_group
 VK_KHX_multiview
 VK_EXT_blend_operation_advanced
 VK_EXT_depth_range_unrestricted
 VK_EXT_discard_rectangles
 VK_EXT_shader_subgroup_ballot
 VK_EXT_shader_subgroup_vote
 VK_NV_dedicated_allocation
 VK_NV_external_memory
 VK_NV_external_memory_win32
 VK_NV_glsl_shader
 VK_NV_win32_keyed_mutex
 VK_NVX_device_generated_commands
 VK_NVX_multiview_per_view_attributes
--------- ----------------------------------- ---------

```


### Ubuntu 18.04 gnome Aspire v3-772G i7-4702MQ GTX760M 1920x1080

```
total 2 Queue families:

 Queue family [0]: 16 queues,  Graphic,  Compute,  Transfer,  Sparse,  presentation supported.
 Queue family [1]: 1 queues,  Transfer,



----- Total 14 Instance Extension Supported -----
VK_EXT_acquire_xlib_display
VK_EXT_debug_report
VK_EXT_direct_mode_display
VK_EXT_display_surface_counter
VK_KHR_display
VK_KHR_get_physical_device_properties2
VK_KHR_get_surface_capabilities2
VK_KHR_surface
VK_KHR_xcb_surface
VK_KHR_xlib_surface
VK_KHR_external_fence_capabilities
VK_KHR_external_memory_capabilities
VK_KHR_external_semaphore_capabilities
VK_EXT_debug_utils
----- ------------------------------------- -----

\printDeviceExtensions 

--------- Total 33 Device Extension Supported ---------
 VK_KHR_swapchain 
 VK_KHR_16bit_storage 
 VK_KHR_bind_memory2 
 VK_KHR_dedicated_allocation 
 VK_KHR_descriptor_update_template 
 VK_KHR_get_memory_requirements2 
 VK_KHR_image_format_list 
 VK_KHR_maintenance1 
 VK_KHR_maintenance2 
 VK_KHR_push_descriptor 
 VK_KHR_relaxed_block_layout 
 VK_KHR_sampler_mirror_clamp_to_edge 
 VK_KHR_sampler_ycbcr_conversion 
 VK_KHR_shader_draw_parameters 
 VK_KHR_storage_buffer_storage_class 
 VK_KHR_external_memory 
 VK_KHR_external_memory_fd 
 VK_KHR_external_semaphore 
 VK_KHR_external_semaphore_fd 
 VK_KHR_external_fence 
 VK_KHR_external_fence_fd 
 VK_KHR_variable_pointers 
 VK_KHX_device_group 
 VK_KHX_multiview 
 VK_EXT_depth_range_unrestricted 
 VK_EXT_discard_rectangles 
 VK_EXT_display_control 
 VK_EXT_shader_subgroup_ballot 
 VK_EXT_shader_subgroup_vote 
 VK_NV_dedicated_allocation 
 VK_NV_glsl_shader 
 VK_NVX_device_generated_commands 
 VK_NVX_multiview_per_view_attributes 
--------- ----------------------------------- ---------

```
