#include <D3d12.h>
#include <dxgi1_6.h>

#include "tr_local.h"
#include "dx_world.h"
#include "dx_utils.h"

#include "../sdl/win_public.h"
// DX12
extern Dx_Instance	dx;

const int VERTEX_CHUNK_SIZE = 512 * 1024;

const int XYZ_SIZE      = 4 * VERTEX_CHUNK_SIZE;
const int COLOR_SIZE    = 1 * VERTEX_CHUNK_SIZE;
const int ST0_SIZE      = 2 * VERTEX_CHUNK_SIZE;
const int ST1_SIZE      = 2 * VERTEX_CHUNK_SIZE;

const int XYZ_OFFSET    = 0;
const int COLOR_OFFSET  = XYZ_OFFSET + XYZ_SIZE;
const int ST0_OFFSET    = COLOR_OFFSET + COLOR_SIZE;
const int ST1_OFFSET    = ST0_OFFSET + ST0_SIZE;

const int VERTEX_BUFFER_SIZE = XYZ_SIZE + COLOR_SIZE + ST0_SIZE + ST1_SIZE;
const int INDEX_BUFFER_SIZE = 2 * 1024 * 1024;


static DXGI_FORMAT get_depth_format()
{
	// allway enable stencil
	return DXGI_FORMAT_D24_UNORM_S8_UINT;
}


void UpdateForSizeChange(UINT clientWidth, UINT clientHeight)
{

	dx.cl_win_width = clientWidth;
	dx.cl_win_height = clientHeight;

	glConfig.vidWidth = clientWidth;
	glConfig.vidHeight = clientHeight;
	glConfig.windowAspect = static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
}

unsigned int DX_GetRenderAreaWidth(void)
{
	return dx.cl_win_width;
}

unsigned int DX_GetRenderAreaHeight(void)
{
	return dx.cl_win_height;
}

// Set up the screen viewport and scissor rect to match the current window size and scene rendering resolution.
void UpdatePostViewAndScissor()
{
/*
	float viewWidthRatio = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Width) / m_width;
	float viewHeightRatio = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Height) / m_height;

	float x = 1.0f;
	float y = 1.0f;

	if (viewWidthRatio < viewHeightRatio)
	{
		// The scaled image's height will fit to the viewport's height and 
		// its width will be smaller than the viewport's width.
		x = viewWidthRatio / viewHeightRatio;
	}
	else
	{
		// The scaled image's width will fit to the viewport's width and 
		// its height may be smaller than the viewport's height.
		y = viewHeightRatio / viewWidthRatio;
	}

	m_postViewport.TopLeftX = m_width * (1.0f - x) / 2.0f;
	m_postViewport.TopLeftY = m_height * (1.0f - y) / 2.0f;
	m_postViewport.Width = x * m_width;
	m_postViewport.Height = y * m_height;

	m_postScissorRect.left = static_cast<LONG>(m_postViewport.TopLeftX);
	m_postScissorRect.right = static_cast<LONG>(m_postViewport.TopLeftX + m_postViewport.Width);
	m_postScissorRect.top = static_cast<LONG>(m_postViewport.TopLeftY);
	m_postScissorRect.bottom = static_cast<LONG>(m_postViewport.TopLeftY + m_postViewport.Height);
*/
}


void LoadSizeDependentResources()
{

	UpdatePostViewAndScissor();

	// Create frame resources.
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx.rtv_heap->GetCPUDescriptorHandleForHeapStart();

		// Create a RTV for each frame.
		for (UINT n = 0; n < SWAPCHAIN_BUFFER_COUNT; ++n)
		{
			dx.swapchain->GetBuffer(n, IID_PPV_ARGS(&dx.render_targets[n]));
			dx.device->CreateRenderTargetView(dx.render_targets[n], nullptr, rtvHandle);
			// rtvHandle.Offset(1, m_rtvDescriptorSize);

			// NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);
		}
	}

	// Update resolutions shown in app title.
	// UpdateTitle();

	// This is where you would create/resize intermediate render targets, depth stencils, or other resources
	// dependent on the window size.

}

void OnSizeChanged(UINT width, UINT height)
{

	// Determine if the swap buffers and other resources need to be resized or not.
	if ((width != glConfig.vidWidth || height != glConfig.vidHeight) )
	{
		// Flush all current GPU commands.
		// WaitForGpu();

		dx_wait_device_idle();


		// Release the resources holding references to the swap chain
		// (requirement of IDXGISwapChain::ResizeBuffers) and 
		// reset the frame fence values to the current fence value.
		for (UINT n = 0; n < SWAPCHAIN_BUFFER_COUNT; n++)
		{
			dx.render_targets[n]->Release();
			// m_fenceValues[n] = m_fenceValues[dx.frame_index];
		}
		// dx.fence_value = ;
		

		// Resize the swap chain to the desired dimensions.
		DXGI_SWAP_CHAIN_DESC desc = {};
		dx.swapchain->GetDesc(&desc);
		DX_CHECK( dx.swapchain->ResizeBuffers(SWAPCHAIN_BUFFER_COUNT,
			width, height, desc.BufferDesc.Format, desc.Flags) );



		BOOL fullscreenState;
		// Get the state associated with full-screen mode.
		dx.swapchain->GetFullscreenState(&fullscreenState, nullptr);
		// m_windowedMode = !fullscreenState;

		// Reset the frame index to the current back buffer index.
		dx.frame_index = dx.swapchain->GetCurrentBackBufferIndex();

		// Update the width, height, and aspect ratio member variables.
		UpdateForSizeChange(width, height);

		LoadSizeDependentResources();
	}

}


void RE_WinMessage(unsigned int msgType, int x, int y, int w, int h)
{
	ri.Printf(PRINT_ALL, "message type:%d from windows system: %d, %d, %d, %d",
		msgType, x, y, w, h);
	switch(msgType)
	{
		case 1: 
			// width, height
			OnSizeChanged(w, h);
			break;
		case 4:
		default:
			break;
	}
}


void RE_WaitRenderFinishCurFrame(void)
{
	dx_wait_device_idle();
}

static D3D12_HEAP_PROPERTIES get_heap_properties(D3D12_HEAP_TYPE heap_type)
{
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = heap_type;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 1;
	properties.VisibleNodeMask = 1;
	return properties;
}



static D3D12_RESOURCE_DESC get_buffer_desc(UINT64 size)
{
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	return desc;
}

ID3D12PipelineState* create_pipeline(const DX_Pipeline_Def& def);

static void DX_CreateCommandQueue(ID3D12CommandQueue** ppCmdQueue)
{
	D3D12_COMMAND_QUEUE_DESC queue_desc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
		D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };

	DX_CHECK( dx.device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(ppCmdQueue)) );

	ri.Printf( PRINT_ALL, "Command queue created. \n" );
}

bool CheckTearingSupport(void)
{
	bool tearingSupport = false;
#ifndef PIXSUPPORT
	IDXGIFactory6* factory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	BOOL allowTearing = FALSE;
	if (SUCCEEDED(hr))
	{
		hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	}

	tearingSupport = SUCCEEDED(hr) && allowTearing;


#else
	m_tearingSupport = TRUE;
#endif

	factory->Release();
	factory = nullptr;

	ri.Printf(PRINT_ALL, "tearing Support: %s \n" ,tearingSupport ? "Yes":"No");

	return tearingSupport;
}

// You cannot cast a DXGI_SWAP_CHAIN_DESC1 to a DXGI_SWAP_CHAIN_DESC and vice versa. 
// An application must explicitly use the IDXGISwapChain1::GetDesc1 method 
// to retrieve the newer version of the swap-chain description structure.
// In full-screen mode, there is a dedicated front buffer; in windowed mode, 
// the desktop is the front buffer.
static void DX_CreateSwapChain(ID3D12CommandQueue* pCmdQueue, DXGI_FORMAT fmt, UINT numTargetBuf,
	void* pContext, IDXGIFactory4* pFactory, IDXGISwapChain3 ** ppWwapchain)
{
	const WinVars_t * const pWinSys = (WinVars_t*)pContext;

	UpdateForSizeChange(pWinSys->winWidth, pWinSys->winHeight);

	bool isTearingSupport = CheckTearingSupport();
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	// width
	swap_chain_desc.Width = pWinSys->winWidth;
	// height
	swap_chain_desc.Height = pWinSys->winHeight;
	swap_chain_desc.Format = fmt;
	swap_chain_desc.Stereo = false;
	// A DXGI_SAMPLE_DESC structure that describes multi-sampling parameters.
	// This member is valid only with bit-block transfer (bitblt) model swap chains.
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	// A DXGI_USAGE-typed value that describes the surface usage and CPU access 
	// options for the back buffer. The back buffer can be used for shader input
	// or render-target output
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// A value that describes the number of buffers in the swap chain.
	// When you create a full-screen swap chain, you typically include
	// the front buffer in this value.
	swap_chain_desc.BufferCount = numTargetBuf;

	// A DXGI_SCALING-typed value that identifies resize behavior 
	// if the size of the back buffer is not equal to the target output.
	//
	// DXGI_SCALING_STRETCH: 
	// Directs DXGI to make the back-buffer contents scale to fit the presentation target size. 
	// This is the implicit behavior of DXGI when you call the IDXGIFactory::CreateSwapChain method.
	//
	// DXGI_SCALING_NONE
	// Directs DXGI to make the back-buffer contents appear without any scaling 
	// when the presentation target size is not equal to the back-buffer size. 
	// The top edges of the back buffer and presentation target are aligned together.
	// If the WS_EX_LAYOUTRTL style is associated with the HWND handle to the target
	// output window, the right edges of the back buffer and presentation target are 
	// aligned together; otherwise, the left edges are aligned together. 
	// All target area outside the back buffer is filled with window background color. 
	// This value specifies that all target areas outside the back buffer of a swap chain 
	// are filled with the background color that you specify in a call to
	// IDXGISwapChain1::SetBackgroundColor.
	//
	// DXGI_SCALING_ASPECT_RATIO_STRETCH:
	// Directs DXGI to make the back-buffer contents scale to fit the presentation target size,
	// while preserving the aspect ratio of the back-buffer. If the scaled back-buffer does not
	// fill the presentation area, it will be centered with black borders. 
	// This constant is supported on Windows Phone 8 and Windows 10.
	// Note that with legacy Win32 window swapchains, this works the same as DXGI_SCALING_STRETCH.
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	// A DXGI_SWAP_EFFECT-typed value that describes the presentation model
	// that is used by the swap chain and options for handling the contents
	// of the presentation buffer after presenting a surface. 
	//
	// FLIP_SEQUENTIAL: to specify that DXGI persist the contents of the back buffer
	// after you call IDXGISwapChain1::Present1, cannot be used with multisampling
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	// DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// The DXGI swap chain might change the display mode of an output when
	// making a full-screen transition. 
	// To enable the automatic display mode change:
	// DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	// swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING ;
	// It is recommended to always use the tearing flag when it is available.
	
	
	swap_chain_desc.Flags = isTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	/*
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
	fullscreen_desc.RefreshRate.Numerator = 125;
	fullscreen_desc.RefreshRate.Denominator = 1;
	fullscreen_desc.ScanlineOrdering =
		DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	fullscreen_desc.Scaling =
		DXGI_MODE_SCALING_UNSPECIFIED;
	fullscreen_desc.Windowed = true;
	*/

	IDXGISwapChain1* pTmpSwapchain;
	// IDXGIOutput * pOutputInfo = nullptr;



	DX_CHECK( pFactory->CreateSwapChainForHwnd(
		pCmdQueue,
		pWinSys->hWnd,
		&swap_chain_desc,
		nullptr,
		nullptr,
		&pTmpSwapchain
	));

	if (isTearingSupport)
	{
		// When tearing support is enabled we will handle ALT+Enter key presses in
		// the window message loop rather than let DXGI handle it by calling 
		// SetFullscreenState. this makes DXGI unable to respond to mode changes.
		
		// If the application switches to full - screen mode, DXGI will choose a 
		// fullscreen resolution to be the smallest supported resolution that is 
		// larger or the same size as the current back buffer size.

		// Applications can make some changes to make the transition from windowed 
		// to full screen more efficient. For example, on a WM_SIZE message, the 
		// application should release any outstanding swapchain back buffers, 
		// call IDXGISwapChain::ResizeBuffers, then reacquire the back buffers 
		// from the swap chain(s). This gives the swap chain(s) an opportunity to 
		// resize the back buffers, and/or recreate them to enable fullscreen 
		// flipping operation. If the application does not perform this sequence, 
		// DXGI will still make the fullscreen / windowed transition, but may be 
		// forced to use a stretch operation(since the back buffers may not be 
		// the correct size), which may be less efficient. Even if a stretch is 
		// not required, presentation may not be optimal because the back buffers
		// might not be directly interchangeable with the front buffer. 
		// Thus, a call to ResizeBuffers on WM_SIZE is always recommended, 
		// since WM_SIZE is always sent during a fullscreen transition.
		
		pFactory->MakeWindowAssociation(pWinSys->hWnd, DXGI_MWA_NO_ALT_ENTER);
	}

	// Although there are mechanisms by which an object can express the 
	// functionality it provides statically (before it is instantiated), 
	// the fundamental COM mechanism is to use the IUnknown method 
	// called QueryInterface.
	// Every interface is derived from IUnknown, so every interface has
	// an implementation of QueryInterface. Regardless of implementation, 
	// this method queries an object using the IID of the interface to 
	// which the caller wants a pointer. 
	// 
	// If the object supports that interface, QueryInterface retrieves a 
	// pointer to the interface, while also calling AddRef. 
	// Otherwise, it returns the E_NOINTERFACE error code.
	//
	// Note that you must obey Reference Counting rules at all times. 
	// If you call Release on an interface pointer to decrement the 
	// reference count to zero, you should not use that pointer again. 
	// Occasionally you may need to obtain a weak reference to an object 
	// (that is, you may wish to obtain a pointer to one of its interfaces
	// without incrementing the reference count), but it is not acceptable 
	// to do this by calling QueryInterface followed by Release. 
	// The pointer obtained in such a manner is invalid and should not be used. 
	// This more readily becomes apparent when _ATL_DEBUG_INTERFACES is defined, 
	// so defining this macro is a useful way of finding reference counting bugs.
	pTmpSwapchain->QueryInterface( IID_PPV_ARGS(ppWwapchain) );

	//
	dx.frame_index = (*ppWwapchain)->GetCurrentBackBufferIndex();

	pTmpSwapchain->Release();
	pTmpSwapchain = nullptr;




	ri.Printf(PRINT_ALL, " Swap chain created( %d x %d ). \n", pWinSys->winWidth, pWinSys->winHeight);
}


void DX_CreateDepthBuffer(uint32_t width, uint32_t height, DXGI_FORMAT DSFmt, ID3D12Resource** const pDSbuffer)
{
	D3D12_RESOURCE_DESC depth_desc{};
	depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depth_desc.Alignment = 0;
	depth_desc.Width = width;
	depth_desc.Height = height;
	depth_desc.DepthOrArraySize = 1;
	depth_desc.MipLevels = 1;
	depth_desc.Format = DSFmt;
	depth_desc.SampleDesc.Count = 1;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depth_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimized_clear_value;
	optimized_clear_value.Format = DSFmt;
	optimized_clear_value.DepthStencil.Depth = 1.0f;
	optimized_clear_value.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heaProp;
	heaProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heaProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heaProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heaProp.CreationNodeMask = 1;
	heaProp.VisibleNodeMask = 1;


	DX_CHECK( dx.device->CreateCommittedResource(
		&heaProp,
		D3D12_HEAP_FLAG_NONE,
		&depth_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimized_clear_value,
		IID_PPV_ARGS(pDSbuffer) )
	);

	ri.Printf(PRINT_ALL, " depth Stencil buffer created. \n ");

}

void DX_CreateDSBufferView(DXGI_FORMAT DSFmt, ID3D12Resource* const pDSbuffer, ID3D12DescriptorHeap* const pDsvHeap)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC view_desc{};
	view_desc.Format = DSFmt;
	view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	view_desc.Flags = D3D12_DSV_FLAG_NONE;

	dx.device->CreateDepthStencilView(pDSbuffer, &view_desc,
		pDsvHeap->GetCPUDescriptorHandleForHeapStart() );

	ri.Printf(PRINT_ALL, " View of depth Stencil buffer created. \n ");
}


void DX_CreateRtvHeap(uint32_t size, ID3D12DescriptorHeap** const pRtvHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	heap_desc.NumDescriptors = size;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_desc.NodeMask = 0;
	DX_CHECK( dx.device->CreateDescriptorHeap( &heap_desc, IID_PPV_ARGS(pRtvHeap) ));
	
	ri.Printf(PRINT_ALL, " render target view heap created. \n ");
}

void DX_CreateDsvHeap(uint32_t size, ID3D12DescriptorHeap** const pDsvHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	heap_desc.NumDescriptors = size;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heap_desc.NodeMask = 0;
	DX_CHECK(dx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(pDsvHeap)));

	ri.Printf(PRINT_ALL, " render Depth stencil view heap created. \n ");
}

void DX_CreateSRVheap(uint32_t size, ID3D12DescriptorHeap** const pSrvHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	heap_desc.NumDescriptors = size;
	// The descriptor heap for the combination of constant-buffer,
	// shader-resource, and unordered-access views.
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	DX_CHECK(dx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(pSrvHeap)));
}

void DX_CreateISheap(uint32_t size, ID3D12DescriptorHeap** const pSrvHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	heap_desc.NumDescriptors = size;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	DX_CHECK( dx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(pSrvHeap)) );
}


void DX_CreateDevice(IDXGIFactory4* const pFactory, ID3D12Device** const ppDevice)
{
	// A pointer to the video adapter to use when creating a device. 
	// Pass NULL to use the default adapter, which is the first adapter
	// that is enumerated by IDXGIFactory1::EnumAdapters.
	IDXGIAdapter1* pHardwareAdapter = nullptr;

	// max three GPU
	if (r_gpuIndex->integer < 0)
		r_gpuIndex->integer = 0;
	else if (r_gpuIndex->integer > 2)
		r_gpuIndex->integer = 2;

	HRESULT res = pFactory->EnumAdapters1(r_gpuIndex->integer, &pHardwareAdapter);
	if (res == S_OK)
	{
		res = D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(ppDevice));
		if (res == S_OK)
		{
			DXGI_ADAPTER_DESC1 desc;
			
			pHardwareAdapter->GetDesc1(&desc);
			
			if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				ri.Printf(PRINT_ALL, " Create device on Adapter %d successed! \n", r_gpuIndex->integer);
				// printWideStr(desc.Description);

				// printOutputInfo(pHardwareAdapter);

				ri.Printf(PRINT_ALL, "\n");

				pHardwareAdapter->Release();
				pHardwareAdapter = nullptr;
				return;
			}
		}
	}

	// create on r_gpuIndex failed, enum from the zero index!

	// The IDXGIAdapter1 interface represents a display sub-system 
	// (including one or more GPU's, DACs and video memory).
	UINT adapter_index = 0;
	// Enumerates both adapters (video cards) with or without outputs.
	// adapter_index: The index of the adapter to enumerate.
	// The address of a pointer to an IDXGIAdapter1 interface at the 
	// position specified by the adapter_index parameter.
	// EnumAdapterByGpuPreference(, , , );

	while (pFactory->EnumAdapters1(adapter_index, &pHardwareAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pHardwareAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}
		// check for 11_0 feature level support
		// Creates a device that represents the display adapter.
		if (SUCCEEDED(D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(ppDevice))))
		{
			r_gpuIndex->integer = adapter_index;

			ri.Printf(PRINT_ALL, " Create Device successed. Using Adapter: \n");

			printWideStr(desc.Description);

			ri.Printf(PRINT_ALL, "\n");

			break;
		}
		else
		{
			ri.Printf(PRINT_WARNING, " Failed create device on this adapter. \n");
		}
		++adapter_index;
	}

	pHardwareAdapter->Release();
	pHardwareAdapter = nullptr;
}





void dx_initialize(void * pWinContext)
{
	ri.Printf(PRINT_ALL, " d3d12 Initial. \n");
	// enable validation in debug configuration
#ifndef NDEBUG
	ID3D12Debug* debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
		debug_controller->Release();
	}
#endif

	// Enables creating Microsoft DirectX Graphics Infrastructure (DXGI) objects
	// Use a DXGI 1.1 factory to generate objects that enumerate adapters, 
	// create swap chains, and associate a window with the alt+enter key sequence
	// for toggling to and from the full-screen display mode.

	// If the CreateDXGIFactory1 function succeeds, 
	// the reference count on the IDXGIFactory1 interface is incremented.
	//
	// To avoid a memory leak, when you finish using the interface,
	// call the IDXGIFactory1::Release method to release the interface.

	// To create a Microsoft DirectX Graphics Infrastructure (DXGI) 1.2 factory interface,
	// pass IDXGIFactory2 into either the CreateDXGIFactory or CreateDXGIFactory1 function
	// or call QueryInterface from a factory object that either CreateDXGIFactory or
	// CreateDXGIFactory1 returns.
	// IDXGIFactory4 Enables creating Microsoft DirectX Graphics Infrastructure (DXGI) objects.
	IDXGIFactory4* pFactory;
	// Creates a DXGI 1.1 factory that can be used to generate other DXGI objects.
	// Use IDXGIFactory or IDXGIFactory1, but not both in an application.
	// The CreateDXGIFactory function does not exist for Windows Store apps. 
	// Instead, Windows Store apps use the CreateDXGIFactory1 function. 
#ifndef NDEBUG
	// This function accepts a flag indicating whether DXGIDebug.dll is loaded.
	// The function otherwise behaves identically to CreateDXGIFactory1.
	DX_CHECK( CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pFactory)) ); 
#else
	DX_CHECK( CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory)) );
#endif
	

	DX_CreateDevice(pFactory, &dx.device);

	// allway enable stencil

	DX_CreateCommandQueue( &dx.command_queue );

	// Create swap chain.
	DX_CreateSwapChain(dx.command_queue, DXGI_FORMAT_R8G8B8A8_UNORM, 
		SWAPCHAIN_BUFFER_COUNT, pWinContext, pFactory, &dx.swapchain);


	for (int i = 0; i < SWAPCHAIN_BUFFER_COUNT; ++i)
	{
		DX_CHECK(dx.swapchain->GetBuffer(i, IID_PPV_ARGS(&dx.render_targets[i])));
	}
	

	// If the CreateDXGIFactory1 function succeeds, the reference count 
	// on the IDXGIFactory1 interface is incremented. 
	// To avoid a memory leak, when you finish using the interface, 
	// call the IDXGIFactory1::Release method to release the interface.
	// DXGI 1.1 support is required, which is available on Windows 7.

	pFactory->Release();
	pFactory = nullptr;

	//
	// Create command allocators and command list.
	//
	{

		DX_CHECK(dx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&dx.command_allocator)));

		DX_CHECK(dx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&dx.helper_command_allocator)));

		DX_CHECK(dx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dx.command_allocator, nullptr,
			IID_PPV_ARGS(&dx.command_list)));
		DX_CHECK(dx.command_list->Close());
	}

	//
	// Create synchronization objects.
	//
	{
		DX_CHECK(dx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&dx.fence)));
		dx.fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	//
	// Create descriptor heaps.
	//
	{
		// RTV heap.
		DX_CreateRtvHeap(SWAPCHAIN_BUFFER_COUNT, &dx.rtv_heap);
		// DSV heap.
		DX_CreateDsvHeap(1, &dx.dsv_heap);
		// SRV heap.
		DX_CreateSRVheap(MAX_DRAWIMAGES, &dx.srv_heap);
		// Image Sampler heap.
		DX_CreateISheap(SAMPLER_COUNT, &dx.sampler_heap);

		dx.rtv_descriptor_size = dx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		dx.srv_descriptor_size = dx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dx.sampler_descriptor_size = dx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	//
	// Create descriptors.
	//
	{
		// RTV descriptors.
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx.rtv_heap->GetCPUDescriptorHandleForHeapStart();
			for (int i = 0; i < SWAPCHAIN_BUFFER_COUNT; ++i)
			{
				dx.device->CreateRenderTargetView(dx.render_targets[i], nullptr, rtv_handle);
				rtv_handle.ptr += dx.rtv_descriptor_size;
			}
		}

		// Samplers.
		{
			{
				DX_Sampler_Def def;
				def.repeat_texture = true;
				def.gl_mag_filter = gl_filter_max;
				def.gl_min_filter = gl_filter_min;
				dx_create_sampler_descriptor(def, SAMPLER_MIP_REPEAT);
			}
			{
				DX_Sampler_Def def;
				def.repeat_texture = false;
				def.gl_mag_filter = gl_filter_max;
				def.gl_min_filter = gl_filter_min;
				dx_create_sampler_descriptor(def, SAMPLER_MIP_CLAMP);
			}
			{
				DX_Sampler_Def def;
				def.repeat_texture = true;
				def.gl_mag_filter = GL_LINEAR;
				def.gl_min_filter = GL_LINEAR;
				dx_create_sampler_descriptor(def, SAMPLER_NOMIP_REPEAT);
			}
			{
				DX_Sampler_Def def;
				def.repeat_texture = false;
				def.gl_mag_filter = GL_LINEAR;
				def.gl_min_filter = GL_LINEAR;
				dx_create_sampler_descriptor(def, SAMPLER_NOMIP_CLAMP);
			}
		}
	}

	//
	// Create depth buffer resources.
	//
	DX_CreateDepthBuffer(glConfig.vidWidth, glConfig.vidHeight, get_depth_format(), &dx.depth_stencil_buffer);
	DX_CreateDSBufferView(get_depth_format(), dx.depth_stencil_buffer, dx.dsv_heap);

	//
	// Create root signature.
	//
	{
		D3D12_DESCRIPTOR_RANGE ranges[4] = {};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 0;

		ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[2].NumDescriptors = 1;
		ranges[2].BaseShaderRegister = 1;

		ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		ranges[3].NumDescriptors = 1;
		ranges[3].BaseShaderRegister = 1;

		D3D12_ROOT_PARAMETER root_parameters[5] {};

		root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		root_parameters[0].Constants.ShaderRegister = 0;
		root_parameters[0].Constants.Num32BitValues = 32;
		root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		for (int i = 1; i < 5; ++i)
		{
			root_parameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			root_parameters[i].DescriptorTable.NumDescriptorRanges = 1;
			root_parameters[i].DescriptorTable.pDescriptorRanges = &ranges[i-1];
			root_parameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}

		D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
		// root_signature_desc.NumParameters = _countof(root_parameters);
		root_signature_desc.NumParameters = 5;
		root_signature_desc.pParameters = root_parameters;
		root_signature_desc.NumStaticSamplers = 0;
		root_signature_desc.pStaticSamplers = nullptr;
		root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature;
		ID3DBlob* error;
		DX_CHECK(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1,
			&signature, &error));
		DX_CHECK(dx.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&dx.root_signature)));

		if (signature != nullptr)
			signature->Release();
		if (error != nullptr)
			error->Release();
	}

	//
	// Geometry buffers.
	//
	{
		// store geometry in upload heap since Q3 regenerates it every frame
		DX_CHECK(dx.device->CreateCommittedResource(
			&get_heap_properties(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&get_buffer_desc(VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&dx.geometry_buffer)));

		void* p_data;
		D3D12_RANGE read_range{};
        DX_CHECK(dx.geometry_buffer->Map(0, &read_range, &p_data));

		dx.vertex_buffer_ptr = static_cast<byte*>(p_data);

		assert((VERTEX_BUFFER_SIZE & 0xffff) == 0); // index buffer offset should be 64K aligned.
		dx.index_buffer_ptr = static_cast<byte*>(p_data) + VERTEX_BUFFER_SIZE;
	}

	//
	// Standard pipelines.
	//
	{
		// skybox
		{
			DX_Pipeline_Def def;
			def.shader_type = single_texture;
			def.state_bits = 0;
			def.face_culling = CT_FRONT_SIDED;
			def.polygon_offset = false;
			def.clipping_plane = false;
			def.mirror = false;
			dx.skybox_pipeline = create_pipeline(def);
		}

		// Q3 stencil shadows
		{
			{
				DX_Pipeline_Def def;
				def.polygon_offset = false;
				def.state_bits = 0;
				def.shader_type = single_texture;
				def.clipping_plane = false;
				def.shadow_phase = DX_Shadow_Phase::shadow_edges_rendering;

				cullType_t cull_types[2] = {CT_FRONT_SIDED, CT_BACK_SIDED};
				bool mirror_flags[2] = {false, true};

				for (int i = 0; i < 2; i++) {
					def.face_culling = cull_types[i];
					for (int j = 0; j < 2; j++) {
						def.mirror = mirror_flags[j];
						dx.shadow_volume_pipelines[i][j] = create_pipeline(def);
					}
				}
			}

			{
				DX_Pipeline_Def def;
				def.face_culling = CT_FRONT_SIDED;
				def.polygon_offset = false;
				def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
				def.shader_type = single_texture;
				def.clipping_plane = false;
				def.mirror = false;
				def.shadow_phase = DX_Shadow_Phase::fullscreen_quad_rendering;
				dx.shadow_finish_pipeline = create_pipeline(def);
			}
		}

		// fog and dlights
		{
			DX_Pipeline_Def def;
			def.shader_type = single_texture;
			def.clipping_plane = false;
			def.mirror = false;

			unsigned int fog_state_bits[2] = {
				GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL,
				GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
			};
			unsigned int dlight_state_bits[2] = {
				GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL,
				GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL
			};
			bool polygon_offset[2] = {false, true};

			for (int i = 0; i < 2; ++i)
			{
				unsigned fog_state = fog_state_bits[i];
				unsigned dlight_state = dlight_state_bits[i];

				for (int j = 0; j < 3; ++j)
				{
					def.face_culling = j; // cullType_t value

					for (int k = 0; k < 2; ++k)
					{
						def.polygon_offset = polygon_offset[k];

						def.state_bits = fog_state;
						dx.fog_pipelines[i][j][k] = create_pipeline(def);

						def.state_bits = dlight_state;
						dx.dlight_pipelines[i][j][k] = create_pipeline(def);
					}
				}
			}
		}

		// debug pipelines
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;
			dx.tris_debug_pipeline = create_pipeline(def);
		}
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;
			def.face_culling = CT_BACK_SIDED;
			dx.tris_mirror_debug_pipeline = create_pipeline(def);
		}
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_DEPTHMASK_TRUE;
			def.line_primitives = true;
			dx.normals_debug_pipeline = create_pipeline(def);
		}
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			dx.surface_debug_pipeline_solid = create_pipeline(def);
		}
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			def.line_primitives = true;
			dx.surface_debug_pipeline_outline = create_pipeline(def);
		}
		{
			DX_Pipeline_Def def;
			def.state_bits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			dx.images_debug_pipeline = create_pipeline(def);
		}
	}

	dx.active = true;
}


void dx_shutdown()
{
	CloseHandle(dx.fence_event);

	for (int i = 0; i < SWAPCHAIN_BUFFER_COUNT; ++i)
	{
		dx.render_targets[i]->Release();
	}
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			dx.shadow_volume_pipelines[i][j]->Release();
		}
	}
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 2; k++) {
				dx.fog_pipelines[i][j][k]->Release();
				dx.dlight_pipelines[i][j][k]->Release();
			}
		}
	}

	dx.swapchain->Release();
	dx.command_allocator->Release();
	dx.helper_command_allocator->Release();
	dx.rtv_heap->Release();
	dx.srv_heap->Release();
	dx.sampler_heap->Release();
	dx.root_signature->Release();
	dx.command_queue->Release();
	dx.command_list->Release();
	dx.fence->Release();
	dx.depth_stencil_buffer->Release();
	dx.dsv_heap->Release();
	dx.geometry_buffer->Release();
	dx.skybox_pipeline->Release();
	dx.shadow_finish_pipeline->Release();
	dx.tris_debug_pipeline->Release();
	dx.tris_mirror_debug_pipeline->Release();
	dx.normals_debug_pipeline->Release();
	dx.surface_debug_pipeline_solid->Release();
	dx.surface_debug_pipeline_outline->Release();
	dx.images_debug_pipeline->Release();

	dx.device->Release();

	memset(&dx, 0, sizeof(dx));
}

void dx_release_resources()
{
	ri.Printf(PRINT_ALL, " Release DirectX12 Resources. \n");
	dx_wait_device_idle();

	for (int i = 0; i < dx_world.num_pipelines; ++i)
	{
		dx_world.pipelines[i]->Release();
	}

	for (int i = 0; i < MAX_VK_IMAGES; ++i)
	{
		if (dx_world.images[i].texture != nullptr) {
			dx_world.images[i].texture->Release();
		}
	}

	memset(&dx_world, 0, sizeof(dx_world));

	// Reset geometry buffer's current offsets.
	dx.xyz_elements = 0;
	dx.color_st_elements = 0;
	dx.index_buffer_offset = 0;
}

void dx_wait_device_idle()
{
	++dx.fence_value;
	DX_CHECK(dx.command_queue->Signal(dx.fence, dx.fence_value));
	DX_CHECK(dx.fence->SetEventOnCompletion(dx.fence_value, dx.fence_event));
	WaitForSingleObject(dx.fence_event, INFINITE);
}




static ID3D12PipelineState* create_pipeline(const DX_Pipeline_Def& def)
{
	// single texture VS
	extern unsigned char single_texture_vs[];
	extern long long single_texture_vs_size;

	extern unsigned char single_texture_clipping_plane_vs[];
	extern long long single_texture_clipping_plane_vs_size;

	// multi texture VS
	extern unsigned char multi_texture_vs[];
	extern long long multi_texture_vs_size;

	extern unsigned char multi_texture_clipping_plane_vs[];
	extern long long multi_texture_clipping_plane_vs_size;

	// single texture PS
	extern unsigned char single_texture_ps[];
	extern long long single_texture_ps_size;

	extern unsigned char single_texture_gt0_ps[];
	extern long long single_texture_gt0_ps_size;

	extern unsigned char single_texture_lt80_ps[];
	extern long long single_texture_lt80_ps_size;

	extern unsigned char single_texture_ge80_ps[];
	extern long long single_texture_ge80_ps_size;

	// multi texture mul PS
	extern unsigned char multi_texture_mul_ps[];
	extern long long multi_texture_mul_ps_size;

	extern unsigned char multi_texture_mul_gt0_ps[];
	extern long long multi_texture_mul_gt0_ps_size;

	extern unsigned char multi_texture_mul_lt80_ps[];
	extern long long multi_texture_mul_lt80_ps_size;

	extern unsigned char multi_texture_mul_ge80_ps[];
	extern long long multi_texture_mul_ge80_ps_size;

	// multi texture add PS
	extern unsigned char multi_texture_add_ps[];
	extern long long multi_texture_add_ps_size;

	extern unsigned char multi_texture_add_gt0_ps[];
	extern long long multi_texture_add_gt0_ps_size;

	extern unsigned char multi_texture_add_lt80_ps[];
	extern long long multi_texture_add_lt80_ps_size;

	extern unsigned char multi_texture_add_ge80_ps[];
	extern long long multi_texture_add_ge80_ps_size;


	D3D12_SHADER_BYTECODE vs_bytecode;
	D3D12_SHADER_BYTECODE ps_bytecode;

	if (def.shader_type == single_texture)
	{
		if (def.clipping_plane)
		{
			vs_bytecode.pShaderBytecode = single_texture_clipping_plane_vs;
			vs_bytecode.BytecodeLength = single_texture_clipping_plane_vs_size;
		} 
		else
		{
			// vs_bytecode = BYTECODE(single_texture_vs);
			vs_bytecode.pShaderBytecode = single_texture_vs;
			vs_bytecode.BytecodeLength = single_texture_vs_size;
		}

		// GET_PS_BYTECODE(single_texture)
		if ((def.state_bits & GLS_ATEST_BITS) == 0)
		{
			ps_bytecode.pShaderBytecode = single_texture_ps;
			ps_bytecode.BytecodeLength = single_texture_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GT_0)
		{
			//ps_bytecode = BYTECODE(single_texture_gt0_ps);
			ps_bytecode.pShaderBytecode = single_texture_gt0_ps;
			ps_bytecode.BytecodeLength = single_texture_gt0_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_LT_80)
		{
			// ps_bytecode = BYTECODE(single_texture_lt80_ps);
			ps_bytecode.pShaderBytecode = single_texture_lt80_ps;
			ps_bytecode.BytecodeLength = single_texture_lt80_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GE_80)
		{
			//ps_bytecode = BYTECODE(single_texture_ge80_ps);
			ps_bytecode.pShaderBytecode = single_texture_ge80_ps;
			ps_bytecode.BytecodeLength = single_texture_ge80_ps_size;
		}
		else {
			ri.Error(ERR_DROP, "create_pipeline: invalid alpha test state bits\n");
		}
	}
	else if (def.shader_type == multi_texture_mul)
	{
		if (def.clipping_plane)
		{
			// vs_bytecode = BYTECODE(multi_texture_clipping_plane_vs);
			vs_bytecode.pShaderBytecode = multi_texture_clipping_plane_vs;
			vs_bytecode.BytecodeLength = multi_texture_clipping_plane_vs_size;
		} 
		else
		{
			// vs_bytecode = BYTECODE(multi_texture_vs);
			vs_bytecode.pShaderBytecode = multi_texture_vs;
			vs_bytecode.BytecodeLength = multi_texture_vs_size;
		}


		if ((def.state_bits & GLS_ATEST_BITS) == 0)
		{
			ps_bytecode.pShaderBytecode = multi_texture_mul_ps;
			ps_bytecode.BytecodeLength = multi_texture_mul_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GT_0)
		{
			//ps_bytecode = BYTECODE(single_texture_gt0_ps);
			ps_bytecode.pShaderBytecode = multi_texture_mul_gt0_ps;
			ps_bytecode.BytecodeLength = multi_texture_mul_gt0_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_LT_80)
		{
			// ps_bytecode = BYTECODE(single_texture_lt80_ps);
			ps_bytecode.pShaderBytecode = multi_texture_mul_lt80_ps;
			ps_bytecode.BytecodeLength = multi_texture_mul_lt80_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GE_80)
		{
			//ps_bytecode = BYTECODE(single_texture_ge80_ps);
			ps_bytecode.pShaderBytecode = multi_texture_mul_ge80_ps;
			ps_bytecode.BytecodeLength = multi_texture_mul_ge80_ps_size;
		}
		else {
			ri.Error(ERR_DROP, "create_pipeline: invalid alpha test state bits\n");
		}
	}
	else if (def.shader_type == multi_texture_add)
	{
		if (def.clipping_plane)
		{
			// vs_bytecode = BYTECODE(multi_texture_clipping_plane_vs);
			vs_bytecode.pShaderBytecode = multi_texture_clipping_plane_vs;
			vs_bytecode.BytecodeLength = multi_texture_clipping_plane_vs_size;
		}
		else
		{
			// vs_bytecode = BYTECODE(multi_texture_vs);
			vs_bytecode.pShaderBytecode = multi_texture_vs;
			vs_bytecode.BytecodeLength = multi_texture_vs_size;
		}


		if ((def.state_bits & GLS_ATEST_BITS) == 0)
		{
			ps_bytecode.pShaderBytecode = multi_texture_add_ps;
			ps_bytecode.BytecodeLength = multi_texture_add_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GT_0)
		{
			//ps_bytecode = BYTECODE(single_texture_gt0_ps);
			ps_bytecode.pShaderBytecode = multi_texture_add_gt0_ps;
			ps_bytecode.BytecodeLength = multi_texture_add_gt0_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_LT_80)
		{
			// ps_bytecode = BYTECODE(single_texture_lt80_ps);
			ps_bytecode.pShaderBytecode = multi_texture_add_lt80_ps;
			ps_bytecode.BytecodeLength = multi_texture_add_lt80_ps_size;
		}
		else if (def.state_bits & GLS_ATEST_GE_80)
		{
			//ps_bytecode = BYTECODE(single_texture_ge80_ps);
			ps_bytecode.pShaderBytecode = multi_texture_add_ge80_ps;
			ps_bytecode.BytecodeLength = multi_texture_add_ge80_ps_size;
		}
		else {
			ri.Error(ERR_DROP, "create_pipeline: invalid alpha test state bits\n");
		}
	}


	// Vertex elements.
	D3D12_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	//
	// Blend.
	//
	D3D12_BLEND_DESC blend_state;
	blend_state.AlphaToCoverageEnable = FALSE;
	blend_state.IndependentBlendEnable = FALSE;
	auto& rt_blend_desc = blend_state.RenderTarget[0];
	rt_blend_desc.BlendEnable = (def.state_bits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? TRUE : FALSE;
	rt_blend_desc.LogicOpEnable = FALSE;

	if (rt_blend_desc.BlendEnable) {
		switch (def.state_bits & GLS_SRCBLEND_BITS) {
			case GLS_SRCBLEND_ZERO:
				rt_blend_desc.SrcBlend = D3D12_BLEND_ZERO;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				rt_blend_desc.SrcBlend = D3D12_BLEND_ONE;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				rt_blend_desc.SrcBlend = D3D12_BLEND_DEST_COLOR;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				rt_blend_desc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				rt_blend_desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				rt_blend_desc.SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				rt_blend_desc.SrcBlend = D3D12_BLEND_DEST_ALPHA;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				rt_blend_desc.SrcBlend = D3D12_BLEND_INV_DEST_ALPHA;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				rt_blend_desc.SrcBlend = D3D12_BLEND_SRC_ALPHA_SAT;
				rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA_SAT;
				break;
			default:
				ri.Error( ERR_DROP, "create_pipeline: invalid src blend state bits\n" );
				break;
		}
		switch (def.state_bits & GLS_DSTBLEND_BITS) {
			case GLS_DSTBLEND_ZERO:
				rt_blend_desc.DestBlend = D3D12_BLEND_ZERO;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				rt_blend_desc.DestBlend = D3D12_BLEND_ONE;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				rt_blend_desc.DestBlend = D3D12_BLEND_SRC_COLOR;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				rt_blend_desc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				rt_blend_desc.DestBlend = D3D12_BLEND_SRC_ALPHA;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				rt_blend_desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				rt_blend_desc.DestBlend = D3D12_BLEND_DEST_ALPHA;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				rt_blend_desc.DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
				rt_blend_desc.DestBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "create_pipeline: invalid dst blend state bits\n" );
				break;
		}
	}
	rt_blend_desc.BlendOp = D3D12_BLEND_OP_ADD;
	rt_blend_desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rt_blend_desc.LogicOp = D3D12_LOGIC_OP_COPY;
	rt_blend_desc.RenderTargetWriteMask = (def.shadow_phase == DX_Shadow_Phase::shadow_edges_rendering) ? 0 : D3D12_COLOR_WRITE_ENABLE_ALL;

	//
	// Rasteriazation state.
	//
	D3D12_RASTERIZER_DESC rasterization_state = {};
	rasterization_state.FillMode = (def.state_bits & GLS_POLYMODE_LINE) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

	if (def.face_culling == CT_TWO_SIDED)
		rasterization_state.CullMode = D3D12_CULL_MODE_NONE;
	else if (def.face_culling == CT_FRONT_SIDED)
		rasterization_state.CullMode = (def.mirror ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK);
	else if (def.face_culling == CT_BACK_SIDED)
		rasterization_state.CullMode = (def.mirror ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_FRONT);
	else
		ri.Error(ERR_DROP, "create_pipeline: invalid face culling mode\n");

	rasterization_state.FrontCounterClockwise = FALSE; // Q3 defaults to clockwise vertex order
	rasterization_state.DepthBias = def.polygon_offset ? r_offsetUnits->integer : 0;
	rasterization_state.DepthBiasClamp = 0.0f;
	rasterization_state.SlopeScaledDepthBias = def.polygon_offset ? r_offsetFactor->value : 0.0f;
	rasterization_state.DepthClipEnable = TRUE;
	rasterization_state.MultisampleEnable = FALSE;
	rasterization_state.AntialiasedLineEnable = FALSE;
	rasterization_state.ForcedSampleCount = 0;
	rasterization_state.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//
	// Depth/stencil state.
	//
	D3D12_DEPTH_STENCIL_DESC depth_stencil_state = {};
	depth_stencil_state.DepthEnable = (def.state_bits & GLS_DEPTHTEST_DISABLE) ? FALSE : TRUE;
	depth_stencil_state.DepthWriteMask = (def.state_bits & GLS_DEPTHMASK_TRUE) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	depth_stencil_state.DepthFunc = (def.state_bits & GLS_DEPTHFUNC_EQUAL) ? D3D12_COMPARISON_FUNC_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depth_stencil_state.StencilEnable = (def.shadow_phase != DX_Shadow_Phase::disabled) ? TRUE : FALSE;
	depth_stencil_state.StencilReadMask = 255;
	depth_stencil_state.StencilWriteMask = 255;

	if (def.shadow_phase == DX_Shadow_Phase::shadow_edges_rendering) {
		depth_stencil_state.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		depth_stencil_state.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		depth_stencil_state.FrontFace.StencilPassOp = (def.face_culling == CT_FRONT_SIDED) ? D3D12_STENCIL_OP_INCR_SAT : D3D12_STENCIL_OP_DECR_SAT;
		depth_stencil_state.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		depth_stencil_state.BackFace = depth_stencil_state.FrontFace;
	} else if (def.shadow_phase == DX_Shadow_Phase::fullscreen_quad_rendering) {
		depth_stencil_state.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		depth_stencil_state.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		depth_stencil_state.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		depth_stencil_state.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;

		depth_stencil_state.BackFace = depth_stencil_state.FrontFace;
	} else {
		depth_stencil_state.FrontFace = {};
		depth_stencil_state.BackFace = {};
	}

	//
	// Create pipeline state.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
	pipeline_desc.pRootSignature = dx.root_signature;
	pipeline_desc.VS = vs_bytecode;
	pipeline_desc.PS = ps_bytecode;
	pipeline_desc.BlendState = blend_state;
	pipeline_desc.SampleMask = UINT_MAX;
	pipeline_desc.RasterizerState = rasterization_state;
	pipeline_desc.DepthStencilState = depth_stencil_state;
	pipeline_desc.InputLayout = { input_element_desc, def.shader_type == single_texture ? 3u : 4u };
	pipeline_desc.PrimitiveTopologyType = def.line_primitives ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_desc.NumRenderTargets = 1;
	pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline_desc.DSVFormat = get_depth_format();
	pipeline_desc.SampleDesc.Count = 1;
	pipeline_desc.SampleDesc.Quality = 0;

	ID3D12PipelineState* pipeline;
	DX_CHECK(dx.device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline)));
	return pipeline;
}



void dx_create_sampler_descriptor(const DX_Sampler_Def& def, Dx_Sampler_Index sampler_index)
{
	uint32_t min, mag, mip;

	if (def.gl_mag_filter == GL_NEAREST) {
		mag = 0;
	} else if (def.gl_mag_filter == GL_LINEAR) {
		mag = 1;
	} else {
		ri.Error(ERR_FATAL, "create_sampler_descriptor: invalid gl_mag_filter");
	}

	bool max_lod_0_25 = false; // used to emulate OpenGL's GL_LINEAR/GL_NEAREST minification filter
	if (def.gl_min_filter == GL_NEAREST) {
		min = 0;
		mip = 0;
		max_lod_0_25 = true;
	} else if (def.gl_min_filter == GL_LINEAR) {
		min = 1;
		mip = 0;
		max_lod_0_25 = true;
	} else if (def.gl_min_filter == GL_NEAREST_MIPMAP_NEAREST) {
		min = 0;
		mip = 0;
	} else if (def.gl_min_filter == GL_LINEAR_MIPMAP_NEAREST) {
		min = 1;
		mip = 0;
	} else if (def.gl_min_filter == GL_NEAREST_MIPMAP_LINEAR) {
		min = 0;
		mip = 1;
	} else if (def.gl_min_filter == GL_LINEAR_MIPMAP_LINEAR) {
		min = 1;
		mip = 1;
	} else {
		ri.Error(ERR_FATAL, "vk_find_sampler: invalid gl_min_filter");
	}

	D3D12_TEXTURE_ADDRESS_MODE address_mode = def.repeat_texture ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D12_ENCODE_BASIC_FILTER(min, mag, mip, 0);
	sampler_desc.AddressU = address_mode;
	sampler_desc.AddressV = address_mode;
	sampler_desc.AddressW = address_mode;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	sampler_desc.BorderColor[0] = 0.0f;
	sampler_desc.BorderColor[1] = 0.0f;
	sampler_desc.BorderColor[2] = 0.0f;
	sampler_desc.BorderColor[3] = 0.0f;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = max_lod_0_25 ? 0.25f : 12.0f;

	D3D12_CPU_DESCRIPTOR_HANDLE sampler_handle = dx.sampler_heap->GetCPUDescriptorHandleForHeapStart();
	sampler_handle.ptr += dx.sampler_descriptor_size * sampler_index;

	dx.device->CreateSampler(&sampler_desc, sampler_handle);
}


ID3D12PipelineState* dx_find_pipeline(const DX_Pipeline_Def& def)
{
	for (int i = 0; i < dx_world.num_pipelines; ++i) {
		const auto& cur_def = dx_world.pipeline_defs[i];

		if (cur_def.shader_type == def.shader_type &&
			cur_def.state_bits == def.state_bits &&
			cur_def.face_culling == def.face_culling &&
			cur_def.polygon_offset == def.polygon_offset &&
			cur_def.clipping_plane == def.clipping_plane &&
			cur_def.mirror == def.mirror &&
			cur_def.line_primitives == def.line_primitives &&
			cur_def.shadow_phase == def.shadow_phase)
		{
			return dx_world.pipelines[i];
		}
	}

	if (dx_world.num_pipelines >= MAX_VK_PIPELINES) {
		ri.Error(ERR_DROP, "dx_find_pipeline: MAX_VK_PIPELINES hit\n");
	}


	ID3D12PipelineState* pipeline = create_pipeline(def);

	dx_world.pipeline_defs[dx_world.num_pipelines] = def;
	dx_world.pipelines[dx_world.num_pipelines] = pipeline;
	dx_world.num_pipelines++;
	return pipeline;
}

static void get_mvp_transform(float* mvp)
{
	if (backEnd.projection2D) {
		float mvp0 = 2.0f / glConfig.vidWidth;
		float mvp5 = 2.0f / glConfig.vidHeight;

		mvp[0]  =  mvp0; mvp[1]  =  0.0f; mvp[2]  = 0.0f; mvp[3]  = 0.0f;
		mvp[4]  =  0.0f; mvp[5]  = -mvp5; mvp[6]  = 0.0f; mvp[7]  = 0.0f;
		mvp[8]  =  0.0f; mvp[9]  = 0.0f; mvp[10] = 1.0f; mvp[11] = 0.0f;
		mvp[12] = -1.0f; mvp[13] = 1.0f; mvp[14] = 0.0f; mvp[15] = 1.0f;

	} 
	else
	{
		const float* p = backEnd.viewParms.projectionMatrix;

		// update q3's proj matrix (opengl) to d3d conventions: z - [0, 1] instead of [-1, 1]
		float zNear	= r_znear->value;
		float zFar = backEnd.viewParms.zFar;
		float P10 = -zFar / (zFar - zNear);
		float P14 = -zFar*zNear / (zFar - zNear);

		float proj[16] = {
			p[0],  p[1],  p[2], p[3],
			p[4],  p[5],  p[6], p[7],
			p[8],  p[9],  P10,  p[11],
			p[12], p[13], P14,  p[15]
		};

		myGlMultMatrix(dx_world.modelview_transform, proj, mvp);
	}
}

static D3D12_RECT get_viewport_rect()
{
	D3D12_RECT r;
	if (backEnd.projection2D)
	{
		r.left = 0.0f;
		r.top = 0.0f;
		r.right = glConfig.vidWidth;
		r.bottom = glConfig.vidHeight;
	}
	else
	{
		r.left = backEnd.viewParms.viewportX;
		r.top = glConfig.vidHeight - (backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight);
		r.right = r.left + backEnd.viewParms.viewportWidth;
		r.bottom = r.top + backEnd.viewParms.viewportHeight;
	}
	return r;
}

static D3D12_VIEWPORT get_viewport(DX_Depth_Range depth_range)
{
	D3D12_RECT r = get_viewport_rect();

	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = (float)r.left;
	viewport.TopLeftY = (float)r.top;
	viewport.Width = (float)(r.right - r.left);
	viewport.Height = (float)(r.bottom - r.top);

	if (depth_range == DX_Depth_Range::force_zero) {
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 0.0f;
	} else if (depth_range == DX_Depth_Range::force_one) {
		viewport.MinDepth = 1.0f;
		viewport.MaxDepth = 1.0f;
	} else if (depth_range == DX_Depth_Range::weapon) {
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 0.3f;
	} else {
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}
	return viewport;
}

static D3D12_RECT get_scissor_rect()
{
	D3D12_RECT r = get_viewport_rect();

	if (r.left < 0)
		r.left = 0;
	if (r.top < 0)
		r.top = 0;

	if (r.right > glConfig.vidWidth)
		r.right = glConfig.vidWidth;
	if (r.bottom > glConfig.vidHeight)
		r.bottom = glConfig.vidHeight;

	return r;
}

void dx_clear_attachments(bool clear_depth_stencil, bool clear_color, float color[4])
{
	if (!clear_depth_stencil && !clear_color)
		return;

	D3D12_RECT clear_rect = get_scissor_rect();

	if (clear_depth_stencil) {
		D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
		if (r_shadows->integer == 2)
			flags |= D3D12_CLEAR_FLAG_STENCIL;

		D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dx.dsv_heap->GetCPUDescriptorHandleForHeapStart();
		dx.command_list->ClearDepthStencilView(dsv_handle, flags, 1.0f, 0, 1, &clear_rect);
	}

	if (clear_color) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		rtv_handle.ptr += dx.frame_index * dx.rtv_descriptor_size;
		dx.command_list->ClearRenderTargetView(rtv_handle, color, 1, &clear_rect);
	}
}

void dx_bind_geometry()
{
	// xyz stream
	{
		if ((dx.xyz_elements + tess.numVertexes) * sizeof(vec4_t) > XYZ_SIZE)
			ri.Error(ERR_DROP, "dx_bind_geometry: vertex buffer overflow (xyz)\n");

		byte* dst = dx.vertex_buffer_ptr + XYZ_OFFSET + dx.xyz_elements * sizeof(vec4_t);
		memcpy(dst, tess.xyz, tess.numVertexes * sizeof(vec4_t));

		uint32_t xyz_offset = XYZ_OFFSET + dx.xyz_elements * sizeof(vec4_t);

		D3D12_VERTEX_BUFFER_VIEW xyz_view;
		xyz_view.BufferLocation = dx.geometry_buffer->GetGPUVirtualAddress() + xyz_offset;
		xyz_view.SizeInBytes = static_cast<UINT>(tess.numVertexes * sizeof(vec4_t));
		xyz_view.StrideInBytes = static_cast<UINT>(sizeof(vec4_t));
		dx.command_list->IASetVertexBuffers(0, 1, &xyz_view);

		dx.xyz_elements += tess.numVertexes;
	}

	// indexes stream
	{
		uint32_t indexes_size = tess.numIndexes * sizeof(uint32_t);        

		if (dx.index_buffer_offset + indexes_size > INDEX_BUFFER_SIZE)
			ri.Error(ERR_DROP, "dx_bind_geometry: index buffer overflow\n");

		byte* dst = dx.index_buffer_ptr + dx.index_buffer_offset;
		memcpy(dst, tess.indexes, indexes_size);

		D3D12_INDEX_BUFFER_VIEW index_view;
		index_view.BufferLocation = dx.geometry_buffer->GetGPUVirtualAddress() + VERTEX_BUFFER_SIZE + dx.index_buffer_offset;
		index_view.SizeInBytes = static_cast<UINT>(indexes_size);
		index_view.Format = DXGI_FORMAT_R32_UINT;
		dx.command_list->IASetIndexBuffer(&index_view);

		dx.index_buffer_offset += static_cast<int>(indexes_size);
	}

	//
	// Specify push constants.
	//
	float root_constants[16 + 12 + 4]; // mvp transform + eye transform + clipping plane in eye space

	get_mvp_transform(root_constants);
	int root_constant_count = 16;

	if (backEnd.viewParms.isPortal)
	{
		// Eye space transform.
		// NOTE: backEnd.or.modelMatrix incorporates s_flipMatrix, so it should be taken into account 
		// when computing clipping plane too.
		float* eye_xform = root_constants + 16;
		for (int i = 0; i < 12; ++i)
		{
			eye_xform[i] = backEnd.ori.modelMatrix[(i%4)*4 + i/4 ];
		}

		// Clipping plane in eye coordinates.
		float world_plane[4];
		world_plane[0] = backEnd.viewParms.portalPlane.normal[0];
		world_plane[1] = backEnd.viewParms.portalPlane.normal[1];
		world_plane[2] = backEnd.viewParms.portalPlane.normal[2];
		world_plane[3] = backEnd.viewParms.portalPlane.dist;

		float eye_plane[4];
		eye_plane[0] = DotProduct (backEnd.viewParms.ori.axis[0], world_plane);
		eye_plane[1] = DotProduct (backEnd.viewParms.ori.axis[1], world_plane);
		eye_plane[2] = DotProduct (backEnd.viewParms.ori.axis[2], world_plane);
		eye_plane[3] = DotProduct (world_plane, backEnd.viewParms.ori.origin) - world_plane[3];

		// Apply s_flipMatrix to be in the same coordinate system as eye_xfrom.
		root_constants[28] = -eye_plane[1];
		root_constants[29] =  eye_plane[2];
		root_constants[30] = -eye_plane[0];
		root_constants[31] =  eye_plane[3];

		root_constant_count += 16;
	}

	dx.command_list->SetGraphicsRoot32BitConstants(0, root_constant_count, root_constants, 0);
}


void dx_shade_geometry(ID3D12PipelineState* pipeline, bool multitexture, 
	DX_Depth_Range depth_range, bool indexed, bool lines)
{
	// color
	{
		if ((dx.color_st_elements + tess.numVertexes) * sizeof(color4ub_t) > COLOR_SIZE)
			ri.Error(ERR_DROP, "vulkan: vertex buffer overflow (color)\n");

		byte* dst = dx.vertex_buffer_ptr + COLOR_OFFSET + dx.color_st_elements * sizeof(color4ub_t);
		memcpy(dst, tess.svars.colors, tess.numVertexes * sizeof(color4ub_t));
	}
	// st0
	{
		if ((dx.color_st_elements + tess.numVertexes) * sizeof(vec2_t) > ST0_SIZE)
			ri.Error(ERR_DROP, "vulkan: vertex buffer overflow (st0)\n");

		byte* dst = dx.vertex_buffer_ptr + ST0_OFFSET + dx.color_st_elements * sizeof(vec2_t);
		memcpy(dst, tess.svars.texcoords[0], tess.numVertexes * sizeof(vec2_t));
	}
	// st1
	if (multitexture) {
		if ((dx.color_st_elements + tess.numVertexes) * sizeof(vec2_t) > ST1_SIZE)
			ri.Error(ERR_DROP, "vulkan: vertex buffer overflow (st1)\n");

		byte* dst = dx.vertex_buffer_ptr + ST1_OFFSET + dx.color_st_elements * sizeof(vec2_t);
		memcpy(dst, tess.svars.texcoords[1], tess.numVertexes * sizeof(vec2_t));
	}

	//
	// Configure vertex data stream.
	//
	D3D12_VERTEX_BUFFER_VIEW color_st_views[3];
	color_st_views[0].BufferLocation = dx.geometry_buffer->GetGPUVirtualAddress() + COLOR_OFFSET + dx.color_st_elements * sizeof(color4ub_t);
	color_st_views[0].SizeInBytes = static_cast<UINT>(tess.numVertexes * sizeof(color4ub_t));
	color_st_views[0].StrideInBytes = static_cast<UINT>(sizeof(color4ub_t));

	color_st_views[1].BufferLocation = dx.geometry_buffer->GetGPUVirtualAddress() + ST0_OFFSET + dx.color_st_elements * sizeof(vec2_t);
	color_st_views[1].SizeInBytes = static_cast<UINT>(tess.numVertexes * sizeof(vec2_t));
	color_st_views[1].StrideInBytes = static_cast<UINT>(sizeof(vec2_t));

	color_st_views[2].BufferLocation = dx.geometry_buffer->GetGPUVirtualAddress() + ST1_OFFSET + dx.color_st_elements * sizeof(vec2_t);
	color_st_views[2].SizeInBytes = static_cast<UINT>(tess.numVertexes * sizeof(vec2_t));
	color_st_views[2].StrideInBytes = static_cast<UINT>(sizeof(vec2_t));

	dx.command_list->IASetVertexBuffers(1, multitexture ? 3 : 2, color_st_views);
	dx.color_st_elements += tess.numVertexes;

	//
	// Set descriptor tables.
	//
	{
		D3D12_GPU_DESCRIPTOR_HANDLE srv_handle = dx.srv_heap->GetGPUDescriptorHandleForHeapStart();
		srv_handle.ptr += dx.srv_descriptor_size * dx_world.current_image_indices[0];
		dx.command_list->SetGraphicsRootDescriptorTable(1, srv_handle);

		D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle = dx.sampler_heap->GetGPUDescriptorHandleForHeapStart();
		const int sampler_index = dx_world.images[dx_world.current_image_indices[0]].sampler_index;
		sampler_handle.ptr += dx.sampler_descriptor_size * sampler_index;
		dx.command_list->SetGraphicsRootDescriptorTable(2, sampler_handle);
	}

	if (multitexture) {
		D3D12_GPU_DESCRIPTOR_HANDLE srv_handle = dx.srv_heap->GetGPUDescriptorHandleForHeapStart();
		srv_handle.ptr += dx.srv_descriptor_size * dx_world.current_image_indices[1];
		dx.command_list->SetGraphicsRootDescriptorTable(3, srv_handle);

		D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle = dx.sampler_heap->GetGPUDescriptorHandleForHeapStart();
		const int sampler_index = dx_world.images[dx_world.current_image_indices[1]].sampler_index;
		sampler_handle.ptr += dx.sampler_descriptor_size * sampler_index;
		dx.command_list->SetGraphicsRootDescriptorTable(4, sampler_handle);
	}

	//
	// Configure pipeline.
	//
	dx.command_list->SetPipelineState(pipeline);
	dx.command_list->IASetPrimitiveTopology(lines ? D3D10_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_RECT scissor_rect = get_scissor_rect();
	dx.command_list->RSSetScissorRects(1, &scissor_rect);

	D3D12_VIEWPORT viewport = get_viewport(depth_range);
	dx.command_list->RSSetViewports(1, &viewport);

	//
	// Draw.
	//
	if (indexed)
		dx.command_list->DrawIndexedInstanced(tess.numIndexes, 1, 0, 0, 0);
	else
		dx.command_list->DrawInstanced(tess.numVertexes, 1, 0, 0);
}