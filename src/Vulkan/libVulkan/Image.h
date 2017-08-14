#ifndef IMAGE_H
#define IMAGE_H

#include "Instance.h"
namespace vulkan
{
	struct Image
	{
		VkImageType imageType;
		VkFormat format;
		VkExtent3D extent;
		uint32_t levels;
		uint32_t arraySize;
		uint32_t samples;
		VkImageUsageFlags usage;
		VkImageTiling tiling;
		VkDeviceSize size;
		uint32_t alignment;
		VkDeviceSize offset;
		struct DeviceMemory *mem;
	};

	struct ImageView
	{
		const Image *image;
		VkImageAspectFlags aspectMask;
		VkFormat format;
	};
}


#endif