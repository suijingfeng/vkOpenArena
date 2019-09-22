#pragma once
#include "dx.h"
#include "dx_image.h"

struct Dx_World
{
	//
	// Resources.
	//
	int num_pipelines;
	DX_Pipeline_Def pipeline_defs[MAX_VK_PIPELINES];
	ID3D12PipelineState* pipelines[MAX_VK_PIPELINES];

	Dx_Image images[MAX_VK_IMAGES];

	//
	// State.
	//
	int current_image_indices[2];
	float modelview_transform[16];

};

float * R_GetModelViewMatPtr(void);

extern Dx_World	dx_world;		// this data is cleared during ref re-init