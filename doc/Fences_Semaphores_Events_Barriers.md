## Synchronization for Vulkan: Fences, Semaphores, Events, Barriers


The synchronization is well known for CPUs that one has multiple threads 
for writing aimed to updating a buffer, then he needs to synchronize to make
sure all writes have finished, then he can continue to process the data with
multiple threads. However, for GPUs, the synchronization isn¡¯t exposed to
developers which was previously hidden inside the driver. But now, 
synchronization of access to resources in Vulkan is primarily the responsibility
of the application, which named ¡°fences¡±, ¡°Semaphores¡±, ¡°Events¡± and ¡°barriers¡±.


In Vulkan, there are three forms of concurrency during execution: between the 
host and device, between the queues, and between commands within a command buffer. 
Vulkan provides the application with a set of synchronization primitives for 
these purposes. Further, memory caches and other optimizations mean that the 
normal flow of command execution does not guarantee that all memory transactions
from a command are immediately visible to other agents with views into a given
block of memory. Vulkan also provides barrier operations to ensure this type of
synchronization.


Four synchronization primitive types are exposed by Vulkan. These are:

* Fences, being used to communicate completion of execution of command buffer
    submissions to queues back to the application.

* Semaphores, being generally associated with resources or groups of resources
    and can be used to marshal ownership of shared data. Their status is not 
    visible to the host.

* Events, providing a finer-grained synchronization primitive which can be
    signaled at command level granularity by both device and host, and can be
    waited upon by either.

* Barriers, providing execution and memory synchronization between sets of commands.


### Fences

Fences can be used by the host to determine completion of execution of submissions
to queues performed with vkQueueSubmit and vkQueueBindSparse. A fence¡¯s status is
always either signaled or unsignaled. The host can poll the status of a single fence,
or wait for any or all of a group of fences to become signaled. To create a new
fence object, use the command vkCreateFence. Following codes illustrate how to
create fence.
	
```
VkFenceCreateInfo createInfo = {};

createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

createInfo.pNext = nullptr;

createInfo.flags = (flags & FENCE_CREATE_SIGNALED_BIT) ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
// If flags contains VK_FENCE_CREATE_SIGNALED_BIT then the fence object is created
// in the signaled state. Otherwise it is created in the unsignaled state.

VkFence fence;

vkCreateFence(device, &createInfo, nullptr, &fence);
```

Fence must be unsignaled before submitting queue. Following codes illustrate the usage:


```
...

// Fence must be unsignaled before submitting queue

vkResetFences(device, 1, &fence);

...

// Prepare to submit queue.

if (pFence)

{

    // Fence should be unsignaled

    vkGetFenceStatus(device, fence) != VK_SUCCESS;

}

VkSubmitInfo submitInfo = {};

submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

submitInfo.pNext = nullptr;

submitInfo.waitSemaphoreCount = 0;

submitInfo.pWaitSemaphores = nullptr;

submitInfo.commandBufferCount = numCmdBuffers;

submitInfo.pCommandBuffers = pCmdBuffers;

submitInfo.signalSemaphoreCount = 0;

submitInfo.pSignalSemaphores = nullptr;

vkQueueSubmit(m_cmdQueue, 1, &submitInfo, fence);

```

Besides, user could call method vkWaitForFences to wait for completion. 
A fence is a very heavyweight synchronization primitive as it requires 
the GPU to flush all caches at least, and potentially some additional 
synchronization. Due to those costs, fences should be used sparingly. 
In particular, try to group per-frame resources and track them together
with a single fence instead of fine-grained per-resource tracking. 
For instance, all commands buffers used in one frame should be protected
by one fence, instead of one fence per command buffer. Fences should be 
also used sparingly to synchronize the compute, copy and graphics queues
per frame. Ideally, try to submit large batches of asynchronous compute
jobs with a single fence at the end which denotes that all jobs have finished. 
Same goes for copies, you should have the end of all copies signaled with
a single fence to get the best possible performance.

### Semaphores

Semaphores are used to coordinate operations between queues and between
queue submissions within a single queue. An application might associate
semaphores with resources or groups of resources to marshal ownership of
shared data. A semaphore¡¯s status is always either signaled or unsignaled.
Semaphores are signaled by queues and can also be waited on in the same or
different queues until they are signaled. To create a new semaphore object,
use the command vkCreateSemaphore. Following codes illustrate how to create
semaphore and usage of it.

```
VkSemaphoreCreateInfo semaphoreCreateInfo = {};

semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

semaphoreCreateInfo.pNext = nullptr;

semaphoreCreateInfo.flags = 0;

VkSemaphore semaphore;

vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);

// Acquire a presentable image aimed to present
vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, semaphore,
    VK_NULL_HANDLE, &backBufferIndex);

// WaitSemaphore is needed here to reset semaphore to unsignaled state 
// so that semaphore can be used in vkAcquireNextImageKHR again in Present function.

vkQueueWaitSemaphore(presentQueue, semaphore);

// Need call wait idle to make sure WaitSemaphore is done

vkQueueWaitIdle(presentQueue);
```

When a queue signals or waits upon a semaphore, certain implicit ordering
guarantees are provided. Semaphore operations may not make the side effects
of commands visible to the host.


### Events

Events represent a fine-grained synchronization primitive that can be used to
gauge progress through a sequence of commands executed on a queue by Vulkan.
An event is initially in the unsignaled state. It can be signaled by a device,
using commands inserted into the command buffer, or by the host. It can also be
reset to the unsignaled state by a device or the host. The host can query the
state of an event. A device can wait for one or more events to become signaled.
To create an event, use the command vkCreateEvent. Following codes illustrate
how to create semaphore and usage of it.


```
VkEventCreateInfo createInfo = {};

createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

createInfo.pNext = nullptr;

createInfo.flags = 0;

VkEvent event;

vkCreateFence(device, &createInfo, nullptr, &event);

```

Applications should be careful to avoid race conditions when using events.
For example, an event should only be reset if no vkCmdWaitEvents command is
executing that waits upon that event. An act of setting or resetting an event
in one queue may not affect or be visible to other queues. For cross-queue
synchronization, semaphores can be used.



### Barriers

A barrier is a new concept exposed to developers which was previously hidden
inside the driver. A pipeline barrier inserts an execution dependency and a 
set of memory dependencies between a set of commands earlier in the command
buffer and a set of commands later in the command buffer. A pipeline barrier
is recorded by calling vkCmdPipelineBarrier:


```
// Create the barrier which CPU could map query buffer and read the results

VkMemoryBarrier memoryBarrier = {};

memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

memoryBarrier.pNext = nullptr;

memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

memoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

VkDependencyFlags dependencyFlags = 0;

// need a memory barrier before result can be used

vkCmdPipelineBarrier(m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
VK_PIPELINE_STAGE_HOST_BIT, dependencyFlags, 1, &memoryBarrier, 
0, nullptr, 0, nullptr);

```

The method vkCmdPipelineBarrier could be called inside render pass or outside
render pass. If it is called inside render pass, then the first set of commands
is all prior commands in the same subpass and the second set of commands is all
subsequent commands in the same subpass. Otherwise, the first set of commands
is all prior commands submitted to the queue and recorded in the command buffer
and the second set of commands is all subsequent commands recorded in the command
buffer and submitted to the queue.

Barrier has three functions in total:


* Synchronization, ensuring that previous dependent work has completed before new work starts.
* Reformatting, ensuring that data to be read by a unit is in a format that unit understands.
* Visibility, ensuring that data that might be cached on a particular unit is visible to other units that might want to see it.

Besides, Vulkan provides three types of memory barriers: global memory, buffer memory, and image memory.

    Global memory, specifying with an instance of the VkMemoryBarrier structure.
    This type of barrier applies to memory accesses involving all memory objects
    that exist at the time of its execution.
    
    Buffer memory, specifying with an instance of the VkBufferMemoryBarrier
    structure. This type of barrier only applies to memory accesses involving
    a specific range of the specified buffer object. That is, a memory dependency
    formed from a buffer memory barrier is scoped to the specified range of the
    buffer. It is also used to transfer ownership of a buffer range from one 
    queue family to another.
    
    Image memory, specifying with an instance of the VkImageMemoryBarrier structure.
    This type of barrier only applies to memory accesses involving a specific
    subresource range of the specified image object. That is, a memory dependency
    formed from an image memory barrier is scoped to the specified subresources of
    the image. It is also used to perform a layout transition for an image
    subresource range, or to transfer ownership of an image subresource range
    from one queue family to another.

Following illustrate some cases to show how to use barrier:

```
// common codes for buffer memory

VkBufferMemoryBarrier bufferMemoryBarrier;

bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

bufferMemoryBarrier.pNext = nullptr;

bufferMemoryBarrier.srcAccessMask = srcAccessMask;

bufferMemoryBarrier.dstAccessMask = dstAccessMask;

bufferMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;

bufferMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

bufferMemoryBarrier.buffer = pBuffer;

bufferMemoryBarrier.size = VK_WHOLE_SIZE;

srcStageMask = srcStageMask;

dstStageMask = dstStageMask;

// For vkCmdCopyBuffer of constant buffer

bufferMemoryBarrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

vkCmdPipelineBarrier(¡­)

vkCmdCopyBuffer(¡­) // Aimed to copy constant buffer.

// Restore the status.

bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

vkCmdPipelineBarrier(¡­)


// common codes for image memory

VkImageMemoryBarrier imageMemoryBarrier;

imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

imageMemoryBarrier.pNext = nullptr;

imageMemoryBarrier.srcAccessMask = srcAccessMask;

imageMemoryBarrier.dstAccessMask = dstAccessMask;

imageMemoryBarrier.oldLayout = oldState;

imageMemoryBarrier.newLayout = newState;

imageMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;

imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

imageMemoryBarrier.image = pImage;

srcStageMask = srcStageMask;

dstStageMask = dstStageMask;

if (bCoverAllSubResources)

{

    // for null pSubresourceRange, which means all subresources (with depth/stencil aspects if it is D/S format)

    imageMemoryBarrier.subresourceRange.aspectMask = GetImageAspectMaskFromFormat(pImage->GetFormat());

    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;

    imageMemoryBarrier.subresourceRange.levelCount = pImage->GetMipLevels();

    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;

    imageMemoryBarrier.subresourceRange.layerCount = pImage->GetArraySize();

}

else

{

    ...

    imageMemoryBarrier.subresourceRange.aspectMask = subresourceRange.aspectMask;

    imageMemoryBarrier.subresourceRange.baseMipLevel = subresourceRange.baseMipLevel;

    imageMemoryBarrier.subresourceRange.levelCount = subresourceRange.levelCount;

    imageMemoryBarrier.subresourceRange.baseArrayLayer = (pImage->GetImageDimension() == IMAGE_DIMENSION_3D) ? 0 : subresourceRange.baseArrayLayer;

    imageMemoryBarrier.subresourceRange.layerCount = (pImage->GetImageDimension() == IMAGE_DIMENSION_3D) ? 1 : subresourceRange.layerCount;                    

}

// For vkCmdDraw, need to call vkCmdPipelineBarrier before and after.

imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

vkCmdPipelineBarrier(¡­)

vkCmdDraw(¡­) // Aimed to present.

// Restore the status.

imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

vkCmdPipelineBarrier(¡­)


// For vkCmdResolveImage

imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

vkCmdPipelineBarrier(¡­)

vkCmdResolveImage(¡­) // Aimed to resolve image.

// Restore the status.

imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

vkCmdPipelineBarrier(...)
```

Besides, there¡¯re many other situations to need barrier for synchronization.

### Conclusion

The synchronization isn¡¯t exposed to developers which was previously hidden inside the driver. But now, synchronization of access to resources in Vulkan is primarily the responsibility of the application which is intended to be designed to allow advanced applications to drive modern GPUs to their fullest capacity.
