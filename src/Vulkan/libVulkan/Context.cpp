#include <assert.h>
#include "Context.h"

namespace vulkan
{
	const VkExtensionProperties global_ext[] = {
		{ VK_KHR_SURFACE_EXTENSION_NAME, 25 }
	};

	const VkExtensionProperties device_extensions[] = {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, 68 },
		{ VK_KHR_win32_surface, 5 }
	};

#define MAKE_VULKAN_ENTRY(aFunction) { #aFunction, reinterpret_cast<PFN_vkVoidFunction>(aFunction) }
	const std::unordered_map < std::string, PFN_vkVoidFunction > func_ptrs = {
		MAKE_VULKAN_ENTRY(vkCreateInstance),
		MAKE_VULKAN_ENTRY(vkDestroyInstance),
		MAKE_VULKAN_ENTRY(vkEnumeratePhysicalDevices),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceFeatures),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceFormatProperties),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceImageFormatProperties),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceProperties),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceQueueFamilyProperties),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceMemoryProperties),
		MAKE_VULKAN_ENTRY(vkGetInstanceProcAddr),
		MAKE_VULKAN_ENTRY(vkGetDeviceProcAddr),
		MAKE_VULKAN_ENTRY(vkCreateDevice),
		MAKE_VULKAN_ENTRY(vkDestroyDevice),
		MAKE_VULKAN_ENTRY(vkEnumerateInstanceExtensionProperties),
		MAKE_VULKAN_ENTRY(vkEnumerateDeviceExtensionProperties),
		MAKE_VULKAN_ENTRY(vkEnumerateInstanceLayerProperties),
		MAKE_VULKAN_ENTRY(vkEnumerateDeviceLayerProperties),
		MAKE_VULKAN_ENTRY(vkGetDeviceQueue),
		MAKE_VULKAN_ENTRY(vkQueueSubmit),
		MAKE_VULKAN_ENTRY(vkQueueWaitIdle),
		MAKE_VULKAN_ENTRY(vkDeviceWaitIdle),
		MAKE_VULKAN_ENTRY(vkAllocateMemory),
		MAKE_VULKAN_ENTRY(vkFreeMemory),
		MAKE_VULKAN_ENTRY(vkMapMemory),
		MAKE_VULKAN_ENTRY(vkUnmapMemory),
		MAKE_VULKAN_ENTRY(vkFlushMappedMemoryRanges),
		MAKE_VULKAN_ENTRY(vkInvalidateMappedMemoryRanges),
		MAKE_VULKAN_ENTRY(vkGetDeviceMemoryCommitment),
		MAKE_VULKAN_ENTRY(vkBindBufferMemory),
		MAKE_VULKAN_ENTRY(vkBindImageMemory),
		MAKE_VULKAN_ENTRY(vkGetBufferMemoryRequirements),
		MAKE_VULKAN_ENTRY(vkGetImageMemoryRequirements),
		MAKE_VULKAN_ENTRY(vkGetImageSparseMemoryRequirements),
		MAKE_VULKAN_ENTRY(vkGetPhysicalDeviceSparseImageFormatProperties),
		MAKE_VULKAN_ENTRY(vkQueueBindSparse),
		MAKE_VULKAN_ENTRY(vkCreateFence),
		MAKE_VULKAN_ENTRY(vkDestroyFence),
		MAKE_VULKAN_ENTRY(vkResetFences),
		MAKE_VULKAN_ENTRY(vkGetFenceStatus),
		MAKE_VULKAN_ENTRY(vkWaitForFences),
		MAKE_VULKAN_ENTRY(vkCreateSemaphore),
		MAKE_VULKAN_ENTRY(vkDestroySemaphore),
		MAKE_VULKAN_ENTRY(vkCreateEvent),
		MAKE_VULKAN_ENTRY(vkDestroyEvent),
		MAKE_VULKAN_ENTRY(vkGetEventStatus),
		MAKE_VULKAN_ENTRY(vkSetEvent),
		MAKE_VULKAN_ENTRY(vkResetEvent),
		MAKE_VULKAN_ENTRY(vkCreateQueryPool),
		MAKE_VULKAN_ENTRY(vkDestroyQueryPool),
		MAKE_VULKAN_ENTRY(vkGetQueryPoolResults),
		MAKE_VULKAN_ENTRY(vkCreateBuffer),
		MAKE_VULKAN_ENTRY(vkDestroyBuffer),
		MAKE_VULKAN_ENTRY(vkCreateBufferView),
		MAKE_VULKAN_ENTRY(vkDestroyBufferView),
		MAKE_VULKAN_ENTRY(vkCreateImage),
		MAKE_VULKAN_ENTRY(vkDestroyImage),
		MAKE_VULKAN_ENTRY(vkGetImageSubresourceLayout),
		MAKE_VULKAN_ENTRY(vkCreateImageView),
		MAKE_VULKAN_ENTRY(vkDestroyImageView),
		MAKE_VULKAN_ENTRY(vkCreateShaderModule),
		MAKE_VULKAN_ENTRY(vkDestroyShaderModule),
		MAKE_VULKAN_ENTRY(vkCreatePipelineCache),
		MAKE_VULKAN_ENTRY(vkDestroyPipelineCache),
		MAKE_VULKAN_ENTRY(vkGetPipelineCacheData),
		MAKE_VULKAN_ENTRY(vkMergePipelineCaches),
		MAKE_VULKAN_ENTRY(vkCreateGraphicsPipelines),
		MAKE_VULKAN_ENTRY(vkCreateComputePipelines),
		MAKE_VULKAN_ENTRY(vkDestroyPipeline),
		MAKE_VULKAN_ENTRY(vkCreatePipelineLayout),
		MAKE_VULKAN_ENTRY(vkDestroyPipelineLayout),
		MAKE_VULKAN_ENTRY(vkCreateSampler),
		MAKE_VULKAN_ENTRY(vkDestroySampler),
		MAKE_VULKAN_ENTRY(vkCreateDescriptorSetLayout),
		MAKE_VULKAN_ENTRY(vkDestroyDescriptorSetLayout),
		MAKE_VULKAN_ENTRY(vkCreateDescriptorPool),
		MAKE_VULKAN_ENTRY(vkDestroyDescriptorPool),
		MAKE_VULKAN_ENTRY(vkResetDescriptorPool),
		MAKE_VULKAN_ENTRY(vkAllocateDescriptorSets),
		MAKE_VULKAN_ENTRY(vkFreeDescriptorSets),
		MAKE_VULKAN_ENTRY(vkUpdateDescriptorSets),
		MAKE_VULKAN_ENTRY(vkCreateFramebuffer),
		MAKE_VULKAN_ENTRY(vkDestroyFramebuffer),
		MAKE_VULKAN_ENTRY(vkCreateRenderPass),
		MAKE_VULKAN_ENTRY(vkDestroyRenderPass),
		MAKE_VULKAN_ENTRY(vkGetRenderAreaGranularity),
		MAKE_VULKAN_ENTRY(vkCreateCommandPool),
		MAKE_VULKAN_ENTRY(vkDestroyCommandPool),
		MAKE_VULKAN_ENTRY(vkResetCommandPool),
		MAKE_VULKAN_ENTRY(vkAllocateCommandBuffers),
		MAKE_VULKAN_ENTRY(vkFreeCommandBuffers),
		MAKE_VULKAN_ENTRY(vkBeginCommandBuffer),
		MAKE_VULKAN_ENTRY(vkEndCommandBuffer),
		MAKE_VULKAN_ENTRY(vkResetCommandBuffer),
		MAKE_VULKAN_ENTRY(vkCmdBindPipeline),
		MAKE_VULKAN_ENTRY(vkCmdSetViewport),
		MAKE_VULKAN_ENTRY(vkCmdSetScissor),
		MAKE_VULKAN_ENTRY(vkCmdSetLineWidth),
		MAKE_VULKAN_ENTRY(vkCmdSetDepthBias),
		MAKE_VULKAN_ENTRY(vkCmdSetBlendConstants),
		MAKE_VULKAN_ENTRY(vkCmdSetDepthBounds),
		MAKE_VULKAN_ENTRY(vkCmdSetStencilCompareMask),
		MAKE_VULKAN_ENTRY(vkCmdSetStencilWriteMask),
		MAKE_VULKAN_ENTRY(vkCmdSetStencilReference),
		MAKE_VULKAN_ENTRY(vkCmdBindDescriptorSets),
		MAKE_VULKAN_ENTRY(vkCmdBindDescriptorSets),
		MAKE_VULKAN_ENTRY(vkCmdBindIndexBuffer),
		MAKE_VULKAN_ENTRY(vkCmdBindVertexBuffers),
		MAKE_VULKAN_ENTRY(vkCmdDraw),
		MAKE_VULKAN_ENTRY(vkCmdDrawIndexed),
		MAKE_VULKAN_ENTRY(vkCmdDrawIndirect),
		MAKE_VULKAN_ENTRY(vkCmdDrawIndexedIndirect),
		MAKE_VULKAN_ENTRY(vkCmdDispatch),
		MAKE_VULKAN_ENTRY(vkCmdDispatchIndirect),
		MAKE_VULKAN_ENTRY(vkCmdCopyBuffer),
		MAKE_VULKAN_ENTRY(vkCmdCopyImage),
		MAKE_VULKAN_ENTRY(vkCmdBlitImage),
		MAKE_VULKAN_ENTRY(vkCmdCopyBufferToImage),
		MAKE_VULKAN_ENTRY(vkCmdCopyImageToBuffer),
		MAKE_VULKAN_ENTRY(vkCmdUpdateBuffer),
		MAKE_VULKAN_ENTRY(vkCmdFillBuffer),
		MAKE_VULKAN_ENTRY(vkCmdClearColorImage),
		MAKE_VULKAN_ENTRY(vkCmdClearDepthStencilImage),
		MAKE_VULKAN_ENTRY(vkCmdClearAttachments),
		MAKE_VULKAN_ENTRY(vkCmdResolveImage),
		MAKE_VULKAN_ENTRY(vkCmdSetEvent),
		MAKE_VULKAN_ENTRY(vkCmdResetEvent),
		MAKE_VULKAN_ENTRY(vkCmdWaitEvents),
		MAKE_VULKAN_ENTRY(vkCmdPipelineBarrier),
		MAKE_VULKAN_ENTRY(vkCmdBeginQuery),
		MAKE_VULKAN_ENTRY(vkCmdEndQuery),
		MAKE_VULKAN_ENTRY(vkCmdResetQueryPool),
		MAKE_VULKAN_ENTRY(vkCmdWriteTimestamp),
		MAKE_VULKAN_ENTRY(vkCmdCopyQueryPoolResults),
		MAKE_VULKAN_ENTRY(vkCmdPushConstants),
		MAKE_VULKAN_ENTRY(vkCmdBeginRenderPass),
		MAKE_VULKAN_ENTRY(vkCmdNextSubpass),
		MAKE_VULKAN_ENTRY(vkCmdEndRenderPass),
		MAKE_VULKAN_ENTRY(vkCmdExecuteCommands),
		MAKE_VULKAN_ENTRY(vkDestroySurfaceKHR),
		MAKE_VULKAN_ENTRY(vkCreateSwapchainKHR),
		MAKE_VULKAN_ENTRY(vkDestroySwapchainKHR),
		MAKE_VULKAN_ENTRY(vkGetSwapchainImagesKHR),
		MAKE_VULKAN_ENTRY(vkAcquireNextImageKHR),
		MAKE_VULKAN_ENTRY(vkCreateWin32SurfaceKHR),
		MAKE_VULKAN_ENTRY(vkGetSwapchainStatusKHR)
	};
#undef MAKE_VULKAN_ENTRY

	void * VKAPI_CALL default_alloc_func(void *pUserData, size_t size, size_t align, VkSystemAllocationScope allocationScope)
	{
		assert(align == 8);
		return malloc(size);
	}

	void * VKAPI_CALL default_realloc_func(void *pUserData, void *pOriginal, size_t size, size_t align, VkSystemAllocationScope allocationScope)
	{
		return realloc(pOriginal, size);
	}

	void VKAPI_CALL default_free_func(void *pUserData, void *pMemory)
	{
		free(pMemory);
	}

	void VKAPI_PTR default_internal_alloc_func(void * pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
		assert(0);
	}

	void VKAPI_PTR default_internal_free_func(void * pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
	{
		assert(0);
	}
} 
