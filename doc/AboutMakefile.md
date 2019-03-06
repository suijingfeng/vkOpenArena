## About Makefile ##

The following variables may be set, either on the command line or in
Makefile.local:

```
  CFLAGS               - use this for custom CFLAGS
  V                    - set to show cc command line when building
  DEFAULT_BASEDIR      - extra path to search for baseq3 and such
  BUILD_SERVER         - build the 'ioq3ded' server binary
  BUILD_CLIENT         - build the 'ioquake3' client binary
  BUILD_BASEGAME       - build the 'baseq3' binaries
  BUILD_MISSIONPACK    - build the 'missionpack' binaries
  BUILD_GAME_SO        - build the game shared libraries
  BUILD_GAME_QVM       - build the game qvms
  BUILD_STANDALONE     - build binaries suited for stand-alone games
  SERVERBIN            - rename 'ioq3ded' server binary
  CLIENTBIN            - rename 'ioquake3' client binary
  USE_RENDERER_DLOPEN  - build and use the renderer in a library
  USE_YACC             - use yacc to update code/tools/lcc/lburg/gram.c
  BASEGAME             - rename 'baseq3'
  BASEGAME_CFLAGS      - custom CFLAGS for basegame
  MISSIONPACK          - rename 'missionpack'
  MISSIONPACK_CFLAGS   - custom CFLAGS for missionpack (default '-DMISSIONPACK')
  USE_OPENAL           - use OpenAL where available
  USE_OPENAL_DLOPEN    - link with OpenAL at runtime
  USE_CURL             - use libcurl for http/ftp download support
  USE_CURL_DLOPEN      - link with libcurl at runtime
  USE_CODEC_VORBIS     - enable Ogg Vorbis support
  USE_CODEC_OPUS       - enable Ogg Opus support
  USE_MUMBLE           - enable Mumble support
  USE_VOIP             - enable built-in VoIP support
  USE_FREETYPE         - enable FreeType support for rendering fonts
  USE_INTERNAL_LIBS    - build internal libraries instead of dynamically
                         linking against system libraries; this just sets
                         the default for USE_INTERNAL_ZLIB etc.
                         and USE_LOCAL_HEADERS
  USE_INTERNAL_ZLIB    - build and link against internal zlib
  USE_INTERNAL_JPEG    - build and link against internal JPEG library
  USE_INTERNAL_OGG     - build and link against internal ogg library
  USE_INTERNAL_OPUS    - build and link against internal opus/opusfile libraries
  USE_LOCAL_HEADERS    - use headers local to ioq3 instead of system ones
  DEBUG_CFLAGS         - C compiler flags to use for building debug version
  COPYDIR              - the target installation directory
  TEMPDIR              - specify user defined directory for temp files
```

The defaults for these variables differ depending on the target platform.
