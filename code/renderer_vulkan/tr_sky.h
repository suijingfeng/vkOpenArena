#ifndef TR_SKY_H_
#define TR_SKY_H_

#include "tr_image.h"

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6], *innerbox[6];
} skyParms_t;


#endif
