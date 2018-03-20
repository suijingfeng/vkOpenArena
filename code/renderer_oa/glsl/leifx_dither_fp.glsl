// "LeiFX" shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2015 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.


// Dithering method from xTibor (shadertoy)
// New undither version


uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

float erroredtable[16] = {
	16,4,13,1,   
	8,12,5,9,
	14,2,15,3,
	6,10,7,11		
};



// Rotated

#define		DITHERAMOUNT		0	// adjust this to make the dithering more opaque etc
#define		DITHERBIAS		0	// adjust this to make the dither more or less in between light/dark

void main()
{
	
    	// Sampling The Texture And Passing It To The Frame Buffer 
	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec4 what;

	float aht = 1.0f;

	what.x = aht;	// shut
	what.y = aht;	// up
	what.z = aht;	// warnings
	vec4 huh;

	// *****************
	// STAGE 0
	// Grab our sampled pixels for processing......
	// *****************
	vec2 px;	// shut
	float egh = 1.0f;
	px.x = u_ScreenToNextPixelX;
	px.y =  u_ScreenToNextPixelY;

	vec4	pe1 = texture2D(u_Texture0, texture_coordinate);			// first pixel...
	

	// *****************
	// STAGE 2
	// Reduce color depth of sampled pixels....
	// *****************

	{
	vec4 reduct;		// 16 bits per pixel (5-6-5)

	reduct.r = 32;
	reduct.g = 64;	// gotta be 16bpp for that green!
	reduct.b = 32;

  	pe1 = pow(pe1, what);  	pe1 *= reduct;  	pe1 = floor(pe1);	pe1 = pe1 / reduct;  	pe1 = pow(pe1, what);




	}

}	