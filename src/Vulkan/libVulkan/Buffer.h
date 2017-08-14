#ifndef BUFFER_H
#define BUFFER_H
#include "Instance.h"
#include "Image.h"
#include "CommandAllocator.h"
#include "Common/Resource.hpp"

namespace vulkan
{
	struct Buffer
	{
		Device *device;
		VkDeviceSize size;
		VkBufferUsageFlags usage;
		VkDeviceSize offset;
		struct DeviceMemory *mem;
	};

	struct VertexBinding
	{
		Buffer buffer;
		VkDeviceSize offset;
		sw::Resource *resource;
	};

	struct Framebuffer
	{
		uint32_t width;
		uint32_t height;
		uint32_t layers;

		uint32_t attachmentCount;
		struct ImageView attachments;
		struct RenderPass *renderpass;
	};

	struct CommandPool
	{
		VkAllocationCallbacks alloc;
		uint32_t queueFamilyIndex;
		struct CommandBuffer *buffers;
	};

	struct CommandBuffer
	{
		Device *device;
		CommandPool *pool;
		VkCommandBufferUsageFlags usageFlags;
		VkCommandBufferLevel level;
		backend::CommandAllocator *cmdAllocator;
		backend::CommandIterator *cmdIterator;

		struct State
		{
			struct Pipeline *pipeline;
			struct Framebuffer *framebuffer;
			struct RenderPass *renderPass;
			VertexBinding *vertBind;
			VkRect2D renderArea;
			VkAttachmentDescription *attachments;
		}state;
	};

	struct Fence
	{
		bool submitted;
	};

	struct Semaphore
	{
		VkStructureType sType;
		VkSemaphoreCreateFlags flags;
	};
}
#endif
