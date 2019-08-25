#pragma once
#include <d3d12.h>

enum Dx_Sampler_Index {
	SAMPLER_MIP_REPEAT,
	SAMPLER_MIP_CLAMP,
	SAMPLER_NOMIP_REPEAT,
	SAMPLER_NOMIP_CLAMP,
	SAMPLER_COUNT
};

struct Dx_Image {
	ID3D12Resource* texture = nullptr;
	Dx_Sampler_Index sampler_index = SAMPLER_COUNT;
};


struct Image_Upload_Data {
	byte* buffer;
	int buffer_size;
	int mip_levels;
	int base_level_width;
	int base_level_height;
};

Dx_Image upload_dx_image(const Image_Upload_Data& upload_data, bool repeat_texture, int image_index);
Dx_Image dx_create_image(int width, int height, int mip_levels, bool repeat_texture, int image_index);


void dx_create_sampler_descriptor(const DX_Sampler_Def& def, Dx_Sampler_Index sampler_index);