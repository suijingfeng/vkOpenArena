
COMPILE_PLATFORM=$(shell uname | sed -e s/_.*//|tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e s/i.86/x86/ | sed -e 's/^arm.*/arm/')


ifndef BUILD_STANDALONE
  BUILD_STANDALONE = 1
endif
ifndef BUILD_CLIENT
  BUILD_CLIENT = 1
endif
ifndef BUILD_SERVER
  BUILD_SERVER = 1
endif

ifndef USE_RENDERER_DLOPEN
USE_RENDERER_DLOPEN=0
endif

ifdef BUILD_XF86DGA
  HAVE_XF86DGA=1
endif


BASEGAME=baseoa
VERSION=3.0.0a

#############################################################################
#
# If you require a different configuration from the defaults below, create a
# new file named "Makefile.local" in the same directory as this file and define
# your parameters there. This allows you to change configuration without
# causing problems with keeping up to date with the repository.
#
#############################################################################

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM


ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif

ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif


ifndef ARCH
ARCH=$(COMPILE_ARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(COMPILE_PLATFORM))
  CROSS_COMPILING=1
else
  CROSS_COMPILING=0

  ifneq ($(ARCH),$(COMPILE_ARCH))
    CROSS_COMPILING=1
  endif
endif
export CROSS_COMPILING

ifndef VERSION
VERSION=1.36
endif

ifndef CLIENTBIN
CLIENTBIN=openarena
endif

ifndef SERVERBIN
SERVERBIN=oa_ded
endif

ifndef BASEGAME
BASEGAME=baseq3
endif


ifndef COPYDIR
COPYDIR="/usr/local/games/OpenArena"
endif

ifndef COPYBINDIR
COPYBINDIR=$(COPYDIR)
endif

ifndef MOUNT_DIR
MOUNT_DIR=code
endif

ifndef BUILD_DIR
BUILD_DIR=build
endif

ifndef TEMPDIR
TEMPDIR=/tmp
endif

ifndef GENERATE_DEPENDENCIES
GENERATE_DEPENDENCIES=1
endif

ifndef USE_OPENAL
USE_OPENAL=1
endif


ifndef USE_OPENAL_DLOPEN
USE_OPENAL_DLOPEN=0
endif


ifndef USE_CODEC_VORBIS
USE_CODEC_VORBIS=1
endif

ifndef USE_CODEC_XMP
USE_CODEC_XMP=1
endif

ifndef USE_CODEC_OPUS
USE_CODEC_OPUS=0
endif

ifndef USE_MUMBLE
USE_MUMBLE=1
endif

ifndef USE_VOIP
USE_VOIP=1
endif

ifndef USE_FREETYPE
USE_FREETYPE=0
endif

ifndef USE_INTERNAL_LIBS
USE_INTERNAL_LIBS=0
endif


ifndef DEBUG_CFLAGS
DEBUG_CFLAGS= -g
endif



#############################################################################

BD=$(BUILD_DIR)/debug-$(PLATFORM)-$(ARCH)
BR=$(BUILD_DIR)/release-$(PLATFORM)-$(ARCH)
CDIR=$(MOUNT_DIR)/client
SDIR=$(MOUNT_DIR)/server
RCOMMONDIR=$(MOUNT_DIR)/renderercommon
RGL1DIR=$(MOUNT_DIR)/renderergl1
RGL2DIR=$(MOUNT_DIR)/renderergl2
ROADIR=$(MOUNT_DIR)/renderer_oa
CMDIR=$(MOUNT_DIR)/qcommon
SYSDIR=$(MOUNT_DIR)/sys
GDIR=$(MOUNT_DIR)/game
BLIBDIR=$(MOUNT_DIR)/botlib
NDIR=$(MOUNT_DIR)/null
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
VORBISDIR=$(MOUNT_DIR)/libvorbis-1.3.4
OPUSDIR=$(MOUNT_DIR)/opus-1.1
LOKISETUPDIR=misc/setup
NSISDIR=misc/nsis
LIBSDIR=$(MOUNT_DIR)/libs

bin_path=$(shell which $(1) 2> /dev/null)

# We won't need this if we only build the server
ifneq ($(BUILD_CLIENT),0)
  # set PKG_CONFIG_PATH to influence this, e.g.
  # PKG_CONFIG_PATH=/opt/cross/i386-mingw32msvc/lib/pkgconfig
  ifneq ($(call bin_path, pkg-config),)
    CURL_CFLAGS ?= $(shell pkg-config --silence-errors --cflags libcurl)
    CURL_LIBS ?= $(shell pkg-config --silence-errors --libs libcurl)
    OPENAL_CFLAGS ?= $(shell pkg-config --silence-errors --cflags openal)
    OPENAL_LIBS ?= $(shell pkg-config --silence-errors --libs openal)
    FREETYPE_CFLAGS ?= $(shell pkg-config --silence-errors --cflags freetype2)
  else
    # assume they're in the system default paths (no -I or -L needed)
    CURL_LIBS ?= -lcurl
    OPENAL_LIBS ?= -lopenal
  endif

endif



#############################################################################
# SETUP AND BUILD -- LINUX
#############################################################################

INSTALL=install
MKDIR=mkdir
EXTRA_FILES=
CLIENT_EXTRA_FILES=


ifneq (,$(findstring "$(PLATFORM)", "linux" "gnu_kfreebsd" "kfreebsd-gnu" "gnu"))

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes -pipe -DUSE_ICON -DARCH_STRING=\\\"$(ARCH)\\\"

  OPTIMIZEVM = -O3 -funroll-loops -fomit-frame-pointer
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math


  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM = -O3 -fomit-frame-pointer -funroll-loops -mmmx -msse2
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
  else
  ifeq ($(ARCH),x86)
    OPTIMIZEVM = -O3 -march=i586 -fomit-frame-pointer -funroll-loops
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
  else
  

endif

endif
  LDFLAGS = 
  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC -fvisibility=hidden
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-ldl -lm -lX11 -lGL
  RENDERER_LIBS = -lGL

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) $(OPENAL_LIBS)
    endif
  endif

    CLIENT_CFLAGS += $(CURL_CFLAGS)
	CLIENT_LIBS += $(CURL_LIBS)

  ifeq ($(USE_MUMBLE),1)
    CLIENT_LIBS += -lrt
  endif


else # ifeq Linux

#############################################################################
# SETUP AND BUILD -- GENERIC
#############################################################################
  BASE_CFLAGS=
  OPTIMIZE = -O3

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared

endif #Linux


ifndef CC
  CC=gcc
endif

ifndef RANLIB
  RANLIB=ranlib
endif


TARGETS =

ifndef FULLBINEXT
  FULLBINEXT=.$(ARCH)$(BINEXT)
endif

ifndef SHLIBNAME
  SHLIBNAME=$(ARCH).$(SHLIBEXT)
endif

ifneq ($(BUILD_SERVER),0)
  TARGETS += $(B)/$(SERVERBIN)$(FULLBINEXT)
endif

ifneq ($(BUILD_CLIENT),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
    TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT) $(B)/renderer_opengl1_$(SHLIBNAME) $(B)/renderer_openarena_$(SHLIBNAME) $(B)/renderer_opengl2_$(SHLIBNAME)
  else
    TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT) $(B)/$(CLIENTBIN)_opengl1$(FULLBINEXT) $(B)/$(CLIENTBIN)_opengl2$(FULLBINEXT)
  endif
endif



ifeq ($(USE_OPENAL),1)
  CLIENT_CFLAGS += -DUSE_OPENAL
  ifeq ($(USE_OPENAL_DLOPEN),1)
    CLIENT_CFLAGS += -DUSE_OPENAL_DLOPEN
  endif
endif

  CLIENT_CFLAGS += -DUSE_CURL


ifeq ($(USE_VOIP),1)
  CLIENT_CFLAGS += -DUSE_VOIP
  SERVER_CFLAGS += -DUSE_VOIP
  NEED_OPUS=1
endif

ifeq ($(USE_CODEC_XMP),1)
  CLIENT_CFLAGS += -DUSE_CODEC_XMP
  CLIENT_LIBS += -lxmp
  NEED_OGG=1
endif

ifeq ($(USE_CODEC_OPUS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_OPUS
  NEED_OPUS=1
endif

ifeq ($(NEED_OPUS),1)
    OPUS_CFLAGS ?= $(shell pkg-config --silence-errors --cflags opusfile opus || true)
    OPUS_LIBS ?= $(shell pkg-config --silence-errors --libs opusfile opus || echo -lopusfile -lopus)

  CLIENT_CFLAGS += $(OPUS_CFLAGS)
  CLIENT_LIBS += $(OPUS_LIBS)
  NEED_OGG=1
endif

ifeq ($(USE_CODEC_VORBIS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_VORBIS

    VORBIS_CFLAGS ?= $(shell pkg-config --silence-errors --cflags vorbisfile vorbis || true)
    VORBIS_LIBS ?= $(shell pkg-config --silence-errors --libs vorbisfile vorbis || echo -lvorbisfile -lvorbis)
  CLIENT_CFLAGS += $(VORBIS_CFLAGS)
  CLIENT_LIBS += $(VORBIS_LIBS)
  NEED_OGG=1
endif

ifeq ($(NEED_OGG),1)
	OGG_CFLAGS ?= $(shell pkg-config --silence-errors --cflags ogg || true)
	OGG_LIBS ?= $(shell pkg-config --silence-errors --libs ogg || echo -logg)
	CLIENT_CFLAGS += $(OGG_CFLAGS)
	CLIENT_LIBS += $(OGG_LIBS)
endif

ifeq ($(USE_RENDERER_DLOPEN),1)
  CLIENT_CFLAGS += -DUSE_RENDERER_DLOPEN
endif

ifeq ($(USE_MUMBLE),1)
  CLIENT_CFLAGS += -DUSE_MUMBLE
endif

  ZLIB_CFLAGS ?= $(shell pkg-config --silence-errors --cflags zlib || true)
  ZLIB_LIBS ?= $(shell pkg-config --silence-errors --libs zlib || echo -lz)

BASE_CFLAGS += $(ZLIB_CFLAGS)
LIBS += $(ZLIB_LIBS)

  # IJG libjpeg doesn't have pkg-config, but libjpeg-turbo uses libjpeg.pc;
  # we fall back to hard-coded answers if libjpeg.pc is unavailable
  JPEG_CFLAGS ?= $(shell pkg-config --silence-errors --cflags libjpeg || true)
  JPEG_LIBS ?= $(shell pkg-config --silence-errors --libs libjpeg || echo -ljpeg)
  BASE_CFLAGS += $(JPEG_CFLAGS)
  RENDERER_LIBS += $(JPEG_LIBS)

ifeq ($(USE_FREETYPE),1)
  FREETYPE_CFLAGS ?= $(shell pkg-config --silence-errors --cflags freetype2 || true)
  FREETYPE_LIBS ?= $(shell pkg-config --silence-errors --libs freetype2 || echo -lfreetype)

  BASE_CFLAGS += -DBUILD_FREETYPE $(FREETYPE_CFLAGS)
  RENDERER_LIBS += $(FREETYPE_LIBS)
endif


ifdef DEFAULT_BASEDIR
  BASE_CFLAGS += -DDEFAULT_BASEDIR=\\\"$(DEFAULT_BASEDIR)\\\"
endif


ifeq ($(BUILD_STANDALONE),1)
  BASE_CFLAGS += -DSTANDALONE
endif

ifeq ($(GENERATE_DEPENDENCIES),1)
  DEPEND_CFLAGS = -MMD
else
  DEPEND_CFLAGS =
endif

ifeq ($(NO_STRIP),1)
  STRIP_FLAG =
else
  STRIP_FLAG = -s
endif

BASE_CFLAGS += -DPRODUCT_VERSION=\\\"$(VERSION)\\\"
BASE_CFLAGS += -Wstrict-aliasing=2 -Wmissing-format-attribute
BASE_CFLAGS += -Werror-implicit-function-declaration

ifeq ($(V),1)
echo_cmd=@:
Q=
else
echo_cmd=@echo
Q=@
endif


define DO_CC
$(echo_cmd) "CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_REF_CC
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_REF_STR
$(echo_cmd) "REF_STR $<"
$(Q)rm -f $@
$(Q)echo "const char *fallbackShader_$(notdir $(basename $<)) =" >> $@
$(Q)cat $< | sed -e 's/^/\"/;s/$$/\\n\"/' | tr -d '\r' >> $@
$(Q)echo ";" >> $@
endef

define DO_BOT_CC
$(echo_cmd) "BOT_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(BOTCFLAGS) $(OPTIMIZE) -DBOTLIB -o $@ -c $<
endef

ifeq ($(GENERATE_DEPENDENCIES),1)
  DO_QVM_DEP=cat $(@:%.o=%.d) | sed -e 's/\.o/\.asm/g' >> $(@:%.o=%.d)
endif

define DO_SHLIB_CC
$(echo_cmd) "SHLIB_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef




define DO_AS
$(echo_cmd) "AS $<"
$(Q)$(CC) $(CFLAGS) $(OPTIMIZE) -x assembler-with-cpp -o $@ -c $<
endef

define DO_DED_CC
$(echo_cmd) "DED_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) -DDEDICATED $(CFLAGS) $(SERVER_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_WINDRES
$(echo_cmd) "WINDRES $<"
$(Q)$(WINDRES) -i $< -o $@
endef


#############################################################################
# MAIN TARGETS
#############################################################################

default: release
all: debug release

debug:
	@$(MAKE) targets B=$(BD) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="$(DEBUG_CFLAGS)" OPTIMIZEVM="$(DEBUG_CFLAGS)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V) LDFLAGS="-g $(LDFLAGS)"

release:
	@$(MAKE) targets B=$(BR) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="-DNDEBUG $(OPTIMIZE)" OPTIMIZEVM="-DNDEBUG $(OPTIMIZEVM)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

ifneq ($(call bin_path, tput),)
  TERM_COLUMNS=$(shell if c=`tput cols`; then echo $$(($$c-4)); else echo 76; fi)
else
  TERM_COLUMNS=76
endif

define ADD_COPY_TARGET
TARGETS += $2
$2: $1
	$(echo_cmd) "CP $$<"
	@cp $1 $2
endef

# These functions allow us to generate rules for copying a list of files
# into the base directory of the build; this is useful for bundling libs,
# README files or whatever else
define GENERATE_COPY_TARGETS
$(foreach FILE,$1, \
  $(eval $(call ADD_COPY_TARGET, \
    $(FILE), \
    $(addprefix $(B)/,$(notdir $(FILE))))))
endef

$(call GENERATE_COPY_TARGETS,$(EXTRA_FILES))

ifneq ($(BUILD_CLIENT),0)
  $(call GENERATE_COPY_TARGETS,$(CLIENT_EXTRA_FILES))
endif

NAKED_TARGETS=$(shell echo $(TARGETS) | sed -e "s!$(B)/!!g")

print_list=-@for i in $(1); \
     do \
             echo "    $$i"; \
     done

ifneq ($(call bin_path, fmt),)
  print_wrapped=@echo $(1) | fmt -w $(TERM_COLUMNS) | sed -e "s/^\(.*\)$$/    \1/"
else
  print_wrapped=$(print_list)
endif

# Create the build directories, check libraries and print out
# an informational message, then start building
targets: makedirs
	@echo ""
	@echo "  Building in $(B):"
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  ARCH: $(ARCH)"
	@echo "  VERSION: $(VERSION)"
	@echo "  COMPILE_PLATFORM: $(COMPILE_PLATFORM)"
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  CC: $(CC)"
ifeq ($(PLATFORM),mingw32)
	@echo "  WINDRES: $(WINDRES)"
endif
	@echo ""
	@echo "  CFLAGS:"
	$(call print_wrapped, $(CFLAGS) $(OPTIMIZE))
	@echo ""
	@echo "  CLIENT_CFLAGS:"
	$(call print_wrapped, $(CLIENT_CFLAGS))
	@echo ""
	@echo "  SERVER_CFLAGS:"
	$(call print_wrapped, $(SERVER_CFLAGS))
	@echo ""
	@echo "  LDFLAGS:"
	$(call print_wrapped, $(LDFLAGS))
	@echo ""
	@echo "  LIBS:"
	$(call print_wrapped, $(LIBS))
	@echo ""
	@echo "  CLIENT_LIBS:"
	$(call print_wrapped, $(CLIENT_LIBS))
	@echo ""
	@echo "  Output:"
	$(call print_list, $(NAKED_TARGETS))
	@echo ""
ifneq ($(TARGETS),)
  ifndef DEBUG_MAKEFILE
	@$(MAKE) $(TARGETS) $(B).zip V=$(V)
  endif
endif

$(B).zip: $(TARGETS)
ifeq ($(PLATFORM),darwin)
  ifdef ARCHIVE
	@("./make-macosx-app.sh" release $(ARCH); if [ "$$?" -eq 0 ] && [ -d "$(B)/ioquake3.app" ]; then rm -f $@; cd $(B) && zip --symlinks -r9 ../../$@ `find "ioquake3.app" -print | sed -e "s!$(B)/!!g"`; else rm -f $@; cd $(B) && zip -r9 ../../$@ $(NAKED_TARGETS); fi)
  endif
endif
ifneq ($(PLATFORM),darwin)
  ifdef ARCHIVE
	@rm -f $@
	@(cd $(B) && zip -r9 ../../$@ $(NAKED_TARGETS))
  endif
endif

makedirs:
	@if [ ! -d $(BUILD_DIR) ];then $(MKDIR) $(BUILD_DIR);fi
	@if [ ! -d $(B) ];then $(MKDIR) $(B);fi
	@if [ ! -d $(B)/client ];then $(MKDIR) $(B)/client;fi
	@if [ ! -d $(B)/client/opus ];then $(MKDIR) $(B)/client/opus;fi
	@if [ ! -d $(B)/client/vorbis ];then $(MKDIR) $(B)/client/vorbis;fi
	@if [ ! -d $(B)/renderergl1 ];then $(MKDIR) $(B)/renderergl1;fi
	@if [ ! -d $(B)/renderergl2 ];then $(MKDIR) $(B)/renderergl2;fi
	@if [ ! -d $(B)/renderergl2/glsl ];then $(MKDIR) $(B)/renderergl2/glsl;fi
	@if [ ! -d $(B)/renderer_oa ];then $(MKDIR) $(B)/renderer_oa;fi
	@if [ ! -d $(B)/ded ];then $(MKDIR) $(B)/ded;fi



#############################################################################
# CLIENT/SERVER
#############################################################################

Q3OBJ = \
  $(B)/client/cl_cgame.o \
  $(B)/client/cl_cin.o \
  $(B)/client/cl_console.o \
  $(B)/client/cl_input.o \
  $(B)/client/cl_keys.o \
  $(B)/client/cl_main.o \
  $(B)/client/cl_net_chan.o \
  $(B)/client/cl_parse.o \
  $(B)/client/cl_scrn.o \
  $(B)/client/cl_ui.o \
  $(B)/client/cl_avi.o \
  \
  $(B)/client/cm_load.o \
  $(B)/client/cm_patch.o \
  $(B)/client/cm_polylib.o \
  $(B)/client/cm_test.o \
  $(B)/client/cm_trace.o \
  \
  $(B)/client/cmd.o \
  $(B)/client/common.o \
  $(B)/client/cvar.o \
  $(B)/client/files.o \
  $(B)/client/md4.o \
  $(B)/client/md5.o \
  $(B)/client/msg.o \
  $(B)/client/net_chan.o \
  $(B)/client/net_ip.o \
  $(B)/client/huffman.o \
  \
  $(B)/client/snd_adpcm.o \
  $(B)/client/snd_dma.o \
  $(B)/client/snd_mem.o \
  $(B)/client/snd_mix.o \
  $(B)/client/snd_wavelet.o \
  \
  $(B)/client/snd_main.o \
  $(B)/client/snd_codec.o \
  $(B)/client/snd_codec_wav.o \
  $(B)/client/snd_codec_ogg.o \
  $(B)/client/snd_codec_opus.o \
  $(B)/client/snd_codec_xmp.o \
  \
  $(B)/client/qal.o \
  $(B)/client/snd_openal.o \
  \
  $(B)/client/cl_curl.o \
  \
  $(B)/client/sv_bot.o \
  $(B)/client/sv_ccmds.o \
  $(B)/client/sv_client.o \
  $(B)/client/sv_game.o \
  $(B)/client/sv_init.o \
  $(B)/client/sv_main.o \
  $(B)/client/sv_net_chan.o \
  $(B)/client/sv_snapshot.o \
  $(B)/client/sv_world.o \
  \
  $(B)/client/q_shared.o \
  \
  $(B)/client/unzip.o \
  $(B)/client/ioapi.o \
  $(B)/client/puff.o \
  $(B)/client/vm.o \
  $(B)/client/be_aas_bspq3.o \
  $(B)/client/be_aas_cluster.o \
  $(B)/client/be_aas_debug.o \
  $(B)/client/be_aas_entity.o \
  $(B)/client/be_aas_file.o \
  $(B)/client/be_aas_main.o \
  $(B)/client/be_aas_move.o \
  $(B)/client/be_aas_optimize.o \
  $(B)/client/be_aas_reach.o \
  $(B)/client/be_aas_route.o \
  $(B)/client/be_aas_routealt.o \
  $(B)/client/be_aas_sample.o \
  $(B)/client/be_ai_char.o \
  $(B)/client/be_ai_chat.o \
  $(B)/client/be_ai_gen.o \
  $(B)/client/be_ai_goal.o \
  $(B)/client/be_ai_move.o \
  $(B)/client/be_ai_weap.o \
  $(B)/client/be_ai_weight.o \
  $(B)/client/be_ea.o \
  $(B)/client/be_interface.o \
  $(B)/client/l_crc.o \
  $(B)/client/l_libvar.o \
  $(B)/client/l_log.o \
  $(B)/client/l_memory.o \
  $(B)/client/l_precomp.o \
  $(B)/client/l_script.o \
  $(B)/client/l_struct.o \
  \
  $(B)/client/signals_linux.o \
  $(B)/client/glimp_linux.o \
  $(B)/client/inputs_linux.o \
  $(B)/client/gamma_linux.o \
  $(B)/client/sound_linux.o	\
  $(B)/client/x11_randr.o \
  $(B)/client/x11_vidmode.o \
  $(B)/client/con_tty.o	\
  $(B)/client/con_log.o \
  \
  $(B)/client/sys_linux.o \
  $(B)/client/sys_loadlib.o \
  $(B)/client/main.o

ifeq ($(HAVE_XF86DGA), 1)
  Q3OBJ += $(B)/client/x11_dga.o
endif



Q3R2OBJ = \
  $(B)/renderergl2/tr_animation.o \
  $(B)/renderergl2/tr_backend.o \
  $(B)/renderergl2/tr_bsp.o \
  $(B)/renderergl2/tr_cmds.o \
  $(B)/renderergl2/tr_curve.o \
  $(B)/renderergl2/tr_dsa.o \
  $(B)/renderergl2/tr_fbo.o \
  $(B)/renderergl2/tr_flares.o \
  $(B)/renderergl2/tr_font.o \
  $(B)/renderergl2/tr_glsl.o \
  $(B)/renderergl2/tr_image.o \
  $(B)/renderergl2/tr_image_bmp.o \
  $(B)/renderergl2/tr_image_jpg.o \
  $(B)/renderergl2/tr_image_pcx.o \
  $(B)/renderergl2/tr_image_png.o \
  $(B)/renderergl2/tr_image_tga.o \
  $(B)/renderergl2/tr_image_dds.o \
  $(B)/renderergl2/tr_init.o \
  $(B)/renderergl2/tr_light.o \
  $(B)/renderergl2/tr_main.o \
  $(B)/renderergl2/tr_marks.o \
  $(B)/renderergl2/tr_mesh.o \
  $(B)/renderergl2/tr_model.o \
  $(B)/renderergl2/tr_model_iqm.o \
  $(B)/renderergl2/tr_postprocess.o \
  $(B)/renderergl2/tr_scene.o \
  $(B)/renderergl2/tr_shade.o \
  $(B)/renderergl2/tr_shade_calc.o \
  $(B)/renderergl2/tr_shader.o \
  $(B)/renderergl2/tr_shadows.o \
  $(B)/renderergl2/tr_sky.o \
  $(B)/renderergl2/tr_surface.o \
  $(B)/renderergl2/tr_vbo.o \
  $(B)/renderergl2/tr_world.o \
  $(B)/renderergl2/tr_shared.o


Q3R2STRINGOBJ = \
  $(B)/renderergl2/glsl/bokeh_fp.o \
  $(B)/renderergl2/glsl/bokeh_vp.o \
  $(B)/renderergl2/glsl/calclevels4x_fp.o \
  $(B)/renderergl2/glsl/calclevels4x_vp.o \
  $(B)/renderergl2/glsl/depthblur_fp.o \
  $(B)/renderergl2/glsl/depthblur_vp.o \
  $(B)/renderergl2/glsl/dlight_fp.o \
  $(B)/renderergl2/glsl/dlight_vp.o \
  $(B)/renderergl2/glsl/down4x_fp.o \
  $(B)/renderergl2/glsl/down4x_vp.o \
  $(B)/renderergl2/glsl/fogpass_fp.o \
  $(B)/renderergl2/glsl/fogpass_vp.o \
  $(B)/renderergl2/glsl/generic_fp.o \
  $(B)/renderergl2/glsl/generic_vp.o \
  $(B)/renderergl2/glsl/lightall_fp.o \
  $(B)/renderergl2/glsl/lightall_vp.o \
  $(B)/renderergl2/glsl/pshadow_fp.o \
  $(B)/renderergl2/glsl/pshadow_vp.o \
  $(B)/renderergl2/glsl/shadowfill_fp.o \
  $(B)/renderergl2/glsl/shadowfill_vp.o \
  $(B)/renderergl2/glsl/shadowmask_fp.o \
  $(B)/renderergl2/glsl/shadowmask_vp.o \
  $(B)/renderergl2/glsl/ssao_fp.o \
  $(B)/renderergl2/glsl/ssao_vp.o \
  $(B)/renderergl2/glsl/texturecolor_fp.o \
  $(B)/renderergl2/glsl/texturecolor_vp.o \
  $(B)/renderergl2/glsl/tonemap_fp.o \
  $(B)/renderergl2/glsl/tonemap_vp.o

Q3ROBJ = \
  $(B)/renderergl1/tr_animation.o \
  $(B)/renderergl1/tr_backend.o \
  $(B)/renderergl1/tr_bsp.o \
  $(B)/renderergl1/tr_cmds.o \
  $(B)/renderergl1/tr_curve.o \
  $(B)/renderergl1/tr_flares.o \
  $(B)/renderergl1/tr_font.o \
  $(B)/renderergl1/tr_image.o \
  $(B)/renderergl1/tr_image_bmp.o \
  $(B)/renderergl1/tr_image_jpg.o \
  $(B)/renderergl1/tr_image_pcx.o \
  $(B)/renderergl1/tr_image_png.o \
  $(B)/renderergl1/tr_image_tga.o \
  $(B)/renderergl1/tr_init.o \
  $(B)/renderergl1/tr_light.o \
  $(B)/renderergl1/tr_main.o \
  $(B)/renderergl1/tr_marks.o \
  $(B)/renderergl1/tr_mesh.o \
  $(B)/renderergl1/tr_model.o \
  $(B)/renderergl1/tr_model_iqm.o \
  $(B)/renderergl1/tr_scene.o \
  $(B)/renderergl1/tr_shade.o \
  $(B)/renderergl1/tr_shade_calc.o \
  $(B)/renderergl1/tr_shader.o \
  $(B)/renderergl1/tr_shadows.o \
  $(B)/renderergl1/tr_sky.o \
  $(B)/renderergl1/tr_surface.o \
  $(B)/renderergl1/tr_world.o \
  $(B)/renderergl1/tr_shared.o


Q3ROAOBJ = \
  $(B)/renderer_oa/tr_animation.o \
  $(B)/renderer_oa/tr_backend.o \
  $(B)/renderer_oa/tr_bsp.o \
  $(B)/renderer_oa/tr_cmds.o \
  $(B)/renderer_oa/tr_curve.o \
  $(B)/renderer_oa/tr_flares.o \
  $(B)/renderer_oa/tr_font.o \
  $(B)/renderer_oa/tr_image.o \
  $(B)/renderer_oa/tr_image_png.o \
  $(B)/renderer_oa/tr_image_jpg.o \
  $(B)/renderer_oa/tr_image_bmp.o \
  $(B)/renderer_oa/tr_image_tga.o \
  $(B)/renderer_oa/tr_image_pcx.o \
  $(B)/renderer_oa/tr_init.o \
  $(B)/renderer_oa/tr_light.o \
  $(B)/renderer_oa/tr_main.o \
  $(B)/renderer_oa/tr_marks.o \
  $(B)/renderer_oa/tr_model.o \
  $(B)/renderer_oa/tr_model_iqm.o \
  $(B)/renderer_oa/tr_scene.o \
  $(B)/renderer_oa/tr_shade.o \
  $(B)/renderer_oa/tr_shade_calc.o \
  $(B)/renderer_oa/tr_shader.o \
  $(B)/renderer_oa/tr_shadows.o \
  $(B)/renderer_oa/tr_sky.o \
  $(B)/renderer_oa/tr_surface.o \
  $(B)/renderer_oa/tr_world.o \
  $(B)/renderer_oa/tr_shared.o


ifneq ($(USE_RENDERER_DLOPEN), 0)
  Q3ROBJ += $(B)/renderergl1/q_shared.o
  
  Q3ROAOBJ += $(B)/renderer_oa/q_shared.o

  Q3R2OBJ += $(B)/renderergl2/q_shared.o 
endif


ifeq ($(ARCH),x86)
  Q3OBJ += \
    $(B)/client/snd_mixa.o \
    $(B)/client/matha.o \
    $(B)/client/snapvector.o 
endif
ifeq ($(ARCH),x86_64)
  Q3OBJ += \
    $(B)/client/snapvector.o
endif

    Q3OBJ += $(B)/client/vm_x86.o

ifeq ($(USE_MUMBLE),1)
  Q3OBJ += $(B)/client/libmumblelink.o
endif

ifneq ($(USE_RENDERER_DLOPEN),0)
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) $(NOTSHLIBLDFLAGS)\
		-o $@ $(Q3OBJ) $(JPGOBJ) $(CLIENT_LIBS) $(LIBS)

$(B)/renderer_opengl2_$(SHLIBNAME): $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_openarena_$(SHLIBNAME): $(Q3ROAOBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROAOBJ) $(JPGOBJ) $(THREAD_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_opengl1_$(SHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) $(THREAD_LIBS) $(RENDERER_LIBS) $(LIBS)


else

$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(Q3ROAOBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3ROAOBJ) $(JPGOBJ) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/$(CLIENTBIN)_opengl1$(FULLBINEXT): $(Q3OBJ) $(Q3ROBJ)  $(JPGOBJ) 
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) $(NOTSHLIBLDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3ROBJ) $(JPGOBJ) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)
										
$(B)/$(CLIENTBIN)_opengl2$(FULLBINEXT): $(Q3OBJ) $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) $(NOTSHLIBLDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3R2OBJ) $(Q3R2STRINGOBJ) $(JPGOBJ) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)


endif

#	cp $< $@
#	$(RANLIB) $@



#############################################################################
# DEDICATED SERVER
#############################################################################

Q3DOBJ = \
  $(B)/ded/sv_bot.o \
  $(B)/ded/sv_client.o \
  $(B)/ded/sv_ccmds.o \
  $(B)/ded/sv_game.o \
  $(B)/ded/sv_init.o \
  $(B)/ded/sv_main.o \
  $(B)/ded/sv_net_chan.o \
  $(B)/ded/sv_snapshot.o \
  $(B)/ded/sv_world.o \
  \
  $(B)/ded/cm_load.o \
  $(B)/ded/cm_patch.o \
  $(B)/ded/cm_polylib.o \
  $(B)/ded/cm_test.o \
  $(B)/ded/cm_trace.o \
  $(B)/ded/cmd.o \
  $(B)/ded/common.o \
  $(B)/ded/cvar.o \
  $(B)/ded/files.o \
  $(B)/ded/md4.o \
  $(B)/ded/msg.o \
  $(B)/ded/net_chan.o \
  $(B)/ded/net_ip.o \
  $(B)/ded/huffman.o \
  \
  $(B)/ded/q_shared.o \
  \
  $(B)/ded/unzip.o \
  $(B)/ded/ioapi.o \
  $(B)/ded/vm.o \
  $(B)/ded/sys_loadlib.o \
  \
  $(B)/ded/be_aas_bspq3.o \
  $(B)/ded/be_aas_cluster.o \
  $(B)/ded/be_aas_debug.o \
  $(B)/ded/be_aas_entity.o \
  $(B)/ded/be_aas_file.o \
  $(B)/ded/be_aas_main.o \
  $(B)/ded/be_aas_move.o \
  $(B)/ded/be_aas_optimize.o \
  $(B)/ded/be_aas_reach.o \
  $(B)/ded/be_aas_route.o \
  $(B)/ded/be_aas_routealt.o \
  $(B)/ded/be_aas_sample.o \
  $(B)/ded/be_ai_char.o \
  $(B)/ded/be_ai_chat.o \
  $(B)/ded/be_ai_gen.o \
  $(B)/ded/be_ai_goal.o \
  $(B)/ded/be_ai_move.o \
  $(B)/ded/be_ai_weap.o \
  $(B)/ded/be_ai_weight.o \
  $(B)/ded/be_ea.o \
  $(B)/ded/be_interface.o \
  $(B)/ded/l_crc.o \
  $(B)/ded/l_libvar.o \
  $(B)/ded/l_log.o \
  $(B)/ded/l_memory.o \
  $(B)/ded/l_precomp.o \
  $(B)/ded/l_script.o \
  $(B)/ded/l_struct.o \
  \
  $(B)/ded/null_client.o \
  $(B)/ded/null_snddma.o \
  \
  $(B)/ded/con_log.o \
  $(B)/ded/main.o

ifeq ($(ARCH),x86)
  Q3DOBJ += \
    $(B)/ded/snd_mixa.o \
    $(B)/ded/matha.o \
    $(B)/ded/snapvector.o
endif
ifeq ($(ARCH),x86_64)
  Q3DOBJ += \
    $(B)/ded/snapvector.o
endif

  Q3DOBJ += $(B)/ded/vm_x86.o
  
  Q3DOBJ += \
    $(B)/ded/sys_linux.o \
    $(B)/ded/con_tty.o	\
	$(B)/ded/signals_linux.o



$(B)/$(SERVERBIN)$(FULLBINEXT): $(Q3DOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(Q3DOBJ) $(LIBS)






#############################################################################
## CLIENT/SERVER RULES
#############################################################################


$(B)/client/%.o: $(CDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(SDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(CMDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)


$(B)/client/vorbis/%.o: $(VORBISDIR)/lib/%.c
	$(DO_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/src/%.c
	$(DO_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/celt/%.c
	$(DO_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/silk/%.c
	$(DO_CC)

$(B)/client/opus/%.o: $(OPUSDIR)/silk/float/%.c
	$(DO_CC)


$(B)/client/%.o: $(SYSDIR)/%.c
	$(DO_CC)

$(B)/client/%.o: $(SYSDIR)/%.rc
	$(DO_WINDRES)



$(B)/renderer_oa/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(B)/renderer_oa/%.o: $(ROADIR)/%.c
	$(DO_REF_CC)

$(B)/renderer_oa/%.o: $(RCOMMONDIR)/%.c
	$(DO_REF_CC)


$(B)/renderergl1/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/%.o: $(RGL1DIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl1/%.o: $(RCOMMONDIR)/%.c
	$(DO_REF_CC)


$(B)/renderergl2/glsl/%.c: $(RGL2DIR)/glsl/%.glsl
	$(DO_REF_STR)

$(B)/renderergl2/glsl/%.o: $(B)/renderergl2/glsl/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(RGL2DIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(B)/renderergl2/%.o: $(RCOMMONDIR)/%.c
	$(DO_REF_CC)



$(B)/ded/%.o: $(CMDIR)/%.c
	$(DO_CC)  

$(B)/ded/%.o: $(SDIR)/%.c
	$(DO_DED_CC)

$(B)/ded/%.o: $(CMDIR)/%.c
	$(DO_DED_CC)


$(B)/ded/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)

$(B)/ded/%.o: $(SYSDIR)/%.c
	$(DO_DED_CC)

$(B)/ded/%.o: $(SYSDIR)/%.rc
	$(DO_WINDRES)

$(B)/ded/%.o: $(NDIR)/%.c
	$(DO_DED_CC)




#############################################################################
# MISC
#############################################################################

OBJ = $(Q3OBJ) $(Q3ROBJ) $(Q3R2OBJ) $(Q3ROAOBJ)  $(Q3DOBJ) $(JPGOBJ) \
  $(MPGOBJ) $(Q3GOBJ) $(Q3CGOBJ) $(MPCGOBJ) $(Q3UIOBJ) $(MPUIOBJ) \
  $(MPGVMOBJ) $(Q3GVMOBJ) $(Q3CGVMOBJ) $(MPCGVMOBJ) $(Q3UIVMOBJ) $(MPUIVMOBJ)
STRINGOBJ = $(Q3R2STRINGOBJ)


copyfiles: release
	@if [ ! -d $(COPYDIR)/$(BASEGAME) ]; then echo "You need to set COPYDIR to where your Quake3 data is!"; fi


ifneq ($(BUILD_CLIENT),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)$(FULLBINEXT)
  ifneq ($(USE_RENDERER_DLOPEN),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl1_$(SHLIBNAME) $(COPYBINDIR)/renderer_opengl1_$(SHLIBNAME)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_openarena_$(SHLIBNAME) $(COPYBINDIR)/renderer_openarena_$(SHLIBNAME)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl2_$(SHLIBNAME) $(COPYBINDIR)/renderer_opengl2_$(SHLIBNAME)
  else
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)_opengl1$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)_opengl1$(FULLBINEXT)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)_opengl2$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)_opengl2$(FULLBINEXT)
  endif
endif

ifneq ($(BUILD_SERVER),0)
	@if [ -f $(BR)/$(SERVERBIN)$(FULLBINEXT) ]; then \
		$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(SERVERBIN)$(FULLBINEXT) $(COPYBINDIR)/$(SERVERBIN)$(FULLBINEXT); \
	fi
endif



clean: clean-debug clean-release

clean-debug:
	@$(MAKE) clean2 B=$(BD)

clean-release:
	@$(MAKE) clean2 B=$(BR)

clean2:
	@echo "CLEAN $(B)"
	@rm -f $(OBJ)
	@rm -f $(OBJ_D_FILES)
	@rm -f $(STRINGOBJ)
	@rm -f $(TARGETS)


distclean: clean
	@rm -rf $(BUILD_DIR)

installer: release
	@$(MAKE) VERSION=$(VERSION) -C $(LOKISETUPDIR) V=$(V)


dist:
	git archive --format zip --output $(CLIENTBIN)-$(VERSION).zip HEAD

#############################################################################
# DEPENDENCIES
#############################################################################

ifneq ($(B),)
  OBJ_D_FILES=$(filter %.d,$(OBJ:%.o=%.d))
endif

.PHONY: all clean clean2 clean-debug clean-release copyfiles \
	debug default dist distclean installer makedirs \
	release targets

# If the target name contains "clean", don't do a parallel build
ifneq ($(findstring clean, $(MAKECMDGOALS)),)
.NOTPARALLEL:
endif
