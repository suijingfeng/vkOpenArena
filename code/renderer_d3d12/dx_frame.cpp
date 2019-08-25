#include <D3d12.h>
#include <DXGI1_4.h>
#include "tr_local.h"
#include "dx.h" 

// DX12
extern Dx_Instance	dx;				// shouldn't be cleared during ref re-init



void dx_begin_frame(void)
{
	// Gets the current value of the fence.
	// Returns the current value of the fence. 
	// If the device has been removed, the return
	// value will be UINT64_MAX.
	if (dx.fence->GetCompletedValue() < dx.fence_value)
	{
		// Specifies an event that should be fired
		// when the fence reaches a certain value.
		// dx.fence_value: The fence value when the event is to be signaled.
		// dx.fence_event: A handle to the event object.
		DX_CHECK(dx.fence->SetEventOnCompletion(dx.fence_value, dx.fence_event));
		WaitForSingleObject(dx.fence_event, INFINITE);
	}

	dx.frame_index = dx.swapchain->GetCurrentBackBufferIndex();

	DX_CHECK(dx.command_allocator->Reset());
	DX_CHECK(dx.command_list->Reset(dx.command_allocator, nullptr));

	dx.command_list->SetGraphicsRootSignature(dx.root_signature);

	ID3D12DescriptorHeap* heaps[2] = { dx.srv_heap, dx.sampler_heap };
	//dx.command_list->SetDescriptorHeaps(_countof(heaps), heaps);

	dx.command_list->SetDescriptorHeaps(2, heaps);




	D3D12_RESOURCE_BARRIER barrier1;
	barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier1.Transition.pResource = dx.render_targets[dx.frame_index];
	barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;


	dx.command_list->ResourceBarrier(1, &barrier1);

	D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dx.dsv_heap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = dx.rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += dx.frame_index * dx.rtv_descriptor_size;

	dx.command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

	dx.xyz_elements = 0;
	dx.color_st_elements = 0;
	dx.index_buffer_offset = 0;
}

void dx_end_frame()
{

	D3D12_RESOURCE_BARRIER barrier2;
	barrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier2.Transition.pResource = dx.render_targets[dx.frame_index];
	barrier2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	dx.command_list->ResourceBarrier(1, &barrier2);

	DX_CHECK(dx.command_list->Close());


	ID3D12CommandList* pCommandLists = dx.command_list;
	dx.command_queue->ExecuteCommandLists(1, &pCommandLists);


	++dx.fence_value;
	DX_CHECK(dx.command_queue->Signal(dx.fence, dx.fence_value));

	DX_CHECK(dx.swapchain->Present(0, 0));
}
