#ifndef R_LOAD_IMAGE_H_
#define R_LOAD_IMAGE_H_


/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadBMP( const char *name, unsigned char **pic, int *width, int *height );
void R_LoadJPG( const char *name, unsigned char **pic, int *width, int *height );
void R_LoadPCX( const char *name, unsigned char **pic, int *width, int *height );
void R_LoadPNG( const char *name, unsigned char **pic, int *width, int *height );
void R_LoadTGA( const char *name, unsigned char **pic, int *width, int *height );


#endif
