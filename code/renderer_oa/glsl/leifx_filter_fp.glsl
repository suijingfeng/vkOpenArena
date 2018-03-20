// "LeiFX" shader - Pixel filtering process
// 
// 	Copyright (C) 2013-2015 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// New undither version

uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

uniform float u_CC_Brightness; // pass indicator from engine




//#define BOXFILTER;	// blind attempt at "22-bit" filter

#ifdef BOXFILTER

#define 	PIXELWIDTH 	1.0f

//#define		FILTCAP		0.085	// filtered pixel should not exceed this 
#define		FILTCAP		(32.0f / 255)	// filtered pixel should not exceed this 

#define		FILTCAPG	(FILTCAP / 2)


float filtertable_x[4] = {
	1, -1,-1,1   
};

float filtertable_y[4] = {
	-1,1,-1,1   
};


#else

#define 	PIXELWIDTH 	1.0f

//#define		FILTCAP		0.175	// filtered pixel should not exceed this 
#define		FILTCAP		(64.0f / 255)	// filtered pixel should not exceed this 

#define		FILTCAPG	(FILTCAP / 2)


float filtertable_x[4] = {
	1,-1,1,1  
};

float filtertable_y[4] = {
	0,0,0,0   
};

#endif

void main()
{
	vec2 px;	// shut
	float PIXELWIDTHH = u_ScreenToNextPixelX;
	int pass = int(u_CC_Brightness);

	px.x = u_ScreenToNextPixelX / 1.5;
	px.y = u_ScreenToNextPixelY / 1.5; 
    	// Sampling The Texture And Passing It To The Frame Buffer 
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec4 pixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * (filtertable_x[pass] * PIXELWIDTH), vec2(px.y * (PIXELWIDTH * filtertable_y[pass])))); 
	vec4 pixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * (filtertable_x[pass] * PIXELWIDTH), vec2(px.y * (PIXELWIDTH * filtertable_y[pass])))); 

	vec4 pixeldiff;			// Pixel difference for the dither check
	vec4 pixelmake;			// Pixel to make for adding to the buffer
	vec4 pixeldiffleft;		// Pixel to the left
	vec4 pixelblend;		

	{

		pixelmake.rgb = 0;
		pixeldiff.rgb = pixel2.rgb- gl_FragColor.rgb;

		if (pixeldiff.r > FILTCAP) 		pixeldiff.r = FILTCAP;
		if (pixeldiff.g > FILTCAPG) 		pixeldiff.g = FILTCAPG;
		if (pixeldiff.b > FILTCAP) 		pixeldiff.b = FILTCAP;

		if (pixeldiff.r < -FILTCAP) 		pixeldiff.r = -FILTCAP;
		if (pixeldiff.g < -FILTCAPG) 		pixeldiff.g = -FILTCAPG;
		if (pixeldiff.b < -FILTCAP) 		pixeldiff.b = -FILTCAP;

		pixelmake.rgb = (pixeldiff.rgb / 4);
		gl_FragColor.rgb= (gl_FragColor.rgb + pixelmake.rgb);
	}


}	
