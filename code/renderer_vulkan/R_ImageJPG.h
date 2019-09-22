#ifndef R_IMAGEJPG_H_
#define R_IMAGEJPG_H_

void RE_SaveJPG(char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer, int padding);
unsigned int RE_SaveJPGToBuffer(unsigned char *buffer, unsigned int bufSize, int quality, int image_width, int image_height, unsigned char *image_buffer, int padding);

#endif
