// utils
// Cheaper adapter enumeration
// For Direct3D 12, it’s no longer possible to backtrack from a device to the IDXGIAdapter 
// that was used to create it. It’s also no longer possible to provide D3D_DRIVER_TYPE_WARP
// into D3D12CreateDevice.To make development easier, you can use IDXGIFactory4 to deal with
// both of these.IDXGIFactory4::EnumAdapterByLuid(designed to be paired with 
// ID3D12Device::GetAdapterLuid) enables an app to retrieve information about the adapter 
// where a Direct3D 12 device was created.IDXGIFactory4::EnumWarpAdapter provides an adapter
// which can be provided to D3D12CreateDevice to use the WARP renderer.


// ===================================
//  Video memory budget tracking
// Application developers are encouraged to use a video memory reservation system, 
// to inform the OS of the amount of physical video memory the app cannot go without.
// The amount of physical memory available for a process is known as the "video memory budget".
// The budget can fluctuate noticeably as background processes wakeup and sleep; 
// and fluctuate dramatically when the user switches away to another application.
// The application can be notified when the budget changes and poll both the current budget
// and the currently consumed amount of memory.If an application doesn’t stay within its budget,
// the process will be intermittently frozen to allow other applications to run 
// and/or the creation APIs will return failure. 
// The IDXGIAdapter3 interface provides the methods pertaining to this functionality, 
// in particular QueryVideoMemoryInfo and RegisterVideoMemoryBudgetChangeNotificationEvent.
// For more information, refer to the Direct3D 12 topic on Residency.


// ==========================================
//  Direct3D 12 Swapchain Changes
//
// Some of the existing Direct3D 11 swapchain functionality was deprecated 
// to achieve the overhead reductions in Direct3D 12. While other changes were made 
// to either align to Direct3D 12 concepts or provide better support for Direct3D 12 features.
//
//
// Invariant Backbuffer Identity
// In Direct3D 11, applications could call GetBuffer(0, ? only once. 
// Every call to Present implicitly changed the resource identity of the returned interface.
// Direct3D 12 no longer supports that implicit resource identity change, 
// due to the CPU overhead required and the flexible resource descriptor design.
// As a result, the application must manually call GetBuffer for every each buffer 
// created with the swapchain.The application must manually render to the next buffer
// in the sequence after calling Present.
// Applications are encouraged to create a cache of descriptors for each buffer, 
// instead of recreating many objects each Present.

// ===============================
// Multi - Adapter Support
//
// When a swapchain is created on a multi-GPU adapter, the backbuffers are 
// all created on node 1 and only a single command queue is supported.
// ResizeBuffers1 enables applications to create backbuffers on different nodes,
// allowing a different command queue to be used with each. These capabilities 
// enable Alternate Frame Rendering(AFR) techniques to be used with the swapchain.
// Refer to Multi - adapter.

// ===============================
// Miscellaneous
//
// A command queue object must be passed to CreateSwapChain methods instead of 
// the Direct3D 12 device object.
// Only the following two flip model swap effects are supported :
//
// * DXGI_SWAP_EFFECT_FLIP_DISCARD should be preferred when applications fully 
// render over the backbuffer before presenting it or are interested in 
// supporting multi - adapter scenarios easily.

// * DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL should be used by applications that rely
// on partial presentation optimizations or regularly read from previously 
// presented backbuffers.
// 
// SetFullscreenState no longer exclusively owns the display, 
// so user initiated operating system elements can seamlessly appear 
// in front of application output.Volume settings is an example of this.

#include "tr_local.h"
#include "dx_utils.h"

void printWideStr(wchar_t * const WStr)
{
	size_t len = wcslen(WStr) + 1;
	size_t converted = 0;

	char *CStr = (char*)malloc(len * sizeof(char));
	// Converts a sequence of wide characters to a 
	// corresponding sequence of multibyte characters.
	wcstombs_s(&converted, CStr, len, WStr, _TRUNCATE);
	ri.Printf(PRINT_ALL, "%s\n", CStr);

	free(CStr);
}


void printOutputInfo(IDXGIAdapter1* const pAdapter)
{
	UINT i = 0;
	// An IDXGIOutput interface represents an adapter output (such as a monitor).
	IDXGIOutput * pOutput = nullptr;

	if(pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC monitorDesc;
		pOutput->GetDesc(&monitorDesc);

		printWideStr(monitorDesc.DeviceName);

		++i;
	}
	else
	{
		ri.Printf(PRINT_ALL, "EnumOutputs %d failed. \n", i);
	}

	pOutput->Release();
}


void printAvailableAdapters(IDXGIFactory2* const pHardwareFactory)
{
	// HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	// if ( FAILED(hr) )
	// 	ri.Error( ERR_FATAL, "Direct3D: error returned by %s", CreateDXGIFactory2);

	// The IDXGIAdapter1 interface represents a display sub-system 
	// (including one or more GPU's, DACs and video memory).
	IDXGIAdapter1* pHardwareAdapter = nullptr;

	// adapter_index: The index of the adapter to enumerate.
	UINT adapter_index = 0;

	// Enumerates both adapters (video cards) with or without outputs.

	// The address of a pointer to an IDXGIAdapter1 interface at the 
	// position specified by the adapter_index parameter.

	// To enumerate the display subsystems, use IDXGIFactory1::EnumAdapters1.
	// A display subsystem is often referred to as a video card, however,
	// on some machines the display subsystem is part of the mother board.
	// IDXGIFactory1::EnumAdapters1 enumerates local adapter(s) without 
	// any monitors or outputs attached, as well as adapters(s) with 
	// outputs attached. The first adapter returned will be the local adapter
	// on which the Desktop primary is displayed.
	while (pHardwareFactory->EnumAdapters1(adapter_index, &pHardwareAdapter) != DXGI_ERROR_NOT_FOUND)
	{

		// Gets a DXGI 1.1 description of an adapter(or video card).
		DXGI_ADAPTER_DESC1 desc;
		pHardwareAdapter->GetDesc1(&desc);
		
		ri.Printf(PRINT_ALL, "\n--------------------------------------------\n");

		ri.Printf(PRINT_ALL, "Adapter [%d]: ", adapter_index);
		printWideStr(desc.Description);

		switch (desc.VendorId)
		{
		case 0x8086:
			ri.Printf(PRINT_ALL, "Intel, VendorId: %x, \n", desc.VendorId);
			break;
		case 0x1002:
			ri.Printf(PRINT_ALL, "AMD, VendorId: %x, \n", desc.VendorId);
			break;
		case 0x10DE:
			ri.Printf(PRINT_ALL, "NVIDIA, VendorId: %x, \n", desc.VendorId);
			break;
		default :
			ri.Printf(PRINT_ALL, "UNKNOWN Company, VendorId: %x, \n", desc.VendorId);
			break;
		}


		ri.Printf(PRINT_ALL, "DeviceId: %x, SubSysId: %x \n", desc.DeviceId, desc.SubSysId);
		
		ri.Printf(PRINT_ALL, "Revision: %d, AdapterLuid: %x \n", desc.Revision, desc.AdapterLuid);
		
		// The number of bytes of dedicated video memory that are not shared with the CPU.
		//
		// The number of bytes of dedicated system memory that are not shared with the CPU.
		// This memory is allocated from available system memory at boot time.
		ri.Printf(PRINT_ALL, "VideoMemory: %d MB, DedicatedSystemMemory: %d MB, SharedSystemMemory: %d MB\n",
			desc.DedicatedVideoMemory >> 20, desc.DedicatedSystemMemory >> 20, desc.SharedSystemMemory >> 20);
		// The number of bytes of shared system memory. This is the maximum value
		// of system memory that may be consumed by the adapter during operation.
		// Any incidental memory consumed by the driver as it manages and uses 
		// video memory is additional.

		// get next adapter 
		++adapter_index;
	}

	// do not release it twice
	// pHardwareAdapter->Release();
	ri.Printf(PRINT_ALL, "--------------------------------------------\n\n");

	ri.Printf(PRINT_ALL, " Current adapter index: [%d] \n", r_gpuIndex->integer);
}


unsigned int getNumberOfAvailableAdapters(IDXGIFactory2* const pHardwareFactory)
{
	IDXGIAdapter1* pHardwareAdapter = nullptr;
	// adapter_index: The index of the adapter to enumerate.
	UINT adapter_index = 0;

	while (pHardwareFactory->EnumAdapters1(adapter_index, &pHardwareAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		// Gets a DXGI 1.1 description of an adapter(or video card).
		DXGI_ADAPTER_DESC1 desc;
		pHardwareAdapter->GetDesc1(&desc);

		++adapter_index;
	}
	return adapter_index;
}


void printAvailableAdapters_f(void)
{
	IDXGIFactory2* pFactory;
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	printAvailableAdapters(pFactory);
	pFactory->Release();
}