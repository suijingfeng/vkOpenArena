### The OpenGL Rendering Pipeline Evolution

OpenGL 1.0 API finalized in 1992, first implementation in 1993


* 1.1 (1997): vertex arrays and texture objects
* 1.2 (1998): 3D textures
* 1.3 (2001): cubemap textures, compressed textures, multitextures
* 1.4 (2002): mipmap generation, shadow map textures, etc
* 1.5 (2003): vertex buffer object, shadow comparison functions, occlusion queries, non-power-of-2 textures
* 2.0 (2004): vertex and fragment shading (GLSL 1.1), multiple render targets, etc
* 2.1 (2006): GLSL 1.2, pixel buffer objects, etc
* 3.0 (2008): GLSL 1.3, deprecation model, etc
* 3.1 (2009): GLSL 1.4, texture buffer objects, move much of deprecated functions to ARB compatible extension
* 3.2 (2009): OpenGL Extensions

New features/functions are marked with prefix

* Supported only by one vendor: NV\_float\_buffer (by nvidia)
* Supported by multiple vendors: EXT\_framebuffer\_object
* Reviewed by ARB: ARB\_depth\_texture
* Promoted to standard OpenGL API

Deprecation Model, Contexts, and Profiles

* Redundant and In-efficient functions are deprecated: glBegin(), glEnd()
* OpenGL Contexts ¨C data structures where OpenGL stores the state information used for
rendering : Textures, buffer objects, etc
* Profile ¨C A subset of OpenGL functionality specific to an application domain


## The Rendering Pipeline
* The process to generate two-dimensional images from given virtual cameras and 3D objects
* The pipeline stages implement various core graphics rendering algorithms


## Different Spaces


Eye space: A space where you define the vertex coordinates, normals,
etc. This is before any transformations are taking place. These 
coordinates/normals are multiplied by the OpenGL modelview matrix
into the eye space

Modelview matrix: Viewing transformation matrix (V)
multiplied by modeling transformation matrix (M), i.e.,
`GL\_MODELVIEW = V * M`
OpenGL matrix stack is used to allow different modelview
matrices for different objects

## Different Spaces

### Eye space: Where per vertex lighting calculation is occurred
* Camera is at (0,0,0) and view¡¯s up direction is by default (0,1,0)
* Light position is stored in this space after being multiplied by the OpenGL modelview matrix

* Vertex normals are consumed by the pipeline in this space by the lighting equation

### Clip Space: After projection and before perspective divide
Clipping against view frustum done in this space

` -W <= X <= W; -W <=Y <=W; -W <=Z <=W; `

New vertices are generated as a result of clipping
The view frustum after transformation is a parallelepiped
regardless of orthographic or perspective projection
Perspective Divide

Transform clip space into NDC space
Divide (x,y,z,w) by w where `w = z/-d` (d=1 in OpenGL so w = -z)
Result in foreshortening effect

### Window Space: Map the NDC coordinates into the window
X and Y are integers, relative to the lower left corner of the window
Z are scaled and biased to [0,1]
Rasterization is performed in this space
The geometry processing ends in this space

## The Geometry Stage

Transform coordinates and normal
 Model->world
 World->eye
Normalize the normal vectors
Compute vertex lighting
Generate (if necessary) and transform texture coordinates
Transform to clip space (by projection)
Assemble vertices into primitives
Clip against viewing frustum
Divide by w (perspective divide if applies)
Viewport transformation
Back face culling

## The Rasterizer Stage

Per-pixel operation: assign colors to the pixels in the frame buffer
(a.k.a scan conversion)

Main steps:
* Setup
* Sampling (convert a primitive to fragments)
* Texture lookup and Interpolation (lighting, texturing, z values, etc)
* Color combinations (illumination and texture colors)
* Fogging
* Other pixel tests (scissor, alpha, stencil tests etc)
* Visibility (depth test)
* Blending/compositing/Logic op

Convert each primitive into fragments (not pixels)
Fragment: transient data structures
position (x,y); depth; color; texture coordinates;

Fragments from the rasterized polygons are then selected
(z buffer comparison for instance) to form the frame buffer pixels

### Two main operations:
Fragment selection: generate one fragment for each pixel that is intersected by the primitive

Fragment assignment: sample the primitive properties (colors, depths, etc) for each fragment - nearest neighbor continuity, linear interpolation, etc

Polygon Scan Conversion

The goal is to compute the scanline-primitive intersections
OpenGL Spec does not specify any particular algorithm to use
OpenGL Spec does not specify any particular algorithm to use

Find ymin and ymax for each edge and only test the edge with scanlines in between

For each edge, only calculate the intersection with the ymin;
calculate dx/dy; calculate the new intersection as y=y+1, x+dx/dy

Change x=x+dx/dy to integer arithmetic (such as using
Bresenham¡¯s algorithm)

### Rasterization steps

Texture interpolation
Color interpolation
Fog (blend the fog color with the fragment color based on the
depth value)
Scissor test (test against a rectangular region)
Alpha test (compare with alpha, keep or drop it)
Stencil test(mask the fragment depending on the content of the
stencil buffer)
Depth test (z buffer algorithm)
Alpha blending
Dithering (make the color look better for low res display mode)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_1.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_2.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_3.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_4.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_5.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_6.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_7.png)

![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/pipeline_8.png)

