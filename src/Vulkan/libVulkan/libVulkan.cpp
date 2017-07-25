#include <stdio.h>
#include "Context.h"
#include "utils.h"
#include "Device.h"
#include "Context.h"
#include <assert.h>
#include <windows.h>
#include <iterator>

namespace vulkan
{
	constexpr uint32_t globalExtSize = sizeof(global_ext) / sizeof(global_ext[0]);
	constexpr uint32_t deviceExtSize = sizeof(device_extensions) / sizeof(device_extensions[0]);

	PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance, const char* pName)
	{
		return vkutils::findEntry(pName);
	}

	PFN_vkVoidFunction GetDeviceProcAddr(VkDevice device, const char* pName)
	{
		return vkutils::findEntry(pName);
	}

	VkResult EnumerateInstanceExtensionProperties(const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties)
	{
		if (pProperties == NULL)
		{
			*pPropertyCount = globalExtSize;
			return VK_SUCCESS;
		}

		*pPropertyCount = min(*pPropertyCount, globalExtSize);
		memcpy(pProperties, global_ext, *pPropertyCount);

		if (*pPropertyCount < globalExtSize)
		{
			return VK_INCOMPLETE;
		}

		return VK_SUCCESS;
	}

	VkResult CreateInstance(const VkInstanceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkInstance * pInstance)
	{
		Instance *instance = nullptr;

		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);

		uint32_t ClientVersion;

		// Per spec, must not be 0
		if (pCreateInfo->pApplicationInfo && pCreateInfo->pApplicationInfo->apiVersion != 0)
		{
			ClientVersion = pCreateInfo->pApplicationInfo->apiVersion;
		}
		else
		{
			ClientVersion = VK_MAKE_VERSION(1, 0, 0);
		}

		// TODO: Check more precisely for version
		if (VK_MAKE_VERSION(1, 0, 0) > ClientVersion)
		{
			return VK_ERROR_INCOMPATIBLE_DRIVER;
		}

		for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
		{
			bool foundExt = false;

			for (uint32_t j = 0; j < globalExtSize; ++j)
			{
				if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], global_ext[j].extensionName) == 0)
				{
					foundExt = true;
					break;
				}
			}

			if (!foundExt)
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		if (pCreateInfo->enabledLayerCount != 0)
		{
			return VK_ERROR_LAYER_NOT_PRESENT;
		}

		if (pAllocator == NULL)
		{
			instance = reinterpret_cast<vulkan::Instance *>(vkutils::Alloc(&default_alloc, pAllocator, sizeof(*instance), ALIGNMENT,
				VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE));
		}
		else
		{
			instance = reinterpret_cast<vulkan::Instance *>(pAllocator->pfnAllocation(instance, sizeof(*instance), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE));
		}

		if (instance == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		if (pAllocator)
		{
			instance->alloc = *pAllocator;
		}
		else
		{
			instance->alloc = default_alloc;
		}

		instance->apiVersion = ClientVersion;

		instance->physicalDevice = new PhysicalDevice;

		instance->physicalDevice->instance = instance;
		instance->physicalDevice->memory = { 0 };
		instance->physicalDeviceNum = 1;

		*pInstance = Instance_to_handle(instance);


		return VK_SUCCESS;
	}

	void DestroyInstance(VkInstance instance, const VkAllocationCallbacks * pAllocator)
	{
		GET_FROM_HANDLE(Instance, myInstance, instance);

		assert(myInstance != nullptr);

		if (pAllocator == NULL)
		{
			if (myInstance->physicalDevice != NULL)
			{
				delete myInstance->physicalDevice;
			}

			vkutils::Free(&myInstance->alloc, instance);
		}
		else
		{
			if (myInstance->physicalDevice != NULL)
			{
				delete myInstance->physicalDevice;
			}

			pAllocator->pfnFree(instance, instance);
		}
	}


	VkResult EnumeratePhysicalDevices(VkInstance instance, uint32_t * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevices)
	{
		GET_FROM_HANDLE(Instance, myInstance, instance);

		if (pPhysicalDevices == NULL)
		{
			*pPhysicalDeviceCount = 1;
			myInstance->physicalDeviceNum = *pPhysicalDeviceCount;
			return VK_SUCCESS;
		}

		assert(myInstance->physicalDeviceNum == 1);

		*pPhysicalDevices = reinterpret_cast<VkPhysicalDevice>(PhysicalDevice_to_handle(myInstance->physicalDevice));

		return VK_SUCCESS;
	}

	void GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties * pQueueFamilyProperties)
	{
		VkQueueFamilyProperties *props = pQueueFamilyProperties;

		if (props == NULL)
		{
			*pQueueFamilyPropertyCount = 1;
			return;
		}

		else
		{
			*pQueueFamilyProperties = queue_family_properties;
		}
	}

	void GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures * pFeatures)
	{
		*pFeatures = VkPhysicalDeviceFeatures{
			false, // textureCompressionETC2
			false, // textureCompressionASTC_LDR
			false, // textureCompressionBC
			false, // occlusionQueryPrecise
			false, // pipelineStatisticsQuery
			false, // vertexPipelineStoresAndAtomics
			false, // fragmentStoresAndAtomics
			false, // shaderTessellationAndGeometryPointSize
			false, // shaderImageGatherExtended
			false, // shaderStorageImageExtendedFormats
			false, // shaderStorageImageMultisample
			false, // shaderStorageImageReadWithoutFormat
			false, // shaderStorageImageWriteWithoutFormat
			false, // shaderUniformBufferArrayDynamicIndexing
			false, // shaderSampledImageArrayDynamicIndexing
			false, // shaderStorageBufferArrayDynamicIndexing
			false, // shaderStorageImageArrayDynamicIndexing
			false, // shaderClipDistance
			false, // shaderCullDistance
			false, // shaderFloat64
			false, // shaderInt64
			false, // shaderInt16
			false, // shaderResourceResidency
			false, // shaderResourceMinLod
			false, // sparseBinding
			false, // sparseResidencyBuffer
			false, // sparseResidencyImage2D
			false, // sparseResidencyImage3D
			false, // sparseResidency2Samples
			false, // sparseResidency4Samples
			false, // sparseResidency8Samples
			false, // sparseResidency16Samples
			false, // sparseResidencyAliased
			false, // variableMultisampleRate
			false  // inheritedQueries
		};
	}

	void GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties * pProperties)
	{
		GET_FROM_HANDLE(PhysicalDevice, pDevice, physicalDevice);

		VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;

		VkPhysicalDeviceLimits limits = {
			(1 << 14), // maxImageDimension1D
			(1 << 14), // maxImageDimension2D
			(1 << 11), // maxImageDimension3D
			(1 << 14), // maxImageDimensionCube
			(1 << 11), // maxImageArrayLayers
			128 * 1024 * 1024, // maxTexelBufferElements
			(1ul << 27), // maxUniformBufferRange
			16, // maxStorageBufferRange
			128, // maxPushConstantsSize
			UINT32_MAX, // maxMemoryAllocationCount
			64 * 1024, // maxSamplerAllocationCount
			64, // bufferImageGranularity
			0, // sparseAddressSpaceSize
			8, // maxBoundDescriptorSets
			64, // maxPerStageDescriptorSamplers
			64, // maxPerStageDescriptorUniformBuffers
			64, // maxPerStageDescriptorStorageBuffers
			64, // maxPerStageDescriptorSampledImages
			64, // maxPerStageDescriptorStorageImages
			64, // maxPerStageDescriptorInputAttachments
			128, // maxPerStageResources
			256, // maxDescriptorSetSamplers
			256, // maxDescriptorSetUniformBuffers
			8, // maxDescriptorSetUniformBuffersDynamic
			256, // maxDescriptorSetStorageBuffers
			8, // maxDescriptorSetStorageBuffersDynamic
			256, // maxDescriptorSetSampledImages
			256, // maxDescriptorSetStorageImages
			256, // maxDescriptorSetInputAttachments
			31, // maxVertexInputAttributes
			31, // maxVertexInputBindings
			2047, // maxVertexInputAttributeOffset
			2048, // maxVertexInputBindingStride
			128, // maxVertexOutputComponents
			64, // maxTessellationGenerationLevel
			32, // maxTessellationPatchSize
			128, // maxTessellationControlPerVertexInputComponents
			128, // maxTessellationControlPerVertexOutputComponents
			128, // maxTessellationControlPerPatchOutputComponents
			2048, // maxTessellationControlTotalOutputComponents
			128, // maxTessellationEvaluationInputComponents
			128, // maxTessellationEvaluationOutputComponents
			32, // maxGeometryShaderInvocations
			64, // maxGeometryInputComponents
			128, // maxGeometryOutputComponents
			256, // maxGeometryOutputVertices
			1024, // maxGeometryTotalOutputComponents
			128, // maxFragmentInputComponents
			8, // maxFragmentOutputAttachments
			1, // maxFragmentDualSrcAttachments
			8, // maxFragmentCombinedOutputResources
			32768, // maxComputeSharedMemorySize
			{ 65535, 65535, 65535 }, // maxComputeWorkGroupCount[3]
			16, // maxComputeWorkGroupInvocations
			{ 16, 16, 16,}, // maxComputeWorkGroupSize[3]
			4, // subPixelPrecisionBits
			4, // subTexelPrecisionBits
			4, // mipmapPrecisionBits
			UINT32_MAX, // maxDrawIndexedIndexValue
			UINT32_MAX, // maxDrawIndirectCount
			16, // maxSamplerLodBias
			16, // maxSamplerAnisotropy
			16, // maxViewports
			{ (1 << 14), (1 << 14) }, // maxViewportDimensions[2]
			{ INT16_MIN, INT16_MAX }, // viewportBoundsRange[2]
			13, // viewportSubPixelBits
			4096, // minMemoryMapAlignment
			1, // minTexelBufferOffsetAlignment
			16, // minUniformBufferOffsetAlignment
			4, // minStorageBufferOffsetAlignment
			-8, // minTexelOffset
			7, // maxTexelOffset
			-32, // minTexelGatherOffset
			31, // maxTexelGatherOffset
			-0.5, // minInterpolationOffset
			0.4375, // maxInterpolationOffset
			4, // subPixelInterpolationOffsetBits
			(1 << 14), // maxFramebufferWidth
			(1 << 14), // maxFramebufferHeight
			(1 << 11), // maxFramebufferLayers
			sampleCounts, // framebufferColorSampleCounts
			sampleCounts, // framebufferDepthSampleCounts
			sampleCounts, // framebufferStencilSampleCounts
			sampleCounts, // framebufferNoAttachmentsSampleCounts
			8,  // maxColorAttachments
			sampleCounts, // sampledImageColorSampleCounts
			VK_SAMPLE_COUNT_1_BIT, // sampledImageIntegerSampleCounts
			sampleCounts, // sampledImageDepthSampleCounts
			sampleCounts, // sampledImageStencilSampleCounts
			VK_SAMPLE_COUNT_1_BIT, // storageImageSampleCounts
			1, // maxSampleMaskWords
			false, // timestampComputeAndGraphics
			60, // timestampPeriod
			8, // maxClipDistances
			8, // maxCullDistances
			8, // maxCombinedClipAndCullDistances
			1, // discreteQueuePriorities
			{ 0.125, 255.875 }, // pointSizeRange[2]
			{ 0.0, 7.9921875 }, // lineWidthRange[2]
			(1.0 / 8.0), // pointSizeGranularity
			(1.0 / 128.0), // lineWidthGranularity
			false, // strictLines
			true, // standardSampleLocations
			128, // optimalBufferCopyOffsetAlignment
			128, // optimalBufferCopyRowPitchAlignment
			64 // nonCoherentAtomSize
		};

		*pProperties = VkPhysicalDeviceProperties{
			VK_MAKE_VERSION(1, 0, 0), // apiVersion
			1, // driverVersion
			0x1111, // vendorID
			1, // deviceID
			VK_PHYSICAL_DEVICE_TYPE_CPU, // deviceType
			"SwiftShader Device", // deviceName
			{0}, // pipelineCacheUUID
			limits, // limits
			{0} // sparseProperties
		};
	}

	VkResult EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties)
	{
		if (pProperties == NULL)
		{
			*pPropertyCount = deviceExtSize;
			return VK_SUCCESS;
		}

		*pPropertyCount = min(*pPropertyCount, deviceExtSize);
		memcpy(pProperties, device_extensions, *pPropertyCount);

		if (*pPropertyCount < deviceExtSize)
		{
			return VK_INCOMPLETE;
		}

		return VK_SUCCESS;

	}

	VkResult CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDevice * pDevice)
	{
		GET_FROM_HANDLE(PhysicalDevice, physDevice, physicalDevice);
		struct Device *device;

		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);

		for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
		{
			bool found = false;

			for (uint32_t j = 0; j < sizeof(device_extensions) / sizeof(device_extensions[0]); ++j)
			{
				if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], device_extensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		if (pCreateInfo->enabledLayerCount != 0)
		{
			return VK_ERROR_LAYER_NOT_PRESENT;
		}

		if (pAllocator == NULL)
		{
			device = reinterpret_cast<Device *>(vkutils::Alloc(&physDevice->instance->alloc, pAllocator, sizeof(*device), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE));
		}
		else
		{
			device = reinterpret_cast<Device *>(pAllocator->pfnAllocation(device, sizeof(*device), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE));
		}

		if (device == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		device->instance = physDevice->instance;

		if (pAllocator)
		{
			device->alloc = *pAllocator;
		}
		else
		{
			device->alloc = physDevice->instance->alloc;
		}

		// TODO: Check for features and return VK_ERROR_FEATURE_NOT PRESENT if needed
		*pDevice = Device_to_handle(device);

		return VK_SUCCESS;
	}

	void GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties * pMemoryProperties)
	{
		GET_FROM_HANDLE(PhysicalDevice, physDevice, physicalDevice);

		*pMemoryProperties = physDevice->memory;
	}

	VkResult CreateSampler(VkDevice device, const VkSamplerCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSampler * pSampler)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		Sampler *sampler = nullptr;

		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

		if (pAllocator == NULL)
		{
			sampler = reinterpret_cast<Sampler *>(vkutils::Alloc(&myDevice->alloc, pAllocator, sizeof(*sampler), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT));
		}
		else
		{
			sampler = reinterpret_cast<Sampler *>(pAllocator->pfnAllocation(sampler, sizeof(*sampler), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT));
		}

		if (sampler == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		*pSampler = Sampler_to_handle(sampler);

		return VK_SUCCESS;
	}

	void DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks * pAllocator)
	{
		assert(sampler != NULL);

		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(Sampler, mySampler, sampler);

		if (pAllocator == NULL)
		{
			vkutils::Free(&myDevice->alloc, mySampler);
		}
		else
		{
			pAllocator->pfnFree(device, mySampler);
		}
	}

	VkResult CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkShaderModule * pShaderModule)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
		assert(pCreateInfo->flags == 0);

		ShaderModule *module = nullptr;

		if (pAllocator == NULL)
		{
			module = new (vkutils::Alloc(&myDevice->alloc, pAllocator, sizeof(*module), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) ShaderModule;
		}
		else
		{
			module = new (pAllocator->pfnAllocation(device, sizeof(*module), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) ShaderModule;
		}

		if (module == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		module->data.resize(pCreateInfo->codeSize / 4);
		module->data.assign(pCreateInfo->pCode, pCreateInfo->pCode + (pCreateInfo->codeSize / 4));

		*pShaderModule = ShaderModule_to_handle(module);

		return VK_SUCCESS;
	}

	void DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks * pAllocator)
	{
		assert(shaderModule != NULL);

		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(ShaderModule, myShaderModule, shaderModule);

		if (pAllocator == NULL)
		{
			vkutils::Free(&myDevice->alloc, myShaderModule);
		}
		else
		{
			pAllocator->pfnFree(nullptr, myShaderModule);
		}
	}

}
