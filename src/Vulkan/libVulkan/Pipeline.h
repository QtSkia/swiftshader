#ifndef PIPELINE_H
#define PIPELINE_H

namespace vulkan
{
	struct PipelineLayout
	{
		// Not really sure what to put in here since nothing gets passed
		uint32_t setCount;
	};

	struct Pipeline
	{
		Device *device;
		PipelineLayout *layout;
		VkSubpassDescription *subpass;
	};
}
#endif
