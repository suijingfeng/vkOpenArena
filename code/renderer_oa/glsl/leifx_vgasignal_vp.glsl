
varying vec2 texture_coordinate;
varying vec2 texture_coordinate2;
varying vec2 texture_coordinate3;
varying vec2 texture_coordinate4;
varying vec2 texture_coordinate5;
varying float scale;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

void main()
{
	vec2 eh;


    // Transforming The Vertex 
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 
  
    // Passing The Texture Coordinate Of Texture Unit 0 To The Fragment Shader 
    texture_coordinate = vec2(gl_MultiTexCoord0); 



}
