// "LeiFX" shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// GLSL-based color adjustment/control
// because xorg/x11/sdl sucks.

uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;

uniform float u_CC_Overbright; 
uniform float u_CC_Brightness; 
uniform float u_CC_Gamma; 
uniform float u_CC_Contrast; 
uniform float u_CC_Saturation; 

uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;

void main()
{
	
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec3 color;
	vec3 colord;
	int coloredr;
	int coloredg;
	int coloredb;
	color.r = 1;
	color.g = 1;
	color.b = 1;
	int yeh = 0;
	float ohyes;



	// Overbrights
    	gl_FragColor *= (u_CC_Overbright + 1);

	// Gamma Correction
	float gamma = u_CC_Gamma;

	gl_FragColor.r = pow(gl_FragColor.r, 1.0 / gamma);
	gl_FragColor.g = pow(gl_FragColor.g, 1.0 / gamma);
	gl_FragColor.b = pow(gl_FragColor.b, 1.0 / gamma);

	//gl_FragColor += ((edge));


}	