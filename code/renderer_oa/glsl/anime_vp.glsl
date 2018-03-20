
varying vec2 texture_coordinate;
varying vec2 texture_coordinate2;
varying vec2 texture_coordinate3;
varying vec2 texture_coordinate4;
varying vec2 texture_coordinate5;
varying float scale;
uniform float u_ScreenToNextPixelX;
uniform float u_Time;
uniform float u_ScreenToNextPixelY;
void main()
{
	scale=116.0;
	vec2 eh;

	eh.x = u_ScreenToNextPixelX * scale; 
	eh.y = u_ScreenToNextPixelX * scale;

    // Transforming The Vertex 
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 
  
    // Passing The Texture Coordinate Of Texture Unit 0 To The Fragment Shader 
    texture_coordinate = vec2(gl_MultiTexCoord0); 
   texture_coordinate2 = vec2(gl_MultiTexCoord0);
   texture_coordinate2.x=texture_coordinate.x-0.003*eh.x;
   texture_coordinate3 = vec2(gl_MultiTexCoord0);
   texture_coordinate3.x=texture_coordinate.x+0.003*eh.x;
   texture_coordinate4 = vec2(gl_MultiTexCoord0);
   texture_coordinate4.y=texture_coordinate.y-0.003*eh.y;
   texture_coordinate5 = vec2(gl_MultiTexCoord0);
   texture_coordinate5.y=texture_coordinate.y+0.003*eh.y;


}
