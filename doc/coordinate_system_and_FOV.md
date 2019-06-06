### Coordinate
id software still uses the same coordinate system from 1992 Wolfenstein 3D (as of Doom3 this was still true). It is important to know that if you try to read the renderer source code:

With id's system:

    X axis = Left/Right
    Y axis = Forward/Backward
    Z axis = Up/Down

OpenGL's coordinate system:

    X axis = Left/Right
    Y axis = Up/Down
    Z axis = Forward/Backward

### The Viewport Transformation

The final transformation in the pipeline before rasterization is the
viewport transformation. The coordinates produced by the last stage
in the geometry pipeline (or produced by the clipper) are in
homogeneous clip coordinates.

First, the homogeneous coordinates are all divided through by their
own w components, producing normalized device coordinates.
```
    x_c             x_d
    y_c             y_d
    z_c  *  1/w_c = z_d 
    w_c             1.0
```
Here, the vertex's clip coordinates are represented by `(x_c, y_c, z_c, w_c)`
and the normalized device coordinates are represented by `(x_d, y_d, z_d)`.
The normalized device coordinate is considered to be a 3D coordinate.

Before the primitive can be rasterized, it needs to be transformed into
framebuffer coordinates, which are coordinates relative to the origin of the
framebuffer. This is performed by scaling and biasing the vertex nomalized
device coordinates by the selected viewport transform.

The transform from normalized device coordinates to framebuffer coordinates
is performed as:

```
    x_f     p_x/2 * x_d + o_x
    y_f  =  p_y/2 * y_d + o_y
    z_f     p_z/2 * z_d + o_z
```
which is equal to
```
    x_f     p_x/2   0     0     o_x     x_d
    y_f  =    0    p_y/2  0     o_y  *  y_d
    z_f       0     0    p_z    o_z     z_d
     1        0     0     0      1       1 
```
where the `(x_f, y_z, z_f)` are the coordinates of each vertex in the framebuffer
The x and y fileds of VkViewport are o\_x and o\_y, respecitively, 
and the minDepth is o\_z, The width and height fields are used for p\_x, p\_y
p\_z is formed from the expression (maxDepth - minDepth).


![](https://github.com/suijingfeng/vkOpenArena/blob/master/doc/vulkan_vs_opengl_coordinate.jpg)


### 2D Case

```
void R_Set2dProjectMatrix(float width, float height)
{
    s_ProjectMat2d[0] = 2.0f / width; 
    s_ProjectMat2d[1] = 0.0f; 
    s_ProjectMat2d[2] = 0.0f;
    s_ProjectMat2d[3] = 0.0f;

    s_ProjectMat2d[4] = 0.0f; 
    s_ProjectMat2d[5] = 2.0f / height; 
    s_ProjectMat2d[6] = 0.0f;
    s_ProjectMat2d[7] = 0.0f;

    s_ProjectMat2d[8] = 0.0f; 
    s_ProjectMat2d[9] = 0.0f; 
    s_ProjectMat2d[10] = 1.0f; 
    s_ProjectMat2d[11] = 0.0f;

    s_ProjectMat2d[12] = -1.0f; 
    s_ProjectMat2d[13] = -1.0f; 
    s_ProjectMat2d[14] = 0.0f;
    s_ProjectMat2d[15] = 1.0f;
}
```

fovX: 60.000000, fovY: 19.687500, aspect: 3.327352

fovX: 30.000000, fovY: 30.000000, aspect: 1.000000

fovX: 141.447388, fovY: 116.257965, aspect: 1.777777    16:9  cg\_fov = 140

fovX: 106.260201, fovY: 73.739792, aspect: 1.777778     16:9  cg\_fov = 90

fovX: 100.388855, fovY: 73.739792, aspect: 1.600000     1440:900 cg\_fov = 90
