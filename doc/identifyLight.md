/*
static void R_CreateIdentityLightImage(void)
{
    #define	DEFAULT_SIZE 64
    uint32_t x,y;
	unsigned char data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; ++x)
    {
		for (y=0; y<DEFAULT_SIZE; ++y)
        {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	tr.identityLightImage = R_CreateImage("*identityLight", (unsigned char *)data, DEFAULT_SIZE, DEFAULT_SIZE,
            qfalse, qfalse, GL_REPEAT);
    #undef DEFAULT_SIZE
}
*/

