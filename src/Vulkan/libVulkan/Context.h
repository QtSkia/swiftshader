// Copyright 2017 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VULKAN_CONTEXT_H_
#define VULKAN_CONTEXT_H_

#include <unordered_map>
#include "vulkan/vulkan.h"

namespace vulkan
{
	extern const VkExtensionProperties global_ext[1];
	extern const VkExtensionProperties device_extensions[2];
	extern const std::unordered_map < std::string, PFN_vkVoidFunction > func_ptrs;

	void * VKAPI_CALL default_alloc_func(void *pUserData, size_t size, size_t align, VkSystemAllocationScope allocationScope);
	void * VKAPI_CALL default_realloc_func(void *pUserData, void *pOriginal, size_t size, size_t align, VkSystemAllocationScope allocationScope);
	void VKAPI_CALL default_free_func(void *pUserData, void *pMemory);
	void VKAPI_PTR default_internal_alloc_func(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
	void VKAPI_PTR default_internal_free_func(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);

	static const VkAllocationCallbacks default_alloc = {
		NULL,
		default_alloc_func,
		default_realloc_func,
		default_free_func,
		default_internal_alloc_func,
		default_internal_free_func
	};

	static const VkQueueFamilyProperties
		queue_family_properties = {
		VK_QUEUE_GRAPHICS_BIT |
		VK_QUEUE_COMPUTE_BIT |
		VK_QUEUE_TRANSFER_BIT,
		1, // queueCount
		0,
		{ 1, 1, 1 }, // Per spec: "Queues supporting graphics and/or compute operations must report (1,1,1) in minImageTransferGranularity, meaning that there are no additional restrictions on the granularity of image transfer operations for these queues."
	};



	struct PhysicalDevice
	{
		struct Instance   *instance;
		const char *const name;

		VkPhysicalDeviceMemoryProperties memory;

		PhysicalDevice() : instance(0), name("SwiftShader Device") {}
	};

	struct Instance
	{
		VkAllocationCallbacks                       alloc;

		uint32_t                                    apiVersion;
		int                                         physicalDeviceNum;
		PhysicalDevice                              *physicalDevice;
	};

	struct Device
	{
		struct Instance *instance;
		VkAllocationCallbacks alloc;
	};

	struct Sampler
	{
	};

	struct ShaderModule
	{
		std::vector<uint32_t> data;

		ShaderModule() {}
	};
}
#endif

