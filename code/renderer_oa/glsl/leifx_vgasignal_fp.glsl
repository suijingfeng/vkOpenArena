uniform sampler2D u_Texture0; 
varying vec2 texture_coordinate;
uniform float u_ScreenToNextPixelX;
uniform float u_ScreenToNextPixelY;
uniform float u_ScreenSizeX;
uniform float u_ScreenSizeY;


// 	This is a simple shader that just blurs the whole picture with two given vectors, and scales its intensity
// based on the base res and the real resolution to imitate a degrading signal. Nothing really mindblowing other than to
// imitate the poor DACs of popular VGA cards that are not from a canadian company that specializes in image quality.

float BaseResX = 1024;
float BaseResY = 768;

float Blurs[3] = 	{1.6, 0.7, 0}; // to right and
int Passes = 12;

void main()
{
	
    	// Sampling The Texture And Passing It To The Frame Buffer 
	gl_FragColor = texture2D(u_Texture0, texture_coordinate); 

	vec4 what;

	what.x = 1;	// shut
	what.y = 1;	// up
	what.z = 1;	// warnings
	vec4 huh;


	vec2 px;	
	float egh = 1.0f / (BaseResX  * u_ScreenToNextPixelX);
	px.x = u_ScreenToNextPixelX;// * (BaseResX / u_ScreenSizeX);
	px.y = u_ScreenToNextPixelY;// * (BaseResY / u_ScreenSizeY);

	int passer;

	float passy;


	vec4	pe1 = texture2D(u_Texture0, texture_coordinate);
	vec4	signal;
	for (passer=0; passer<Passes; passer++)
		{
			passy = passer / Passes;
			float prs = (float(passer) / float(Passes));
			float shift1 = -px.x * (Blurs[0] *egh * prs);
			vec2 texcoord = texture_coordinate - (vec2(shift1, 0) * 0.3) + (vec2(shift1, 0) * 1.2);

			signal += texture2D(u_Texture0, texcoord) / Passes / 2;
		}

	for (passer=0; passer<Passes; passer++)
		{
			passy = passer / Passes;
			float prs = (float(passer) / float(Passes));
			float shift2 = -px.y * (Blurs[1] *egh  * prs);
			vec2 texcoord = texture_coordinate - (vec2(0, shift2)) * 0.5 + (vec2(0, shift2));

			signal += texture2D(u_Texture0, texcoord) / Passes / 2;
		}

	gl_FragColor = signal;
}	