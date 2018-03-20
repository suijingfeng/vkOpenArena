
varying vec2 texture_coordinate;
varying vec2 texture_coordinate2;
varying vec2 texture_coordinate3;
varying vec2 texture_coordinate4;
varying vec2 texture_coordinate5;
varying float scale;
uniform float u_CC_Overbright; 
uniform float u_CC_Brightness; 
uniform float u_CC_Gamma; 
uniform float u_CC_Contrast; 
uniform float u_CC_Saturation; 
uniform float u_ScreenToNextPixelX;
uniform float u_Time;
uniform float u_ScreenToNextPixelY;
void main()
{
	scale=0.7;
    // Transforming The Vertex 
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 
  
    // Passing The Texture Coordinate Of Texture Unit 0 To The Fragment Shader 
    texture_coordinate = vec2(gl_MultiTexCoord0); 

}
