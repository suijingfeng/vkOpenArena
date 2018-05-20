#ifndef GLIMPL_H_
#define GLIMPL_H_

//glimp
void GLimp_Init( glconfig_t *config );
void GLimp_EndFrame( void );
void GLimp_Shutdown( qboolean unloadDLL );
void *GLimp_GetProcAddress( const char *symbol );

void SetGammaImpl( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
void InitGammaImpl( glconfig_t *config );

#endif
