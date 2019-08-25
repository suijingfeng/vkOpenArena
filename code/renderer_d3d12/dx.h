#pragma once
#include <d3d12.h>

enum DX_Shader_Type {
	single_texture,
	multi_texture_mul,
	multi_texture_add
};

// used with cg_shadows == 2
enum DX_Shadow_Phase {
	disabled,
	shadow_edges_rendering,
	fullscreen_quad_rendering
};

enum DX_Depth_Range {
	normal, // [0..1]
	force_zero, // [0..0]
	force_one, // [1..1]
	weapon // [0..0.3]
};

struct DX_Sampler_Def {
	bool repeat_texture = false; // clamp/repeat texture addressing mode
	int gl_mag_filter = 0; // GL_XXX mag filter
	int gl_min_filter = 0; // GL_XXX min filter
};

constexpr int MAX_VK_IMAGES = 2048; // should be the same as MAX_DRAWIMAGES
constexpr int MAX_VK_PIPELINES = 1024;
constexpr int SWAPCHAIN_BUFFER_COUNT = 3;

struct DX_Pipeline_Def {
	DX_Shader_Type shader_type = DX_Shader_Type::single_texture;
	unsigned int state_bits = 0; // GLS_XXX flags
	int face_culling = 0;// cullType_t
	bool polygon_offset = false;
	bool clipping_plane = false;
	bool mirror = false;
	bool line_primitives = false;
	DX_Shadow_Phase shadow_phase = DX_Shadow_Phase::disabled;
};

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12Fence;
struct ID3D12PipelineState;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct IDXGISwapChain3;


#define DX_CHECK( function_call ) { \
	HRESULT hr = function_call; \
	if ( FAILED(hr) ) \
		ri.Error(ERR_FATAL, "Direct3D: error returned by %s", #function_call); \
}


//
// Initialization.
//
void dx_initialize(void * pWinContext);
void dx_shutdown();
void dx_release_resources();
void dx_wait_device_idle();

//
// Resources allocation.
//


ID3D12PipelineState* dx_find_pipeline(const DX_Pipeline_Def& def);

//
// Rendering setup.
//
void dx_clear_attachments(bool clear_depth_stencil, bool clear_color, float color[4]);
void dx_bind_geometry();
void dx_shade_geometry(ID3D12PipelineState* pipeline, bool multitexture, DX_Depth_Range depth_range, bool indexed, bool lines);


struct Dx_Instance
{
	bool active = false;

	ID3D12Device* device = nullptr;
	ID3D12CommandQueue* command_queue = nullptr;
	IDXGISwapChain3* swapchain = nullptr;
	UINT frame_index = 0;

	ID3D12CommandAllocator* command_allocator = nullptr;
	ID3D12CommandAllocator* helper_command_allocator = nullptr;
	ID3D12GraphicsCommandList* command_list = nullptr;
	
	ID3D12Fence* fence = nullptr;
	UINT64 fence_value = 0;
	HANDLE fence_event = NULL;

	ID3D12Resource* render_targets[SWAPCHAIN_BUFFER_COUNT];
	ID3D12Resource* depth_stencil_buffer = nullptr;

	ID3D12RootSignature* root_signature = nullptr;

	//
	// Descriptor heaps.
	//
	ID3D12DescriptorHeap* rtv_heap = nullptr;
	ID3D12DescriptorHeap* dsv_heap = nullptr;
	ID3D12DescriptorHeap* srv_heap = nullptr;
	ID3D12DescriptorHeap* sampler_heap = nullptr;

	UINT rtv_descriptor_size = 0;
	UINT srv_descriptor_size = 0;
	UINT sampler_descriptor_size = 0;

	//
	// Geometry buffers.
	//
	byte* vertex_buffer_ptr = nullptr; // pointer to mapped vertex buffer
	int xyz_elements = 0;
	int color_st_elements = 0;

	byte* index_buffer_ptr = nullptr; // pointer to mapped index buffer
	int index_buffer_offset = 0;

	ID3D12Resource* geometry_buffer = nullptr;

	//
	// Standard pipelines.
	//
	ID3D12PipelineState* skybox_pipeline = nullptr;

	// dim 0: 0 - front side, 1 - back size
	// dim 1: 0 - normal view, 1 - mirror view
	ID3D12PipelineState* shadow_volume_pipelines[2][2];
	ID3D12PipelineState* shadow_finish_pipeline = nullptr;

	// dim 0 is based on fogPass_t: 0 - corresponds to FP_EQUAL, 1 - corresponds to FP_LE.
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	ID3D12PipelineState* fog_pipelines[2][3][2];

	// dim 0 is based on dlight additive flag: 0 - not additive, 1 - additive
	// dim 1 is directly a cullType_t enum value.
	// dim 2 is a polygon offset value (0 - off, 1 - on).
	ID3D12PipelineState* dlight_pipelines[2][3][2];

	// debug visualization pipelines
	ID3D12PipelineState* tris_debug_pipeline = nullptr;
	ID3D12PipelineState* tris_mirror_debug_pipeline = nullptr;
	ID3D12PipelineState* normals_debug_pipeline = nullptr;
	ID3D12PipelineState* surface_debug_pipeline_solid = nullptr;
	ID3D12PipelineState* surface_debug_pipeline_outline = nullptr;
	ID3D12PipelineState* images_debug_pipeline = nullptr;
};
