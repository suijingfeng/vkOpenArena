#ifndef R_IMAGE_PROCESS_H_
#define R_IMAGE_PROCESS_H_


void R_SetColorMappings( float overbrightFactor, float gamma );
void R_LightScaleTexture (unsigned char* const dst, unsigned char* const in, uint32_t const nBytes);
void ResampleTexture(unsigned char *pOut, const unsigned int inwidth, const unsigned int inheight,
                        const unsigned char *pIn, const unsigned int outwidth, const unsigned int outheight);
void R_BlendOverTexture(unsigned char* data, const uint32_t pixelCount, const uint32_t l);
//void R_GammaCorrect(unsigned char* buffer, const unsigned int Size);
void R_MipMap(const unsigned char* in, uint32_t width, uint32_t height, unsigned char* out);
void R_MipMap2(const unsigned char* in, uint32_t inWidth, uint32_t inHeight, unsigned char* out);

#endif
