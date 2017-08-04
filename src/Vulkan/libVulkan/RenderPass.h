#ifndef RENDER_PASS_H
#define RENDER_PASS_H
#include "Instance.h"

namespace vulkan
{
	struct RenderPass
	{
		uint32_t attachmentCount;
		uint32_t subpassCount;
		VkAttachmentDescription *attachments;
		VkSubpassDescription subpass[1];
	};
}
#endif
