I'm wondering what the projection plane is used in the OpenGL perspective transformation matrix.


In fact there is no explicit projection plane needed.

The only planes are those limiting the viewing volume. 
Their requirement is that none of the 6 sides of the viewing volume is degenerated to a point 
(what the reason is that e.g. the near clipping plane must not be located at a distance of 0). 
As long as all 6 sides are 2 dimensional, the viewing volume can be transformed into the cube mentioned by Hodgman.

A projection plane doesn't hide anything (as opposed to clipping planes). 
And the precedence of a surface hit is only given by its closer distance to the origin on the projector line in comparison to other hits.
So it plays no role whether a projection plane is on the near clipping plane, the far clipping plane, or in-between them (or even elsewhere).

Moreover, it isn't required that a projection plane is parallel to the near/far clipping planes. 
E.g. the oblique projections (although counting to the parallel projections but not to the perspective projection) 
have non-parallel projection planes w.r.t. the viewing volume. 


near plane = the plane of the final projected image. 
The cameras eye is behind that and the world is in front of that. 
Projection plane = near plane
If near plane is the final projected plane then I have got my answer
