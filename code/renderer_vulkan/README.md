# vulkan backend info
* codes in this dir is "borrow" from https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition, I convert cpp to c so that it can compile.

* vulkan forder is copied form vulkan sdk, not all items in it is used, it need clean.

* I am a naive programmer, need help, doc and instructions.


# Rendering

## General setup. 

Single command buffer that records all the commands. Single render pass which specifies color and depth-stencil attachment. 
Stencil buffer is used to render Q3's stencil shadows (cg\_shadows=2).

## Geometry. 
Quake 3 renderer prepares geometry data for each draw call in tess.xyz and tess.indexes arrays.
OpenGL backend calls qglDrawElements to feed this geometry to the GPU. 
Vulkan backend appends this data to geometry buffers that are bound to host visible memory chunk.
At the end of the frame when command buffer is submitted to the queue the geometry buffers contain all the geometry data to render the frame.
Typically up to 500Kb of vertex data is copied to the vertex buffer and up to 100Kb of index data is copied to the index buffer per frame.

## Descriptor sets.
For each image used by the renderer separate descriptor set is created.
Each descriptor set contains single descriptor (combined image sampler).
For each draw call either one or two (if lightmap is available) descriptor sets are bound.
Descriptor sets are updated only once during initialization.
There are no descriptor set updates during frame.

## Per-primitive uniform data.
Vulkan guarantees that minimum size of push constants range is at least 128 bytes.
To render ordinary view we use 64 bytes to specify mvp transform.
For portaled/mirrored views additional 64 byte are used to specify eye transform and clipping plane.

Pipeline layout. 2 sets + 128 bytes push constant range.

## Pipelines. 
Standard pipelines are created when renderer starts. 
They are used for skybox rendering, fog/dynamic light effects, shadow volumes and various debug features.
Map specific pipelines are created as part of parsing Q3 shaders and are created during map load.
For each Q3 shader we create three pipelines: one pipeline to render regular view and two additional pipelines for portal and mirror views.

## Shaders.
Emulate corresponding fixed-function functionality. 
Vertex shaders are boring with the only thing to mention that for portaled/mirrored views
we additionally compute distance to the clipping plane. 
Fragment shaders do one or two texture lookups and modulate the results by the color.

## Draw calls. 
vkCmdDrawIndexed is used to draw geometry in most cases. Additionally there are few debug features that use vkCmdDraw to convey unindexed vertexes.

## Code
vk.h provides interface that brings Vulkan support to Q3 renderer. 
The interface is quite concise and consists of a dozen of functions that can be divided into 3 categories: 
initialization functions, resource management functions and rendering setup functions.

### Initialization:

* vk\_initialize : initialize Vulkan backend
* vk\_shutdown : shutdown Vulkan backend

### Resource management:

* images: vk\_create\_image/vk\_upload\_image\_data

* descriptor sets: vk\_update\_descriptor\_set

* samplers: vk\_find\_sampler

* pipelines: vk\_find\_pipeline

### Rendering setup:

* vk\_clear\_attachments : clears framebuffer's attachments.

* vk\_bind\_geometry : is called when we start drawing new geometry.

* vk\_shade\_geometry : is called to shade geometry specified with vk\_bind\_geometry. Can be called multiple times for Q3's multi-stage shaders.

* vk\_begin\_frame/vk\_end\_frame : frame setup.

* vk\_read\_pixels : takes a screenshot.


### about turn the intensity/gamma of the drawing wondow

* use r\_gamma in the pulldown window, which nonlinearly correct the image before the uploading to GPU.
`\r_gamma 1.5` then `vid_restart`

* you can also use r\_intensity which turn the intensity linearly.
```
# 1.5 ~ 2.0 give acceptable result
$ \r_intensity 1.8
$ \vid_restart
```

* but why, because original gamma setting program turn the light by setting entire destop window.
which works on newer computer on the market 
but not works on some machine. it is buggy and embarrasing when program abnormal quit or stall.

### new cmd

* pipelineList: list the pipeline we have created;
* gpuMem: image memmory allocated on GPU;
* printOR: print the value of backend.or;

* pipelineList: list the number of pipelines created (about 100, seem to much ?)

* displayResoList: list of the display resolution you monitor supported

* printDeviceExtensions: list the device extensions supported on you device/GPU
* printInstanceExtensions: list the instance extensions supported on you device/GPU



For example:
```
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

* printImgHashTable: print the image hash table usage, which also list the image creaeted and is size info etc.
```
$ \printImgHashTable
-----------------------------------------------------
[7] mipLevels: 7	size: 128x128	gfx/fx/decals/bulletmetal.tga
[9] mipLevels: 7	size: 64x64	icons/icona_machinegun.tga
[9] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz6.tga
[11] mipLevels: 9	size: 512x512	models/weapons2/railgun/skin.tga
[12] mipLevels: 1	size: 128x128	*lightmap9
[12] mipLevels: 1	size: 128x128	*lightmap1
[19] mipLevels: 7	size: 128x128	models/powerups/instant/quadlite.tga
[20] mipLevels: 7	size: 64x64	icons/icona_railgun.tga
[23] mipLevels: 7	size: 128x128	models/weapons2/railgun/scrolly.tga
[24] mipLevels: 7	size: 128x128	sprites/multwake7.tga
[24] mipLevels: 8	size: 256x256	textures/clown/text4.tga
[26] mipLevels: 9	size: 512x512	textures/base_wall/concrete_ow.tga
[27] mipLevels: 1	size: 128x128	models/players/tony/icon_default.tga
[28] mipLevels: 8	size: 256x256	textures/base_floor/clang_floor
[33] mipLevels: 8	size: 256x256	textures/gothic_wall/goldbrick
[42] mipLevels: 7	size: 128x128	gfx/fx/flares/wide.tga
[45] mipLevels: 8	size: 128x128	gfx/2d/crosshairi.tga
[47] mipLevels: 8	size: 256x256	models/powerups/armor/redarmor.tga
[53] mipLevels: 7	size: 128x128	textures/flares/newflare.tga
[60] mipLevels: 9	size: 128x512	textures/base_light/baslt4_1.blend.jpg
[60] mipLevels: 9	size: 128x512	textures/base_light/baslt4_1.jpg
[60] mipLevels: 1	size: 128x128	*lightmap10
[61] mipLevels: 7	size: 64x64	icons/iconr_red.tga
[61] mipLevels: 1	size: 128x128	*lightmap81
[62] mipLevels: 1	size: 128x128	*lightmap72
[63] mipLevels: 1	size: 128x128	models/players/sarge/icon_default.tga
[63] mipLevels: 1	size: 128x128	*lightmap63
[64] mipLevels: 8	size: 128x128	menu/medals/medal_defend.tga
[64] mipLevels: 1	size: 128x128	*lightmap54
[65] mipLevels: 1	size: 128x128	*lightmap45
[66] mipLevels: 7	size: 64x64	icons/iconw_grenade.tga
[66] mipLevels: 1	size: 128x128	*lightmap36
[67] mipLevels: 1	size: 128x128	*lightmap27
[68] mipLevels: 1	size: 128x128	*lightmap18
[70] mipLevels: 8	size: 256x256	textures/base_floor/floor3_3dark_ow
[72] mipLevels: 7	size: 128x128	models/powerups/shield/shieldwire.tga
[81] mipLevels: 6	size: 64x64	models/weapons2/shells/mgunshell.tga
[85] mipLevels: 8	size: 256x256	textures/pulchr/telepulchr.tga
[88] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0015.tga
[88] mipLevels: 7	size: 128x128	sprites/le/smoke3.tga
[88] mipLevels: 7	size: 64x128	textures/base_wall/bluemetalsupport2fline.tga
[89] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0006.tga
[95] mipLevels: 9	size: 512x128	textures/base_light/border7_ceil50.tga
[97] mipLevels: 6	size: 64x64	icons/noammo
[109] mipLevels: 1	size: 128x128	models/players/ayumi/icon_blue.tga
[111] mipLevels: 7	size: 128x128	models/weaphits/bullet_0006.tga
[114] mipLevels: 6	size: 64x64	textures/base_light/light1blue.blend.jpg
[114] mipLevels: 6	size: 64x64	textures/base_light/light1blue.jpg
[119] mipLevels: 7	size: 128x128	gfx/2d/net.tga
[119] mipLevels: 7	size: 64x64	gfx/2d/numbers/minus_32b.tga
[119] mipLevels: 7	size: 64x64	gfx/2d/numbers/one_32b.tga
[120] mipLevels: 8	size: 256x128	textures/oafx/lbeam8.tga
[124] mipLevels: 8	size: 128x128	gfx/2d/crosshairb.tga
[125] mipLevels: 8	size: 64x256	textures/gothic_trim/metalsupport4j
[127] mipLevels: 8	size: 256x256	models/gibs/gibmeatspec.tga
[134] mipLevels: 8	size: 256x256	textures/pulchr/twirl.tga
[137] mipLevels: 8	size: 256x256	models/players/tony/head.tga
[138] mipLevels: 9	size: 512x512	models/players/tony/suit.tga
[140] mipLevels: 6	size: 64x64	models/weapons2/machinegun/sight.tga
[140] mipLevels: 7	size: 128x128	textures/base_trim/spidertrim
[140] mipLevels: 1	size: 128x128	*lightmap2
[141] mipLevels: 6	size: 32x32	menu/art/skill1
[142] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_3.tga
[143] mipLevels: 6	size: 64x64	gfx/misc/smokepuff3.tga
[144] mipLevels: 9	size: 512x512	models/weaphits/spark.tga
[146] mipLevels: 6	size: 64x64	textures/effects/smallhelth_spec
[150] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz7.tga
[151] mipLevels: 8	size: 256x256	textures/base_trim/pewter_shiney.jpg
[152] mipLevels: 8	size: 256x256	models/powerups/shield/juiced-drip.tga
[153] mipLevels: 6	size: 64x64	textures/effects/envmapdimb.tga
[155] mipLevels: 7	size: 64x64	icons/iconw_gauntlet.tga
[159] mipLevels: 7	size: 128x128	sprites/multwake8.tga
[166] mipLevels: 6	size: 64x64	gfx/misc/smokepuffragepro.tga
[166] mipLevels: 7	size: 128x128	gfx/damage/blood_screen.tga
[167] mipLevels: 9	size: 512x512	models/players/penguin/skin.tga
[171] mipLevels: 7	size: 128x128	models/players/ayumi/jet/jet1.tga
[177] mipLevels: 9	size: 512x512	models/players/ayumi/shirt.tga
[177] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0000.tga
[177] mipLevels: 7	size: 128x128	textures/pulchr/airwalk-env.tga
[180] mipLevels: 8	size: 128x128	gfx/2d/crosshairj.tga
[186] mipLevels: 5	size: 57x57	textures/oafx/suitshell
[187] mipLevels: 8	size: 128x128	menu/medals/medal_excellent.tga
[188] mipLevels: 1	size: 128x128	*lightmap20
[189] mipLevels: 1	size: 128x128	*lightmap11
[191] mipLevels: 1	size: 128x128	*lightmap73
[192] mipLevels: 1	size: 128x128	*lightmap64
[193] mipLevels: 6	size: 64x64	sprites/le/blood3.tga
[193] mipLevels: 1	size: 128x128	*lightmap55
[194] mipLevels: 1	size: 128x128	*lightmap46
[195] mipLevels: 1	size: 128x128	*lightmap37
[196] mipLevels: 1	size: 128x128	*lightmap28
[197] mipLevels: 7	size: 64x64	icons/iconw_lightning.tga
[197] mipLevels: 1	size: 128x128	*lightmap19
[200] mipLevels: 7	size: 128x128	textures/base_wall/steed1ge
[202] mipLevels: 8	size: 256x128	textures/base_light/scrolllight.tga
[216] mipLevels: 9	size: 512x512	textures/gothic_trim/km_arena1tower1
[223] mipLevels: 9	size: 512x512	models/weapons2/lightning/skinlightning.tga
[223] mipLevels: 7	size: 128x128	sprites/le/smoke4.tga
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[223] mipLevels: 1	size: 16x16	*scratch
[227] mipLevels: 1	size: 256x32	*fog
[232] mipLevels: 1	size: 256x256	gfx/2d/bigchars.tga
[233] mipLevels: 6	size: 64x64	sprites/le/spark2.tga
[234] mipLevels: 7	size: 128x128	textures/flares/flarey.tga
[236] mipLevels: 1	size: 128x32	menu/tab/ping.tga
[236] mipLevels: 8	size: 256x256	textures/base_wall/comp3b.tga
[238] mipLevels: 7	size: 128x128	sprites/multwake1.tga
[239] mipLevels: 7	size: 64x64	gfx/2d/numbers/zero_32b.tga
[240] mipLevels: 6	size: 64x64	gfx/fx/spec/spots.tga
[242] mipLevels: 8	size: 256x256	models/gibs/f_veins.tga
[242] mipLevels: 8	size: 256x256	textures/base_trim/tin.tga
[243] mipLevels: 8	size: 256x256	textures/oa/fiar2.tga
[244] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0016.tga
[245] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0007.tga
[249] mipLevels: 7	size: 64x64	icons/iconh_mega.tga
[254] mipLevels: 6	size: 64x64	sprites/le/glaw.tga
[255] mipLevels: 9	size: 512x512	models/weapons2/rocketl/skin.tga
[256] mipLevels: 7	size: 128x128	models/weaphits/bullet_0007.tga
[257] mipLevels: 8	size: 256x256	gfx/fx/detail/d_conc.tga
[259] mipLevels: 8	size: 128x128	gfx/2d/crosshairc.tga
[265] mipLevels: 7	size: 128x128	models/weaphits/bullet_0000.tga
[268] mipLevels: 6	size: 64x64	gfx/fx/spec/specenv.tga
[268] mipLevels: 1	size: 128x128	*lightmap3
[271] mipLevels: 6	size: 64x64	textures/effects/envmapligh.tga
[272] mipLevels: 7	size: 128x128	models/powerups/ammo/lighammo2.tga
[274] mipLevels: 6	size: 32x32	menu/art/skill2
[280] mipLevels: 6	size: 64x64	sprites/le/blast.tga
[280] mipLevels: 1	size: 16x32	menu/art/sliderbutt_0
[290] mipLevels: 7	size: 128x128	textures/base_trim/pewter
[291] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_4.tga
[291] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz8.tga
[292] mipLevels: 7	size: 64x64	gfx/2d/numbers/two_32b.tga
[293] mipLevels: 7	size: 64x64	icons/iconr_shard.tga
[294] mipLevels: 8	size: 256x384	textures/base_wall/bms2ftv_glow.tga
[297] mipLevels: 1	size: 256x256	menu/art/font1_prop_glo.tga
[300] mipLevels: 7	size: 64x64	icons/icona_grenade.tga
[300] mipLevels: 7	size: 128x128	models/weaphits/proxbomb_b
[300] mipLevels: 7	size: 128x128	models/weaphits/proxbomb_b.tga
[309] mipLevels: 8	size: 256x256	textures/base_floor/skylight_spec.tga
[313] mipLevels: 7	size: 64x64	icons/iconh_yellow.tga
[314] mipLevels: 7	size: 128x32	gfx/2d/colorbar.tga
[316] mipLevels: 1	size: 128x128	*lightmap30
[317] mipLevels: 1	size: 128x128	*lightmap21
[318] mipLevels: 7	size: 128x128	models/players/ayumi/jet/jet2.tga
[318] mipLevels: 1	size: 128x128	*lightmap12
[320] mipLevels: 1	size: 128x128	*lightmap74
[321] mipLevels: 1	size: 128x128	*lightmap65
[322] mipLevels: 1	size: 128x128	*lightmap56
[323] mipLevels: 1	size: 128x128	*lightmap47
[324] mipLevels: 8	size: 256x256	textures/effects/tinfx2.tga
[324] mipLevels: 1	size: 128x128	*lightmap38
[325] mipLevels: 7	size: 128x128	gfx/damage/hole_lg_mrk.tga
[325] mipLevels: 1	size: 128x128	*lightmap29
[328] mipLevels: 8	size: 256x256	gfx/damage/bulletmult.tga
[328] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz1.tga
[329] mipLevels: 7	size: 64x64	gfx/2d/numbers/nine_32b.tga
[331] mipLevels: 1	size: 128x16	menu/art/slider2
[332] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0010.tga
[333] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0001.tga
[334] mipLevels: 9	size: 256x256	textures/sfx/detail.tga
[338] mipLevels: 8	size: 256x256	textures/sfx/xian_dm3padwallglow.jpg
[339] mipLevels: 1	size: 32x32	menu/art/switch_on
[340] mipLevels: 1	size: 64x64	gfx/2d/defer.tga
[345] mipLevels: 7	size: 64x64	gfx/2d/numbers/five_32b.tga
[345] mipLevels: 8	size: 256x128	textures/base_wall/steed1gf
[349] mipLevels: 6	size: 64x64	textures/base_light/light1red.blend.jpg
[349] mipLevels: 6	size: 64x64	textures/base_light/light1red.jpg
[350] mipLevels: 6	size: 64x64	models/weapons2/shells/sgunshell_2.tga
[355] mipLevels: 8	size: 256x256	textures/base_wall/concrete.tga
[356] mipLevels: 1	size: 128x32	menu/tab/time.tga
[357] mipLevels: 9	size: 512x512	models/weapons2/grenadel/newgren.tga
[360] mipLevels: 7	size: 128x128	textures/base_floor/clang_floor2
[360] mipLevels: 8	size: 256x256	textures/sfx/xian_dm3padwall.jpg
[363] mipLevels: 9	size: 512x512	textures/pulchr/pul1duelsky-a.tga
[368] mipLevels: 7	size: 128x16	textures/base_trim/pewterstep
[373] mipLevels: 7	size: 128x128	sprites/multwake2.tga
[389] mipLevels: 7	size: 128x128	models/weapons2/railgun/f_railgun2.tga
[391] mipLevels: 7	size: 128x128	models/powerups/ammo/grenammo2.tga
[394] mipLevels: 8	size: 128x128	gfx/2d/crosshaird.tga
[396] mipLevels: 1	size: 128x128	*lightmap4
[397] mipLevels: 6	size: 64x64	models/weapons/nailgun/nailtrail.tga
[399] mipLevels: 6	size: 64x64	textures/effects/envmapred.tga
[400] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0017.tga
[400] mipLevels: 8	size: 256x256	gfx/fx/spec/gunmetal.tga
[400] mipLevels: 8	size: 256x256	textures/gothic_trim/pitted_rust3_black.tga
[400] mipLevels: 6	size: 64x64	textures/base_support/flat1_1
[401] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0008.tga
[401] mipLevels: 7	size: 128x128	models/powerups/instant/quadlite2.tga
[402] mipLevels: 8	size: 256x256	models/players/ayumi/hair.tga
[407] mipLevels: 6	size: 32x32	menu/art/skill3
[410] mipLevels: 7	size: 128x128	models/weaphits/bullet_0001.tga
[411] mipLevels: 8	size: 256x256	gfx/2d/bloodspew.tga
[412] mipLevels: 7	size: 128x32	models/powerups/gdstrip.tga
[413] mipLevels: 1	size: 128x32	menu/tab/score.tga
[414] mipLevels: 7	size: 64x64	gfx/2d/numbers/three_32b.tga
[419] mipLevels: 1	size: 16x32	menu/art/sliderbutt_1
[425] mipLevels: 9	size: 512x256	textures/sfx/logo256.tga
[431] mipLevels: 7	size: 64x64	icons/icona_lightning.tga
[440] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_5.tga
[440] mipLevels: 8	size: 256x256	textures/pulchr/back.tga
[444] mipLevels: 1	size: 128x128	*lightmap40
[445] mipLevels: 1	size: 128x128	*lightmap31
[446] mipLevels: 1	size: 128x128	*lightmap22
[447] mipLevels: 1	size: 128x128	*lightmap13
[449] mipLevels: 1	size: 128x128	*lightmap75
[450] mipLevels: 1	size: 128x128	*lightmap66
[451] mipLevels: 7	size: 128x128	gfx/fx/detail/d_met2.tga
[451] mipLevels: 1	size: 128x128	*lightmap57
[452] mipLevels: 10	size: 512x512	textures/pulchr/five-steps-ahead-pindel.tga
[452] mipLevels: 1	size: 128x128	*lightmap48
[453] mipLevels: 6	size: 64x64	textures/base_light/light1.blend.jpg
[453] mipLevels: 6	size: 64x64	textures/base_light/light1.jpg
[453] mipLevels: 1	size: 128x128	*lightmap39
[454] mipLevels: 8	size: 256x128	textures/oafx/lbeam3.tga
[463] mipLevels: 7	size: 128x128	textures/base_support/metal14_1
[465] mipLevels: 1	size: 8x8	*identityLight
[467] mipLevels: 1	size: 256x256	menu/art/font1_prop.tga
[469] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz2.tga
[471] mipLevels: 9	size: 512x512	textures/base_floor/rusty_pentagrate.tga
[476] mipLevels: 6	size: 64x64	sprites/le/blood.tga
[479] mipLevels: 7	size: 128x128	textures/flares/wide.tga
[482] mipLevels: 7	size: 128x32	models/powerups/arstrip.tga
[482] mipLevels: 8	size: 256x256	textures/sfx/proto_zzztblu3.jpg
[487] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0020.tga
[488] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0011.tga
[489] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0002.tga
[490] mipLevels: 7	size: 128x128	textures/base_wall/comp3text.jpg
[490] mipLevels: 1	size: 64x256	textures/pulchr/beam-teleporter.tga
[493] mipLevels: 8	size: 256x256	textures/liquids/slime8.jpg
[495] mipLevels: 9	size: 64x512	models/gibs/splurt.tga
[497] mipLevels: 6	size: 64x64	textures/effects/envmapyel.tga
[499] mipLevels: 9	size: 256x512	textures/ctf/test2
[500] mipLevels: 9	size: 512x512	models/players/grism/enkiskin.tga
[508] mipLevels: 7	size: 128x128	sprites/multwake3.tga
[510] mipLevels: 9	size: 512x512	textures/pulchr/pul1duelsky-b.tga
[514] mipLevels: 8	size: 256x256	textures/base_trim/dirty_pewter
[514] mipLevels: 8	size: 256x256	textures/base_trim/dirty_pewter.tga
[516] mipLevels: 7	size: 64x64	gfx/2d/numbers/six_32b.tga
[518] mipLevels: 8	size: 256x256	textures/base_wall/patch10.jpg
[518] mipLevels: 8	size: 256x256	textures/base_wall/patch10
[522] mipLevels: 6	size: 64x64	models/weapons2/shells/mgunshell_2.tga
[524] mipLevels: 1	size: 128x128	*lightmap5
[527] mipLevels: 8	size: 256x256	models/powerups/ammo/ammolights.tga
[529] mipLevels: 8	size: 128x128	gfx/2d/crosshaire.tga
[533] mipLevels: 7	size: 128x128	textures/gothic_light/gothic_light3.jpg
[534] mipLevels: 8	size: 256x128	textures/base_light/scrolllight2.tga
[540] mipLevels: 6	size: 32x32	menu/art/skill4
[541] mipLevels: 8	size: 256x256	models/weapons2/railgun/f_railgun3.tga
[548] mipLevels: 7	size: 64x64	gfx/2d/numbers/eight_32b.tga
[551] mipLevels: 1	size: 128x128	levelshots/pul1duel-oa.tga
[555] mipLevels: 7	size: 128x128	models/weaphits/bullet_0002.tga
[555] mipLevels: 6	size: 64x64	textures/effects/mediumhelth_spec
[556] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0018.tga
[556] mipLevels: 7	size: 128x128	gfx/fx/decals/bulletgeneric.tga
[557] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0009.tga
[557] mipLevels: 8	size: 128x128	menu/medals/medal_gauntlet.tga
[564] mipLevels: 9	size: 512x512	models/weapons2/gauntlet/gauntlet1.tga
[570] mipLevels: 1	size: 32x32	menu/art/3_cursor2
[572] mipLevels: 1	size: 128x128	*lightmap50
[573] mipLevels: 1	size: 128x128	*lightmap41
[574] mipLevels: 9	size: 256x512	textures/ctf/test2_r
[574] mipLevels: 1	size: 128x128	*lightmap32
[575] mipLevels: 1	size: 128x128	*lightmap23
[576] mipLevels: 1	size: 128x128	*lightmap14
[578] mipLevels: 1	size: 128x128	*lightmap76
[579] mipLevels: 7	size: 64x64	icons/iconr_yellow.tga
[579] mipLevels: 1	size: 128x128	*lightmap67
[580] mipLevels: 8	size: 256x256	models/gibs/skull-4.tga
[580] mipLevels: 7	size: 64x128	textures/pulchr/listlight_blend.tga
[580] mipLevels: 1	size: 128x128	*lightmap58
[581] mipLevels: 1	size: 128x128	*lightmap49
[584] mipLevels: 8	size: 256x256	models/weaphits/smokering2.tga
[586] mipLevels: 7	size: 64x64	icons/iconw_rocket.tga
[586] mipLevels: 7	size: 128x128	textures/base_wall/chrome_env.jpg
[589] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_6.tga
[592] mipLevels: 8	size: 256x128	textures/oafx/lbeam4.tga
[596] mipLevels: 6	size: 64x64	gfx/fx/spec/hairspec.tga
[599] mipLevels: 1	size: 256x256	menu/art/font2_prop.tga
[600] mipLevels: 8	size: 256x256	models/players/ayumi/skirt.tga
[600] mipLevels: 9	size: 512x512	models/powerups/shield/juiced-splat.tga
[604] mipLevels: 1	size: 128x32	menu/tab/name.tga
[607] mipLevels: 6	size: 64x64	textures/effects/mediumhelth
[610] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz3.tga
[610] mipLevels: 8	size: 256x256	models/powerups/armor/yellowarmor.tga
[618] mipLevels: 6	size: 64x64	sprites/le/splash.tga
[620] mipLevels: 8	size: 256x256	textures/base_floor/proto_rustygrate.tga
[625] mipLevels: 1	size: 128x128	models/players/ayumi/icon_default.tga
[629] mipLevels: 8	size: 256x256	textures/pulchr/tele-swirl.tga
[637] mipLevels: 7	size: 64x64	gfx/2d/numbers/seven_32b.tga
[638] mipLevels: 8	size: 256x256	textures/base_door/shinymetaldoor_outside
[643] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0021.tga
[643] mipLevels: 7	size: 128x128	sprites/multwake4.tga
[644] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0012.tga
[645] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0003.tga
[647] mipLevels: 8	size: 256x256	models/players/ayumi/bluehair.tga
[647] mipLevels: 7	size: 128x128	gfx/damage/shadow.tga
[651] mipLevels: 7	size: 128x128	gfx/fx/detail/d_sand.tga
[652] mipLevels: 7	size: 128x128	gfx/damage/plasma_mrk.tga
[652] mipLevels: 1	size: 128x128	*lightmap6
[658] mipLevels: 8	size: 256x384	textures/base_wall/bluemetalsupport2ftv.tga
[661] mipLevels: 8	size: 256x256	textures/oa/fiar.tga
[661] mipLevels: 8	size: 256x256	textures/oa/fiar
[663] mipLevels: 7	size: 64x64	icons/iconw_shotgun.tga
[664] mipLevels: 9	size: 512x512	models\weapons2\grenadel\grenadel
[664] mipLevels: 8	size: 128x128	gfx/2d/crosshairf.tga
[667] mipLevels: 7	size: 128x32	models/powerups/dblrstrip.tga
[667] mipLevels: 6	size: 64x64	sprites/balloon4.tga
[671] mipLevels: 9	size: 512x128	textures/base_light/border7_ceil50glow.tga
[673] mipLevels: 6	size: 32x32	menu/art/skill5
[679] mipLevels: 7	size: 128x128	textures/oa/grenfiar
[680] mipLevels: 8	size: 256x256	models/weapons2/lightning/muzzle1.tga
[684] mipLevels: 1	size: 128x128	models/players/penguin/icon_default.tga
[689] mipLevels: 8	size: 256x256	textures/base_floor/clangdark
[700] mipLevels: 7	size: 128x128	models/weaphits/bullet_0003.tga
[700] mipLevels: 1	size: 128x128	*lightmap60
[701] mipLevels: 1	size: 128x128	*lightmap51
[702] mipLevels: 1	size: 128x128	*lightmap42
[703] mipLevels: 1	size: 128x128	*lightmap33
[704] mipLevels: 1	size: 128x128	*lightmap24
[705] mipLevels: 1	size: 128x128	*lightmap15
[707] mipLevels: 1	size: 128x128	*lightmap77
[708] mipLevels: 1	size: 128x128	*lightmap68
[709] mipLevels: 1	size: 128x128	*lightmap59
[711] mipLevels: 8	size: 256x256	models/gibs/heart.tga
[712] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0019.tga
[714] mipLevels: 6	size: 64x64	gfx/2d/backtile
[716] mipLevels: 9	size: 512x512	models/weapons2/gauntlet/gauntlet2.tga
[718] mipLevels: 9	size: 512x512	textures/base_wall/patch10_beatup4
[719] mipLevels: 8	size: 256x256	gfx/fx/detail/d_met.tga
[720] mipLevels: 6	size: 64x64	textures/effects/smallhelth
[720] mipLevels: 8	size: 256x256	textures/base_trim/pewter_spec
[723] mipLevels: 7	size: 128x128	textures/oafx/quadmultshell
[723] mipLevels: 6	size: 64x64	textures/base_trim/slots1_1
[727] mipLevels: 9	size: 512x512	textures/gothic_trim/column2c_test
[729] mipLevels: 8	size: 256x256	sprites/plasmaa.tga
[730] mipLevels: 8	size: 256x128	textures/oafx/lbeam5.tga
[730] mipLevels: 7	size: 128x128	textures/base_wall/c_met5_2
[737] mipLevels: 8	size: 128x128	menu/medals/medal_impressive.tga
[737] mipLevels: 9	size: 512x512	textures/base_support/pipecolumn4
[738] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_7.tga
[744] mipLevels: 7	size: 128x128	textures/pulchr/teleenv.tga
[745] mipLevels: 8	size: 256x128	textures/base_wall/bluemetalsupport2e.tga
[751] mipLevels: 6	size: 64x64	textures/oafx/greenchrm.tga
[751] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz4.tga
[752] mipLevels: 7	size: 64x64	gfx/2d/numbers/four_32b.tga
[754] mipLevels: 6	size: 64x64	gfx/fx/spec/robawt.tga
[761] mipLevels: 7	size: 128x128	models/powerups/shield/impact.tga
[764] mipLevels: 7	size: 128x128	models/powerups/ammo/machammo2.tga
[766] mipLevels: 7	size: 128x128	models/weapons2/machinegun/f_machinegun2.tga
[767] mipLevels: 8	size: 256x256	models/powerups/armor/shard_env.jpg
[767] mipLevels: 8	size: 128x128	gfx/2d/lag.tga
[768] mipLevels: 6	size: 64x64	textures/base_light/ceil1_22a.blend.tga
[768] mipLevels: 6	size: 64x64	textures/base_light/ceil1_22a.tga
[768] mipLevels: 8	size: 256x384	textures/base_wall/bms2fglow.tga
[776] mipLevels: 7	size: 128x128	textures/base_wall/shiny3
[776] mipLevels: 1	size: 64x128	textures/pulchr/beam-pulchr-white.tga
[778] mipLevels: 7	size: 128x128	sprites/multwake5.tga
[780] mipLevels: 1	size: 128x128	*lightmap7
[785] mipLevels: 8	size: 128x128	menu/medals/medal_assist.tga
[787] mipLevels: 4	size: 16x16	gfx/misc/tracer2.tga
[791] mipLevels: 8	size: 128x128	gfx/damage/blood_stain.tga
[795] mipLevels: 7	size: 128x128	models/powerups/ammo/rockammo2.tga
[799] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0022.tga
[799] mipLevels: 7	size: 64x64	icons/iconw_machinegun.tga
[799] mipLevels: 8	size: 128x128	gfx/2d/crosshairg.tga
[800] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0013.tga
[801] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0004.tga
[802] mipLevels: 8	size: 256x256	models/players/ayumi/boots.tga
[803] mipLevels: 8	size: 256x256	models/players/ayumi/body.tga
[807] mipLevels: 8	size: 256x256	textures/clown/circ4glow.tga
[808] mipLevels: 1	size: 8x8	*white
[810] mipLevels: 7	size: 64x64	icons/iconw_railgun.tga
[814] mipLevels: 7	size: 128x128	textures/effects/megahelth_spec
[814] mipLevels: 6	size: 64x64	gfx/misc/hastesmoke.tga
[818] mipLevels: 7	size: 128x128	textures/base_wall/chrome_env2.jpg
[820] mipLevels: 7	size: 64x64	icons/icona_rocket.tga
[822] mipLevels: 7	size: 128x128	textures/base_floor/floor3_3dark
[826] mipLevels: 6	size: 64x4	models/weapons2/shotgun/shotgun_laser.tga
[827] mipLevels: 8	size: 128x128	menu/medals/medal_capture.tga
[828] mipLevels: 1	size: 128x128	*lightmap70
[829] mipLevels: 1	size: 128x128	*lightmap61
[830] mipLevels: 1	size: 128x128	*lightmap52
[831] mipLevels: 7	size: 128x128	models/weapons2/lightning/f_lightning.tga
[831] mipLevels: 8	size: 256x256	models/weapons2/lightning/muzzle2.tga
[831] mipLevels: 1	size: 128x128	*lightmap43
[832] mipLevels: 1	size: 128x128	*lightmap34
[833] mipLevels: 1	size: 128x128	*lightmap25
[834] mipLevels: 1	size: 128x128	*lightmap16
[836] mipLevels: 1	size: 128x128	*lightmap78
[837] mipLevels: 6	size: 64x64	models/weapons2/railgun/railcore.tga
[837] mipLevels: 1	size: 128x128	*lightmap69
[840] mipLevels: 7	size: 128x128	models/powerups/ammo/railammo2.tga
[842] mipLevels: 7	size: 128x128	sprites/le/smoke1.tga
[844] mipLevels: 7	size: 128x128	textures/flares/lava.tga
[845] mipLevels: 7	size: 128x128	models/weaphits/bullet_0004.tga
[847] mipLevels: 8	size: 256x256	textures/base_wall/comp3env.jpg
[868] mipLevels: 8	size: 256x128	textures/oafx/lbeam6.tga
[868] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_1.tga
[872] mipLevels: 7	size: 128x32	models/powerups/scstrip.tga
[877] mipLevels: 8	size: 256x256	models/ammo/rocket/rocket.tga
[878] mipLevels: 6	size: 64x64	sprites/bubble.tga
[884] mipLevels: 7	size: 64x64	gfx/2d/select.tga
[885] mipLevels: 6	size: 64x64	gfx/fx/spec/skin.tga
[886] mipLevels: 7	size: 128x128	textures/effects/megahelth
[886] mipLevels: 7	size: 128x128	textures/effects/megahelth.tga
[887] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_8.tga
[888] mipLevels: 10	size: 512x512	textures/pulchr/five-steps-ahead.tga
[890] mipLevels: 8	size: 256x256	models/gibs/veins.tga
[891] mipLevels: 7	size: 128x128	models/powerups/ammo/shotammo2.tga
[892] mipLevels: 6	size: 64x64	textures/oa/muzzle/muz5.tga
[893] mipLevels: 7	size: 128x128	models/players/ayumi/eyes.tga
[896] mipLevels: 7	size: 128x128	textures/gothic_trim/metalsupport4i_bit.tga
[897] mipLevels: 7	size: 64x64	icons/icona_shotgun.tga
[900] mipLevels: 8	size: 256x256	models/players/ayumi/head.tga
[907] mipLevels: 8	size: 256x256	models/gibs/genericgibmeat.tga
[908] mipLevels: 1	size: 128x128	*lightmap8
[908] mipLevels: 1	size: 128x128	*lightmap0
[910] mipLevels: 8	size: 256x256	textures/clown/text3.tga
[913] mipLevels: 7	size: 128x128	sprites/multwake6.tga
[920] mipLevels: 9	size: 512x512	models/weapons2/machinegun/mgun
[920] mipLevels: 9	size: 512x512	models/weapons2/machinegun/mgun.tga
[924] mipLevels: 7	size: 128x128	models/weapons2/machinegun/f_machinegun3.tga
[924] mipLevels: 8	size: 256x256	gfx/fx/detail/d_ice.tga
[933] mipLevels: 7	size: 128x128	models/weapons2/shells/sgunshell
[934] mipLevels: 8	size: 128x128	gfx/2d/crosshairh.tga
[937] mipLevels: 1	size: 32x32	menu/art/switch_off
[939] mipLevels: 7	size: 64x64	icons/iconh_green.tga
[944] mipLevels: 7	size: 128x128	textures/base_trim/tinfx.jpg
[947] mipLevels: 7	size: 128x128	textures/base_floor/proto_grate4.tga
[948] mipLevels: 7	size: 128x128	gfx/damage/burn_med_mrk.tga
[953] mipLevels: 7	size: 128x128	textures/gothic_light/gothic_light2_blend.jpg
[956] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0014.tga
[956] mipLevels: 1	size: 128x128	*lightmap80
[957] mipLevels: 7	size: 128x128	textures/sfx/130boom/dpexplosion4_0005.tga
[957] mipLevels: 1	size: 128x128	*lightmap71
[958] mipLevels: 9	size: 512x512	models/players/ayumi/blueshirt.tga
[958] mipLevels: 1	size: 128x128	*lightmap62
[959] mipLevels: 1	size: 128x128	*lightmap53
[960] mipLevels: 1	size: 128x128	*lightmap44
[961] mipLevels: 9	size: 512x512	models/weapons2/shotgun/shotgun.tga
[961] mipLevels: 1	size: 128x128	*lightmap35
[962] mipLevels: 1	size: 128x128	*lightmap26
[963] mipLevels: 1	size: 128x128	*lightmap17
[965] mipLevels: 7	size: 128x128	textures/oafx/regenshell.tga
[965] mipLevels: 1	size: 128x128	*lightmap79
[976] mipLevels: 8	size: 256x256	models/powerups/ammo/ammobox.tga
[977] mipLevels: 7	size: 128x128	sprites/le/smoke2.tga
[982] mipLevels: 8	size: 256x256	models/weapons2/lightning/muzzle3.tga
[986] mipLevels: 5	size: 16x16	*default
[990] mipLevels: 7	size: 128x128	models/weaphits/bullet_0005.tga
[993] mipLevels: 6	size: 64x64	textures/effects/envmapblue2.tga
[996] mipLevels: 8	size: 256x256	textures/gothic_trim/metalsupport4i.tga
[998] mipLevels: 8	size: 128x128	gfx/damage/blood_spurt.tga
[1006] mipLevels: 8	size: 256x128	textures/oafx/lbeam7.tga
[1007] mipLevels: 7	size: 64x128	textures/pulchr/listlight.tga
[1013] mipLevels: 7	size: 128x128	gfx/2d/crosshaira
[1013] mipLevels: 8	size: 256x256	textures/sfx/clangdark_bounce.jpg
[1016] mipLevels: 7	size: 128x128	textures/base_wall/c_met7_2
[1017] mipLevels: 7	size: 128x128	models/weaphits/rlboom/rlboom_2.tga
[1021] mipLevels: 1	size: 16x16	*dlight
  
 Total 490 images, hash Table used: 366/1024
  
 Top 5: 34, 4, 3, 3, 3
-----------------------------------------------------
```
