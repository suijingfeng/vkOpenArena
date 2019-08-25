#include "d3d12.h"
#include "tr_local.h"

#include "dx_image.h"
#include "dx_world.h"

// DX12
extern Dx_Instance	dx;				// shouldn't be cleared during ref re-init

static size_t qAlign(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
};



static void DX_UploadTexture(ID3D12Resource* texture, ID3D12Resource* upload_texture,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* regions, uint32_t mip_levels)
{
	ID3D12GraphicsCommandList* command_list;
	DX_CHECK(dx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dx.helper_command_allocator,
		nullptr, IID_PPV_ARGS(&command_list)));



	
	D3D12_RESOURCE_BARRIER imgBarrier;
	imgBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	imgBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	imgBarrier.Transition.pResource = texture;
	imgBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	imgBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	imgBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	

	command_list->ResourceBarrier(1, &imgBarrier);

	for (uint32_t i = 0; i < mip_levels; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = texture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = upload_texture;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = regions[i];

		command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}


	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;



	command_list->ResourceBarrier(1, &barrier);


	DX_CHECK(command_list->Close());

	ID3D12CommandList* command_lists_array[] = { command_list };
	dx.command_queue->ExecuteCommandLists(1, command_lists_array);
	dx_wait_device_idle();

	command_list->Release();
	dx.helper_command_allocator->Reset();
}


void dx_upload_image_data(ID3D12Resource* texture, int width, int height, int mip_levels,
	const uint8_t* pixels, int bytes_per_pixel)
{
	// Initialize subresource layouts int the upload texture.

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT regions[16];
	UINT64 buffer_size = 0;

	int w = width;
	int h = height;
	for (int i = 0; i < mip_levels; ++i)
	{
		regions[i].Offset = buffer_size;
		regions[i].Footprint.Format = texture->GetDesc().Format;
		regions[i].Footprint.Width = w;
		regions[i].Footprint.Height = h;
		regions[i].Footprint.Depth = 1;
		regions[i].Footprint.RowPitch = static_cast<UINT>(
			qAlign(w * bytes_per_pixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
		buffer_size += qAlign(regions[i].Footprint.RowPitch * h, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
		w >>= 1;
		if (w < 1)
			w = 1;
		h >>= 1;
		if (h < 1)
			h = 1;
	}


	//
	// Create upload upload texture.
	//

	D3D12_HEAP_PROPERTIES heap_properties;
	heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_properties.CreationNodeMask = 1;
	heap_properties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = buffer_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;


	ID3D12Resource* upload_texture;
	DX_CHECK(dx.device->CreateCommittedResource(
		&heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload_texture)));

	byte* pUpload_texture_data;
	DX_CHECK(upload_texture->Map(0, nullptr, reinterpret_cast<void**>(&pUpload_texture_data)));
	w = width;
	h = height;
	for (int i = 0; i < mip_levels; ++i)
	{
		byte* upload_subresource_base = pUpload_texture_data + regions[i].Offset;
		for (int y = 0; y < h; y++) {
			memcpy(upload_subresource_base + regions[i].Footprint.RowPitch * y, pixels, w * bytes_per_pixel);
			pixels += w * bytes_per_pixel;
		}
		w >>= 1;
		if (w < 1) w = 1;
		h >>= 1;
		if (h < 1) h = 1;
	}
	upload_texture->Unmap(0, nullptr);

	//
	// Copy data from upload texture to destination texture.
	//
	DX_UploadTexture(texture, upload_texture, regions, mip_levels);

	upload_texture->Release();
}

Dx_Image dx_create_image(int width, int height, int mip_levels, bool repeat_texture, int image_index)
{
	Dx_Image image;


	// create texture
	{
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = mip_levels;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES properties;
		properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 1;
		properties.VisibleNodeMask = 1;
	

		// DX_CHECK( )
			dx.device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&image.texture));
	}

	// create texture descriptor
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Texture2D.MipLevels = mip_levels;

		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = dx.srv_heap->GetCPUDescriptorHandleForHeapStart().ptr + image_index * dx.srv_descriptor_size;
		dx.device->CreateShaderResourceView(image.texture, &srv_desc, handle);

		dx_world.current_image_indices[glState.currenttmu] = image_index;
	}

	if (mip_levels > 0)
		image.sampler_index = repeat_texture ? SAMPLER_MIP_REPEAT : SAMPLER_MIP_CLAMP;
	else
		image.sampler_index = repeat_texture ? SAMPLER_NOMIP_REPEAT : SAMPLER_NOMIP_CLAMP;

	return image;
}

// DX12
Dx_Image upload_dx_image(const Image_Upload_Data& upload_data, bool repeat_texture, int image_index)
{
	int w = upload_data.base_level_width;
	int h = upload_data.base_level_height;

	bool has_alpha = false;
	for (int i = 0; i < w * h; ++i)
	{
		if (upload_data.buffer[i * 4 + 3] != 255)
		{
			has_alpha = true;
			break;
		}
	}

	Dx_Image image = dx_create_image(w, h, upload_data.mip_levels, repeat_texture, image_index);
	
	dx_upload_image_data(image.texture, w, h, upload_data.mip_levels, upload_data.buffer, 4);

	return image;
}



void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{

	GL_Bind(tr.scratchImage[client]);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if (cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;

		// DX12
		if (dx.active)
		{
			int image_index = tr.scratchImage[client]->index;
			Dx_Image& image = dx_world.images[image_index];
			image.texture->Release();
			image = dx_create_image(cols, rows, 1, false, image_index);
			dx_upload_image_data(image.texture, cols, rows, 1, data, 4);
		}
		else if (gl_active)
		{
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
	}
	else
	{
		if (dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression


			// DX12
			if (dx.active)
			{
				const Dx_Image& image = dx_world.images[tr.scratchImage[client]->index];
				dx_upload_image_data(image.texture, cols, rows, 1, data, 4);
			}
			else if (gl_active)
			{
				qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}

		}
	}
}