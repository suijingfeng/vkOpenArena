// "LeiFX" shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.


uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;

void main()
{
	vec2 px;	// shut
	px.x = u_ScreenToNextPixelX;
	px.y = u_ScreenToNextPixelX;
	float gammaed = 0.3;
	float leifx_linegamma = gammaed;
	float leifx_liney =  0.0078125;0.01568 / 2; // 0.0390625

    	// Sampling The Texture And Passing It To The Frame Buffer 
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	
 	float lines = mod(gl_FragCoord.y, 2.0);	// I realize the 3dfx doesn't actually line up the picture in the gamma process

	//if (lines < 1.0) leifx_linegamma = 0;
	if (lines < 1.0) 	leifx_liney = 0;

//	gl_FragColor.r += leifx_liney;	// it's a slight purple line.
//	gl_FragColor.b += leifx_liney;	// it's a slight purple line.

	float leifx_gamma = 1.0 + gammaed;// + leifx_linegamma;

	gl_FragColor.r = pow(gl_FragColor.r, 1.0 / leifx_gamma);
	gl_FragColor.g = pow(gl_FragColor.g, 1.0 / leifx_gamma);
	gl_FragColor.b = pow(gl_FragColor.b, 1.0 / leifx_gamma);
}	