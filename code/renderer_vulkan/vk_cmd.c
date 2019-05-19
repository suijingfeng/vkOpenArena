#include "VKimpl.h"
#include "vk_instance.h"
#include "ref_import.h" 
//  ===============  Command Buffer Lifecycle ====================
// Each command buffer is always in one of the following states:
//
// Initial
//
// When a command buffer is allocated, it is in the initial state. Some commands are able to
// reset a command buffer, or a set of command buffers, back to this state from any of the 
// executable, recording or invalid state. Command buffers in the initial state can only be
// moved to the recording state, or freed.
//
// Recording
//
// vkBeginCommandBuffer changes the state of a command buffer from the initial state to the
// recording state. Once a command buffer is in the recording state, vkCmd* commands can be
// used to record to the command buffer.
//
// Executable
//
// vkEndCommandBuffer ends the recording of a command buffer, and moves it from the recording
// state to the executable state. Executable command buffers can be submitted, reset, or
// recorded to another command buffer.
//
// Pending
//
// Queue submission of a command buffer changes the state of a command buffer from the
// executable state to the pending state. Whilst in the pending state, applications must
// not attempt to modify the command buffer in any way - as the device may be processing
// the commands recorded to it. Once execution of a command buffer completes, the command
// buffer reverts back to either the executable state, or the invalid state if it was
// recorded with VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT. A synchronization command
// should be used to detect when this occurs.
//
// Invalid
// 
// Some operations, such as modifying or deleting a resource that was used in a command
// recorded to a command buffer, will transition the state of that command buffer into
// the invalid state. Command buffers in the invalid state can only be reset or freed.
//
// Any given command that operates on a command buffer has its own requirements on what
// state a command buffer must be in, which are detailed in the valid usage constraints
// for that command.
// 
// Resetting a command buffer is an operation that discards any previously recorded 
// commands and puts a command buffer in the initial state. Resetting occurs as a
// result of vkResetCommandBuffer or vkResetCommandPool, or as part of 
// vkBeginCommandBuffer (which additionally puts the command buffer in the recording state).
//
// Secondary command buffers can be recorded to a primary command buffer via
// vkCmdExecuteCommands. This partially ties the lifecycle of the two command
// buffers together - if the primary is submitted to a queue, both the primary 
// and any secondaries recorded to it move to the pending state. Once execution
// of the primary completes, so does any secondary recorded within it, and once all 
// executions of each command buffer complete, they move to the executable state.
// If a secondary moves to any other state whilst it is recorded to another command buffer, 
// the primary moves to the invalid state. A primary moving to any other state does 
// not affect the state of the secondary. Resetting or freeing a primary command buffer 
// removes the linkage to any secondary command buffers that were recorded to it.
//
//

void vk_create_command_pool(VkCommandPool* const pPool)
{
    // Command pools are opaque objects that command buffer memory is allocated from,
    // and which allow the implementation to amortize the cost of resource creation
    // across multiple command buffers. Command pools are externally synchronized,
    // meaning that a command pool must not be used concurrently in multiple threads.
    // That includes use via recording commands on any command buffers allocated from
    // the pool, as well as operations that allocate, free, and reset command buffers
    // or the pool itself.
    ri.Printf(PRINT_ALL, " Create command pool: vk.command_pool \n");

    VkCommandPoolCreateInfo desc;
    desc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    desc.pNext = NULL;
    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers
    // allocated from the pool will be short-lived, meaning that they will
    // be reset or freed in a relatively short timeframe. This flag may be
    // used by the implementation to control memory allocation behavior
    // within the pool.
    //
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command
    // buffer allocated from a pool to be individually reset to the initial
    // state; either by calling vkResetCommandBuffer, or via the implicit 
    // reset when calling vkBeginCommandBuffer. If this flag is not set on
    // a pool, then vkResetCommandBuffer must not be called for any command
    // buffer allocated from that pool.
    // 
    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | 
    desc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    desc.queueFamilyIndex = vk.queue_family_index;

    VK_CHECK(qvkCreateCommandPool(vk.device, &desc, NULL, pPool));
}


void vk_create_command_buffer(VkCommandPool pool, VkCommandBuffer* const pBuf)
{
    // Command buffers are objects used to record commands which can be
    // subsequently submitted to a device queue for execution. There are
    // two levels of command buffers:
    // - primary command buffers, which can execute secondary command buffers,
    //   and which are submitted to queues.
    // - secondary command buffers, which can be executed by primary command buffers,
    //   and which are not directly submitted to queues.
    //
    // Recorded commands include commands to bind pipelines and descriptor sets
    // to the command buffer, commands to modify dynamic state, commands to draw
    // (for graphics rendering), commands to dispatch (for compute), commands to
    // execute secondary command buffers (for primary command buffers only), 
    // commands to copy buffers and images, and other commands.
    //
    // Each command buffer manages state independently of other command buffers.
    // There is no inheritance of state across primary and secondary command 
    // buffers, or between secondary command buffers. 
    // 
    // When a command buffer begins recording, all state in that command buffer is undefined. 
    // When secondary command buffer(s) are recorded to execute on a primary command buffer,
    // the secondary command buffer inherits no state from the primary command buffer,
    // and all state of the primary command buffer is undefined after an execute secondary
    // command buffer command is recorded. There is one exception to this rule - if the primary
    // command buffer is inside a render pass instance, then the render pass and subpass state
    // is not disturbed by executing secondary command buffers. Whenever the state of a command
    // buffer is undefined, the application must set all relevant state on the command buffer
    // before any state dependent commands such as draws and dispatches are recorded, otherwise
    // the behavior of executing that command buffer is undefined.
    //
    // Unless otherwise specified, and without explicit synchronization, the various commands
    // submitted to a queue via command buffers may execute in arbitrary order relative to
    // each other, and/or concurrently. Also, the memory side-effects of those commands may
    // not be directly visible to other commands without explicit memory dependencies. 
    // This is true within a command buffer, and across command buffers submitted to a given
    // queue. See the synchronization chapter for information on implicit and explicit
    // synchronization between commands.
    VkCommandBufferAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.commandPool = pool;
    // Can be submitted to a queue for execution,
    // but cannnot be called from other command buffers.
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK( qvkAllocateCommandBuffers(vk.device, &alloc_info, pBuf) );
}


void vk_beginRecordCmds(VkCommandBuffer HCmdBuf)
{
    //qvkResetCommandBuffer(HCmdBuf, );
    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;
    VK_CHECK( qvkBeginCommandBuffer(HCmdBuf, &begin_info) );
}


void vk_commitRecordedCmds(VkCommandBuffer HCmdBuf)
{
    VK_CHECK( qvkEndCommandBuffer( HCmdBuf ) );

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &HCmdBuf;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    VK_CHECK( qvkQueueSubmit(vk.queue, 1, &submit_info, VK_NULL_HANDLE) );
    VK_CHECK( qvkQueueWaitIdle(vk.queue) );
}


void vk_freeCmdBufs(VkCommandBuffer* const pCmdBuf)
{
    // Command buffers will be automatically freed when their
    // command pool is destroyed, so it don't need an explicit 
    // cleanup.
    NO_CHECK( qvkFreeCommandBuffers(vk.device, vk.command_pool, 1, pCmdBuf) ); 
}

void vk_destroy_command_pool(void)
{
    ri.Printf( PRINT_ALL, " Destroy command pool: vk.command_pool. \n" );
    NO_CHECK( qvkDestroyCommandPool(vk.device, vk.command_pool, NULL) );
}
