/*

	1980s japanimation ^_^

 	Copyright (C) 2014 leilei
 
 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option)
 any later version.


  
*/

uniform sampler2D u_Texture0; 
uniform sampler2D u_Texture1; 
uniform sampler2D u_Texture2; 
uniform sampler2D u_Texture3; 
uniform sampler2D u_Texture4; 
uniform sampler2D u_Texture5; 
uniform sampler2D u_Texture6; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_Time;
#define color_enhance 1.3
#define	color_saturation 1.5

// Edges


uniform sampler2D u_Texture7; 
varying vec2 texture_coordinate2;
varying vec2 texture_coordinate3;
varying vec2 texture_coordinate4;
varying vec2 texture_coordinate5;
uniform float u_zFar;

#define 	USE_GRAIN 	= 0;	// film grain effect


float LinearizeDepth(vec2 uv)
{
float n = 8.0; // camera z near
float f = u_zFar; // camera z far
float z = texture2D(u_Texture7, uv).a;
return (2.0 * n) / (f + n - z * (f - n));
}

float crosshatchtfable[16] = {
	16,3,0,4,   
	16,4,0,3,
	16,3,0,4,
	16,4,0,3		
};

float crosshatchtable[16] = {
	0,5,16,9,   
	7,0,5,16,
	16,7,0,5,
	3,16,7,0		
};




// for the spectrumish effect in the bloom
vec4  shinetable[16] = {
	vec4(1.0f, 1.0f, 1.0f, 1),
	vec4(1.0f, 1.0f, 1.0f, 1),
	vec4(1.0f, 1.0f, 1.0f, 1),
	vec4(0.9f, 1.0f, 1.0f, 1),
	vec4(0.6f, 0.8f, 1.0f, 1),
	vec4(0.3f, 0.5f, 1.0f, 1),
	vec4(0.1f, 0.3f, 1.0f, 1),
	vec4(0.1f, 0.4f, 0.7f, 1),
	vec4(0.1f, 0.7f, 0.5f, 1),
	vec4(0.1f, 1.0f, 0.2f, 1),
	vec4(0.2f, 0.8f, 0.1f, 1),
	vec4(0.5f, 0.5f, 0.07f, 1),
	vec4(0.7f, 0.3f, 0.05f, 1),
	vec4(1.0f, 0.0f, 0.0f, 1)
};

vec3  spectrumtable[4] = {
	vec3(1.0f, 1.0f, 1.0f),
	vec3(0.0f, 0.6f, 1.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(1.0f, 0.0f, 0.0f)
};

vec4  spectralfade(float faed)  {
	vec4 yeah;
	float frade = faed;

	frade = pow(faed, 0.2f);

	vec3 spec0 = spectrumtable[0];
	vec3 spec1 = spectrumtable[1];
	vec3 spec2 = spectrumtable[2];
	vec3 spec3 = spectrumtable[3];

	//float sperc0 = * 0.25;

	//yeah

	if (frade < 0.25) yeah.rgb = spectrumtable[0];
	else if (frade < 0.5) yeah.rgb = spectrumtable[1];
	else if (frade < 0.75) yeah.rgb = spectrumtable[2];
	else if (frade < 1.0) yeah.rgb = spectrumtable[3];

	faed -= 0.3f;
	if (faed < 0) faed = 0;

	yeah.rgb = faed;
	return yeah;
}


float rand(float yeah){
	vec2 hah;
	float what = yeah; 
	float eh = texture_coordinate.x;
	float ea = texture_coordinate.y;
	hah.x = what + eh;
	hah.y = what + ea;

    return fract(sin(dot(hah.xy ,vec2(7445,7953))) *  45543.6353);
}

void main()
{
	
    	// Sampling The Texture And Passing It To The Frame Buffer 
	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 


	// Stubbed because I need to finish my shader.
}	