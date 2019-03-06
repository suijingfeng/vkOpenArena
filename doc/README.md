# Quake 3: Arena Rendering Architecture

by Brian Hook, id Software Presentation at GDC 1999


## Introduction

This will give you bare bone scratchpad notes as one might have taken
during the session, with bits and pieces missing or out of order.

Renderer is split into two parts - the front end and the back end.

### Front end

The front end provides an interface to the renderer and allows the game
to submit entities to be drawn. The entities are added to a list and 
will be consumed at a later point in time by the back end.

Unlike modern renderers, the scene is not persisted between frames. 
The scene must be built up every frame by the game. This negatively 
impacts performance somewhat as the renderer can no longer make use of 
temporal coherence between frames - a property of a game where by the 
majority of entities will not move, or will not move every much between 
frames, allowing for some optimizations.


From the game:

* R\_ClearScene: Removes all objects from the current scene
* R\_AddAdditiveLightToScene, R_AddLightToScene: Adds a light to the scene
* R\_AddPolyToScene: Add polygons to scene (usually used to add particles for particle effects)
* R\_AddDecalToScene: Adds a decal to the scene
* R\_RenderScene: Renders the scene. Drawable surfaces are queued to be drawn. This is where the majority of the work is done for pushing work to the backend via the command queue.


Along-side scene rendering, the front end also allows the game to 
render textured rectangles easily for UI elements and text:
* R\_DrawStretchPic: Draw textured rectangle
* R\_DrawRotatePic: Draw rotated textured rectangle, with centre of rotation at the corner
* R\_DrawRotatePic2: Draw rotated textured rectangle, with centre of rotation at the origin
* R\_Font\_DrawString: Draw text with specific font

Each of these calls will add a single command into the command queue
which is consumed by the backend.


### Rough stack trace for R_RenderScene
```
RE_BeginScene
    Setup tr.refdef
    R_AddDecals
if shadows for point lights
    R_RenderDlightCubemaps
        for each cube face
            R_RenderView
if player shadows
    R_RenderPshadowMaps
        for each shadowing entity
            R_AddEntitySurface
        R_SortAndSubmitDrawSurfs
if sun shadows
    R_RenderSunShadowMaps  // first shadow cascade
        R_RotateForViewer
        R_SetupProjectionOrtho
        R_AddWorldSurfaces
        R_AddPolygonSurfaces
        R_AddEntitySurfaces
        R_SortAndSubmitDrawSurfs
    R_RenderSunShadowMaps  // second shadow cascade
    R_RenderSunShadowMaps  // third shadow cascade
R_RenderView
    R_RotateForViewer  // sets up model/view matrices
    R_SetupProjection  // sets up projection matrix
        R_SetupFrustum  // computes the camera frustum
    R_GenerateDrawSurfs
        R_AddWorldSurfaces  // add all visible map surfaces
        R_AddPolygonSurfaces  // adds all particle effects
        R_SetFarClip
        R_SetupProjectionZ  // calculate far Z based on currently added objects
        R_AddEntitySurfaces
            for each entity
                R_AddEntitySurface
    R_SortAndSubmitDrawSurfs
        R_RadixSort // sort surfaces to reduce draws later on
        for each mirror
            R_MirrorViewBySurface
                R_RenderView
        R_AddDrawSurfCmd  // submits a single 'draw' command into the command queue
    R_DebugGraphics
R_AddPostProcessCmd
RE_EndScene
```

## Back end
The backend executes commands only from the command queue. This command queue is filled by the front end, and can be any one of the following commands. Each command is handled by a separate function in `tr_backend.c`.
                 
| Render command name | Handler function | Description |
| --- | --- | --- |
| `RC_END_OF_LIST` | Handled inline - calls `RB_EndSurface` if necessary | Marks the end of the command queue |
| `RC_SET_COLOR` | `RB_SetColor` | Sets the active color |
| `RC_STRETCH_PIC` | `RB_StretchPic` | Draws a textured quad |
| `RC_ROTATE_PIC` | `RB_RotatePic` | Draws a rotated textured quad, with its center of rotation at the corner |
| `RC_ROTATE_PIC2` | `RB_RotatePic2` | Draws a rotated textured quad, with its center of rotation at the origin |
| `RC_DRAW_SURFS` | `RB_DrawSurfs` | Draws surfaces |
| `RC_DRAW_BUFFER` | `RB_DrawBuffer` | Sets the active draw buffer |
| `RC_SWAP_BUFFERS` | `RB_SwapBuffers` | Presents the back buffer to the screen |
| `RC_SCREENSHOT` | `RB_TakeScreenshotCmd` | Takes a screenshot |
| `RC_VIDEOFRAME` | `RB_TakeVideoFrameCmd` | Records a video frame |
| `RC_COLORMASK` | `RB_ColorMask` | Sets the active color mask |
| `RC_CLEARDEPTH` | `RB_ClearDepth` | Clears the depth buffer |
| `RC_CAPSHADOWMAP` | `RB_CaptureShadowMap` | Captures the depth buffer for shadow mapping |
| `RC_POSTPROCESS` | `RB_PostProcess` | Perform post processing |
| `RC_BEGIN_TIMED_BLOCK` | `RB_BeginTimedBlock` | Begin timed block |
| `RC_END_TIMED_BLOCK` | `RB_BeginTimedBlock` | End timed block |

The backend starts processing the command queue in two circumstances:

1. The game engine wishes to render the next frame. `RE_BeginFrame` is called, which will call `R_IsuePendingRenderCommands`.
2. The renderer needs to flush the command queue. This can happen just before a GPU resource is changed or uploaded (e.g. a texture, or memory buffer), so that the synchronization is made explicit (not 100% true, but we'll stick with this), or if the command queue is full. `R_IssuePendingRenderCommands` is called directly.

## Rendering surfaces
To draw anything, the following generally has to happen:

1. Call `RB_BeginSurface` to start a new surface, and reset the `tess` object.
2. Set render matrices either by calling `RB_SetGL2D` for orthographic rendering, or setting up the matrices manually.
3. Upload any data to the GPU if necessary. This includes vertex, index and uniform data.
4. Call `RB_EndSurface` to finalise the surface, which causes it to be drawn or added to the current render pass.

This is a very simplified view - where these steps are changed are described below for 2D and 3D objects.

Originally, in the vanilla renderer, in order to reduce the number of draw calls the surface is left "open" until the state for the surface changes (e.g. if the Q3 shader has changed). When the surface state does change, `RB_EndSurface` is called, and `RB_BeginSurface` is called to start the new surface.

For example, surface A is using shader 1, surface B is using shader 1, and surface C is using shader 2. The call sequence would be:

1. Call `RB_BeginSurface` for surface A, using shader 1
2. Add surface A vertex and index data to `tess`.
3. Add surface B vertex and index data to `tess`.
4. Call `RB_EndSurface` for surfaces A and B, using shader 1
5. Call `RB_BeginSurface` for surface C, using shader 2
6. Add surface C vertex and index data to `tess`.
7. Call `RB_EndSurface` for surface C using shader 2.

Although there are three surfaces, only two draws were issued. This works well for dynamic vertex data, which is re-uploaded every frame, as was the case in the vanilla renderer, and also in rend2 for 2D objects.

### Rendering 2D surfaces
2D objects are drawn using `RB_StretchPic`, `RB_RotatePic` and `RB_RotatePic2`.

### Rendering 3D surfaces
For 3D objects, which are drawn through `RB_DrawSurfs`, it gets a bit more difficult. The majority of model surfaces are loaded into GPU buffers when the model itself is loaded. This means that for each surface we must issue a single draw, regardless of whether the previous surface used the same vertex data.

*Note: a potential optimisation here is to issue an instanced draw for consecutive models which are the same.*

#### Rough stack trace of `RB_DrawSurfs`
```
RB_BeginDrawingView
    if no target FBO
        FBO_Bind  // binds 'default' FBO
    else
        FBO_Bind  // bind target FBO
        if rendering to cube map face
            Update FBO color attachment to use correct cube map face texture

    Setup viewport and scissor
    Clear colour and depth if necessary

if rendering world models and (rendering depth prepass or shadow maps)
    Disable color writes
    Set flag indicating depth-only render
    RB_RenderDrawSurfList
    Clear flag indicating depth-only render
    Enable color writes

    if using MSAA
        FBO_FastBlit  // resolve depth buffer

    if SSAO enabled
        FBO_BlitFromTexture  // copy depth buffer into separate texture

    if sun shadows enabled
        FBO_Bind  // bind fbo storing shadow term
        Setup uniforms for rendering shadows
        RB_InstantQuad2  // draw fullscreen quad with shader to render shadows

    if SSAO enabled
        FBO_Bind  // bind quarter-res fbo for SSAO
        Copy depth buffer to quarter-res FBO
        Downscale and Gaussian blur
        
    FBO_Bind  // reset FBO binding

if not rendering shadows
    RB_RenderDrawSurfList
    if draw sun
        RB_DrawSun

    if draw sun rays
        Draw sun rays

if rendering to cube map face
    Generate mip maps for cube map face
```



## Portability and OS Support
   Win9x
   WinNT (AXP & x86)
   MacOS
   Linux

New renderer with material based shaders (built over OpenGL1.1 Fixed Pipeline).
Window system stuff is abstracted through an intermediate layer.
 

## Incremental improvement on graphics technology

    Volumetric fog
    Portals/mirrors
    Wall marks, shadows, light flares, etc.
    Environment mapping
    Shader architecture allows us to do cool general effects 
    specified in a text file, e.g. fire and quad
    Better lighting
    Specular lighting
    Dynamic character lighting
    Tagged model system
    Optimized for OpenGL
 


* Volumetric fog
    distance based fog sucks
    constant density or gradient
    fog volumes are defined by brushes
    triangles inside a fog volume are rendered with another pass, 
    with alpha equal to density computed as the distance
    from the viewer to the vertex through the fog volume

advantages of our technique
    Allows true volumetric fog

disadvantages
    T-junctions introduced at the boundary between the fog brush 
     and the non-fog brush due to tessellation of the triangles 
    inside the fog-brush.
    Triangle interpolation artifacts
    Excessive triangle count due to the heavy tessellation

## Portals/mirrors

    basically equivalent, only difference is location of the virtual viewpoint
    only a single portal/mirror is allowed at once to avoid infinite recursion
    insert PVS sample point at mirror location.

    https://stackoverflow.com/questions/9449369/quake-3-portal-and-frustum-culling
    
    R_MarkLeaves() marks the precomputed PVS (potentially visible set) of leaf nodes (convex hulls)
    given the location of the camera / eye. The BSP tree traversal solves the drawing order problem
    (depth sorting) for software rendering, but at the time, it still resulted in too much overdraw.
    The PVS is used to prune leaf nodes that are demonstrably not visible from the current node / position.

## Environment mapping 

    Wall marks

    Volumetric shadows

    Problems
       Shadows can cast through surfaces
       Expensive since you have to determine silhouette edges
         (connectivity can be precomputed however)
       Expensive since it requires a LOT of overdraw, both in 
         the quads and the final screen blend
       exhibits artifacts such as inconsistency with world 
         geometry or Gouraud shading on models


## Rendering primitives 

   Quadratic Bezier patches
      Tessellated at load time to arbitrary detail level
      Row/column dropping for dynamic load balancing
      Simpler to manipulate than cubic Bezier patches
      Artifacts not very noticeable   

## MD3 (arbitrary triangle meshes) 
      Multipart player models consisting of connect animated 
      vertex meshes created in 3DSMAX
      Post processed by Q3DATA into MD3 format
      Simple popping LOD scheme
      Based on projected sphere size
      LOD bias capability
      No noticeable artifacts
      Suitable technological progress given our time frame
      Spurred by need for convincing clouds and environment

## standard Q2 sky box 
      projection of clouds onto hemisphere, multiple layers possible
      tessellated output feeds into standard shader pipeline

## Lightmaps
      Covers same world area as Q2, 1 lightmap texel covers 2 sq. ft., 
      which corresponds to one 32x32 texture block Generated using direct lighting, 
      not radiosity.
      
      Diffuse lightmaps blended using src*dst.
      
      Dynamic lights handled through three dynamically modified lightmaps
      uploaded with glTexSubImage2D, performance gain from using subimage.

## Specular lighting
      specular lighting is simply a hacked form of dynamic environment mapping
      specularity encoded in alpha channel of texture (mono-specular materials)
      color iterator stores the generated specular light value in iterated alpha
      walls render lightmap then base texture using src*dst + dst*src.alpha
      models render Gouraud only, then base texture using src*dst + dst*src.alpha

## Character lighting
    Overbrightening
      
      lighting program assumes a dynamic range 2x than normally exists
      during rendering we set the hardware gamma ramp so that the back half of it is saturated to identity
      
      net effect is that all textures are doubled in brightness but only exhibit saturation artifacts if they exceed 50% intensity
      
      requires Get/SetDeviceGammaRamp
      
      not available on NT4
      
      requires 3Dfx extension on Voodoo2
      
      we gain dynamic range in exchange for lower precision

    Cheezy shadows
      project a polygon straight down
      alpha determined by height of object
      basically just a dynamic wall mark

    Sunlight


## Shader Architecture 

actually materials
    many special effects done with very little coding
    descriptions in text (.shader) files
    general information stored in the material definition body
  
    sound type
    editor image
    vertex deformation information
    lighting information
    misc other material global information
    tessellation density (for brushes only)
    sort bias

    multiple stages can specify:
    texture map (name or $lightmap)
    texcoord source
    environment mapping
    texture coordinates
    lightmap coordinates

    alpha and color sources
    identity
    waveform
    client game
    diffuse/specular lighting
    alpha and color modifiers
    unused right now

    texture coordinate modifiers
    rotate (degs/second)
    clamp enable/disable
    scroll (units/second per S & T)
    turbulence
    scale (waveform)
    arbitrary transform
    blend function
    depth test and mask
    alpha test

    multitexture collapsing
    depending on underlying multitexture capability
    we can examine blend funcs and other relevant data
    and collapse into multitexture appropriately
    polygon offset
    used for wall marks, cheezy shadows
  

## Optimized for hardware acceleration
    Triangle meshes have a sort key that encodes material state, sort type, etc
    qsort on state before rendering
    1.5M multitexture tris/second on ATI Rage128 on a PIII/500 with 50% of our time in the OpenGL driver

## Triangle renderer
    Strip order, but not strips
    32B aligned 1K vertex buffers
    we have knowledge of all rendering data before we begin rendering due to our sorting/batching of primitives
    rescale Z range every frame for max precision in depth buffer
  
    TessEnd_MultitexturedLightmapped
    First texture unit in GL_REPLACE mode
    Second texture unit in GL_MODULATE mode
    Color arrays disabled
    Uses ARB_multitexture extension

    TessEnd_VertexLit
    Handles models
    Same as TessEnd_Generic, just less setup/application cruft on our side, looks the same to the driver

## Scalability 
    Vertex light option (fill rate bound or lacking blending modes)
    LOD bias for models (throughput bound)
    Subdivision depth for curves (throughput bound)
    Screen re-size (fill bound)
    Dynamic lights can be disabled (CPU bound)
    Supporting multiple CPU architectures

## OpenGL support 
    Die, minidriver, die
    No support for minidriver or D3D wrapper
    Gave OpenGL an early boost
    Gave OpenGL an anchor to future development
    Wrote Quake III on Intergraph workstations,
    typically dual processor PII/400 machines however with fairly slow (~60mpixel/second) fill rate to keep us honest
    QGL dynamic loading bindings
    LoadLibrary + wglGetProcAddress
    Necessitated because of Voodoo & Voodoo2 ICD vs. minidriver vs. standalone driver
    Allows us to log OpenGL calls

## Transforms 
    We use the full OpenGL transform pipeline
    We do not use the OpenGL lighting pipeline
    We do not use OpenGL fog capabilities

## Extensions Supported
    Written on vanilla OpenGL, extension support is completely optional
    ARB\_multitexture
    texture environment extensions
    S3\_s3tc and other compressed texture extensions
    EXT\_swapinterval
    3DFX\_gamma\_control

## Specific hardware comments 
  * Note to IHVs: Intel wants to work with you on hardware acceleration,
   contact Igor Sinyak (igor.sinyak@intel.com) if interested
  * Voodoo/V2/V3/Banshee, very good OpenGL driver, 16-bit
  * S3 Savage3D, good OpenGL driver, Savage4 has strong multitexture capability
  * ATI Rage Pro, poor image quality, low performance, but still supported owns the market
  * ATI Rage 128, feature complete, very fast OpenGL Rendition V2200 state of ICD is unknown at this point, but good hardware
  * Intel i740, 16-bit, very high quality OpenGL drivers, AGP texturing, average image quality
  * Matrox G200/G400, looks like they have good hardware and recent strong ICD support
  * Nvidia Riva128(ZX), good OpenGL driver, very poor image quality
  * Nvidia RivaTNT/TNT2, feature complete, very fast OpenGL, Recommendations

## The Future: Content and Technology 
   * technological advances are second order effects
   * technological advances need appropriate content
   * high resolution art with large dynamic range
   * level design that maximizes the given the triangle budget
   * lighting design that is effective and dramatic
   * game design that leverages the technology effectively



