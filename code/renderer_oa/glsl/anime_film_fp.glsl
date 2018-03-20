// Anime film video cassette shader 
// 
// 	Copyright (C) 2014 leilei
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.



uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;

uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;

#define 	PIXELWIDTH 2.96f
#define		BLURAMOUNT 0.8f


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
	vec2 px;	// shut



	px.x = u_ScreenToNextPixelX;
	px.y = u_ScreenToNextPixelY;
    	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec2 chromar = texture_coordinate;
	vec2 chromag = texture_coordinate;
	vec2 chromab = texture_coordinate;

	
	
 	float lerma = gl_FragColor.rgb;
	float larma = lerma * 5;


	int errt;
	float blendy;	// to blend unblended with blend... trying to smooth the jag :(
	float blenda;
	vec4 smere;
	vec4 blursum;


	// 
	// Lame Chromatic stuff
	//
	vec2 resolution;
	resolution.x = u_ScreenSizeX;
	resolution.y = u_ScreenSizeY;


	resolution.x = texture_coordinate.x;
	resolution.y = texture_coordinate.y;
	
	vec3 chromaoffset;
	vec3 chromaoffsety;
	vec3 chromatest;
	vec2 chromanudge;
	float signal = 0.5f;
	float signaly = 0.5f;
	float phosph = 50.0f;
	vec3 fropsh;
	fropsh.r = phosph * (gl_FragColor.r);
	fropsh.g = phosph * (gl_FragColor.g);
	fropsh.b = phosph * (gl_FragColor.b);


	//
	// Really cheesy barrel effect
	//
	// By the way, this is completely wrong and will mess up in higher resolutions.
	// It's also slow as hell.
	//
	float chromabarrel = sin(resolution.x * 1.5) + 0.04f;
	if (chromabarrel > 0.5) chromabarrel = chromabarrel * -1 + 1;
	float chromabarrelx = sin(resolution.y * 1.5) + 0.04f;
	if (chromabarrelx > 0.5) chromabarrelx = chromabarrelx * -1 + 1;

	chromabarrel = pow (chromabarrel, 0.1f);
	chromabarrelx = pow (chromabarrelx, 0.1f);

	chromabarrel = pow(1 - chromabarrel, 1.5f);
	chromabarrelx = pow(1 - chromabarrelx, 1.5f);

	signal = chromabarrel * phosph;
	signaly = chromabarrelx * phosph;

	chromaoffset.r = px.x * (chromabarrel * fropsh.r);
	chromaoffset.g = 0;
	chromaoffset.b = px.x * ((chromabarrel * fropsh.b) * -1);

	chromaoffsety.r = px.y * (chromabarrelx * fropsh.r);
	chromaoffsety.g = 0;
	chromaoffsety.b = px.y * ((chromabarrelx * fropsh.b) * -1);

	vec2 chroar;
	vec2 chroag;
	vec2 chroab;

	chroar.x = chromaoffset.r; 	chroar.y = chromaoffsety.r;
	chroag.x = chromaoffset.g; 	chroag.y = chromaoffsety.g;
	chroab.x = chromaoffset.b; 	chroab.y = chromaoffsety.b;

	vec3 chroma4 = texture2D(u_Texture0, texture_coordinate+chroar);
	vec3 chroma5 = texture2D(u_Texture0, texture_coordinate+chroag);
	vec3 chroma6 = texture2D(u_Texture0, texture_coordinate+chroab);

	chromatest.rgb = vec3(chroma4.r, chroma5.g, chroma6.b);


// 
//
//	Soft luminance blur from the analog quantization
//	Also makes things look painterly and causes outlines to be softer
//
	int ertdiv = 4;
	for(errt=1; errt <28;errt+=ertdiv){

	float blendfactor;

	float yerp = 0.08f * errt * (larma * 0.6f + 0.03f);

	vec4 pixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, px.y * yerp)	); 
	vec4 pixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, -px.y * yerp)	); 
	vec4 pixel0 = texture2D(u_Texture0, texture_coordinate + vec2(0, 0)		); 
	vec4 pixel3 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, -px.y * yerp)	); 
	vec4 pixel4 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, px.y * yerp)	); 

// chroma blur lol

	vec4 rpixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, px.y * yerp) +chroar	); 
	vec4 rpixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, -px.y * yerp)+chroar	); 
	vec4 rpixel0 = texture2D(u_Texture0, texture_coordinate + vec2(0, 0)+chroab		); 
	vec4 rpixel3 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, -px.y * yerp)+chroar	); 
	vec4 rpixel4 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, px.y * yerp)+chroar	); 

	vec4 bpixel1 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, px.y * yerp)+chroab	); 
	vec4 bpixel2 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, -px.y * yerp)+chroab	); 
	vec4 bpixel0 = texture2D(u_Texture0, texture_coordinate + vec2(0, 0)+chroab		); 
	vec4 bpixel3 = texture2D(u_Texture0, texture_coordinate + vec2(px.x * yerp, -px.y * yerp)+chroab	); 
	vec4 bpixel4 = texture2D(u_Texture0, texture_coordinate + vec2(-px.x * yerp, px.y * yerp)+chroab	); 

	pixel1.r = rpixel1.r;
	pixel2.r = rpixel2.r;
	pixel0.r = rpixel0.r;
	pixel3.r = rpixel3.r;
	pixel4.r = rpixel4.r;

	pixel1.b = bpixel1.b;
	pixel2.b = bpixel2.b;
	pixel0.b = bpixel0.b;
	pixel3.b = bpixel3.b;
	pixel4.b = bpixel4.b;

	blursum += ((pixel1 + pixel2) + (pixel3 + pixel4)) / 4;



	}

	float grane = rand(blursum.r + blursum.g + blursum.b / 3);

	if (grane > 1.1f) grane = 1.1f;
	if (grane < 0.88f) grane = 0.88f;

	gl_FragColor.rgb = blursum / (errt / ertdiv)  * grane;
}	