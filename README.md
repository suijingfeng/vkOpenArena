# OpenArena Engine [![Build Status](https://travis-ci.org/OpenArena/engine.png?branch=master)](https://travis-ci.org/OpenArena/engine) #

This project is a fork of OpenArena with specific changes to the client and server.

## Building ##


If you are on Ubuntu or Debian, the easiest way to compile this is to install the build dependencies.

```sh
$ sudo apt-get install libcurl4-gnutls-dev libgl1-mesa-dev libsdl2-dev libopus-dev libopusfile-dev libogg-dev zlib1g-dev libvorbis-dev libopenal-dev libjpeg-dev libfreetype6-dev libxmp-dev
$ git clone https://github.com/suijingfeng/sepol.git
$ cd engine
$ make
```


## Switching renderers ##

Recent ioquake3 versions allow for a modular renderer.  This allows you to
select the renderer at runtime rather than compiling in one into the binary.

This feature is disabled by default.  If you wish to disable it, set USE_RENDERER_DLOPEN=1 in Makefile, 
this embeds the opengl2 renderer by default. In OpenArena, it embeds the openarena renderer.

When you start OpenArena, you can pass the name of the dynamic library to load. 
ioquake3 assumes a naming convention renderer_*_.

Example:

```sh
# Enable the default OpenArena renderer:
# This is based on the renderergl1 code.
$ ./openarena.x86_64 +set cl_renderer openarena

# Enable the default ioquake3 renderer (renderergl1):.
# If you are having trouble with OA's renderer, this is a safe alternative.
$ ./openarena.x86_64 +set cl_renderer opengl1

# Enable renderergl2 
$ ./openarena.x86_64 +set cl_renderer opengl2
```

## TODO ##

* Verify that allowing say/say_team to bypass Cmd_Args_Sanitize is safe.
* Try to avoid changing qcommon area to support GLSL.  Canete's renderergl2
  didn't need this change so this renderer shouldn't either.
* Build in FreeBSD and Mac OS X

## NOTTODO ##

* Modifications to the core Quake 3: Arena/ Team Arena gameplay. Put that into your great mod, don't screw with this.

* Removing the SDL/OpenAL header files from git. Having them in git makes it easier
for non-Linux platforms to compile the game. This isn't changing.

* MP3 support. It is already done, see: http://ioquake3.org/extras/patches

* All development should take place on the trunk. This includes bug fixes, new features and experimental stuff.Release branches should not receive any commits that aren't also made to the trunk. Normally you /shouldn't/ commit to release branches as such commits are periodically merged from the trunk. These merges are harder to perform if there are sporadic commits made in the interim.

* Under NO CIRCUMSTANCES ever commit to a tag. A tag is a static snapshot which is intended to be unchanging.

## Status ##

* Initial testing on Ubuntu16.04, Need help testing on other platforms




Maintainers

  * Sui Jingfeng <18949883232@163.com>

