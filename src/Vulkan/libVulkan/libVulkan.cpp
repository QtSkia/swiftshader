#include <stdio.h>
#include <assert.h>
#include "Instance.h"
#include "utils.h"
#include "CommandAllocator.h"
#include "Device.hpp"
#include "Renderer/Context.hpp"
#include "Common/Resource.hpp"


namespace vulkan
{
	struct BufferView // Implementation specific
	{
		const VkBufferViewCreateInfo* createInfo;
	};

	constexpr uint32_t globalExtSize = sizeof(global_ext) / sizeof(global_ext[0]);
	constexpr uint32_t deviceExtSize = sizeof(device_extensions) / sizeof(device_extensions[0]);

	template <typename T>
	T* AllocateObject(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		T* object = nullptr;

		if(pAllocator == NULL)
		{
			object = new (vkutils::Alloc(&myDevice->alloc, pAllocator, sizeof(T), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) T;
		}
		else
		{
			object = new (pAllocator->pfnAllocation(myDevice, sizeof(T), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) T;
		}

		return object;
	}

	template <typename T>
	void DestroyObject(VkDevice device, T object, const VkAllocationCallbacks * pAllocator)
	{
		if(object == VK_NULL_HANDLE)
		{
			return;
		}

		GET_FROM_HANDLE(Device, myDevice, device);
		GET_UNTYPED_FROM_HANDLE(myObject, object);

		if(pAllocator == NULL)
		{
			vkutils::Free(&myDevice->alloc, myObject);
		}
		else
		{
			pAllocator->pfnFree(myDevice, myObject);
		}
	}

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

		*pPropertyCount = MIN(*pPropertyCount, globalExtSize);
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
			instance = reinterpret_cast<vulkan::Instance *>(pAllocator->pfnAllocation(nullptr, sizeof(*instance), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE));
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
		// Need to fill in these memory values to pass getbuffermem tests
		instance->physicalDevice->memory.memoryHeapCount = 1;
		instance->physicalDevice->memory.memoryHeaps[0] = {
			3ull * (1ull << 30), //size
			VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
		};
		instance->physicalDevice->memory.memoryTypeCount = 1;
		instance->physicalDevice->memory.memoryTypes[0] = {
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0
		};
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

	void GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
	{
		pFormatProperties->linearTilingFeatures = 0; // Unsupported format
		pFormatProperties->optimalTilingFeatures = 0; // Unsupported format
		pFormatProperties->bufferFeatures = 0; // Unsupported format
	}

	VkResult EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties)
	{
		if (pProperties == NULL)
		{
			*pPropertyCount = deviceExtSize;
			return VK_SUCCESS;
		}

		*pPropertyCount = MIN(*pPropertyCount, deviceExtSize);
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
		struct Device *device = nullptr;

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
			device = reinterpret_cast<Device *>(pAllocator->pfnAllocation(physDevice, sizeof(*device), ALIGNMENT, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE));
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

		device->context = new sw::Context();
		device->swiftshaderDevice = new SwDevice(device->context);
		device->queue.device = device;
		device->surface = nullptr;

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
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

		Sampler *sampler = AllocateObject<Sampler>(device, pAllocator);

		if (sampler == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		*pSampler = Sampler_to_handle(sampler);

		return VK_SUCCESS;
	}

	void DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, sampler, pAllocator);
	}

	VkResult CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkShaderModule * pShaderModule)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
		assert(pCreateInfo->flags == 0);

		ShaderModule *module = AllocateObject<ShaderModule>(device, pAllocator);

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
		DestroyObject(device, shaderModule, pAllocator);
	}

	void GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue * pQueue)
	{
		assert(queueIndex == 0);
		GET_FROM_HANDLE(Device, myDevice, device);

		*pQueue = Queue_to_handle(&myDevice->queue);
	}

	VkResult CreateBuffer(VkDevice device, const VkBufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkBuffer * pBuffer)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);

		GET_FROM_HANDLE(Device, myDevice, device);

		Buffer *buffer = AllocateObject<Buffer>(device, pAllocator);

		if (buffer == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		buffer->device = myDevice;
		buffer->usage = pCreateInfo->usage;
		buffer->offset = sizeof(*myDevice);
		buffer->size = pCreateInfo->size;

		*pBuffer = Buffer_to_handle(buffer);

		return VK_SUCCESS;
	}

	void DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, buffer, pAllocator);
	}

	VkResult CreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
	{
		BufferView* bufferView = AllocateObject<BufferView>(device, pAllocator);

		bufferView->createInfo = pCreateInfo;

		*pView = BufferView_to_handle(bufferView);

		return VK_SUCCESS;
	}

	void DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
	{
		DestroyObject(device, bufferView, pAllocator);
	}

	void GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(Buffer, myBuffer, buffer);

		PhysicalDevice *pPhysDevice = myDevice->instance->physicalDevice;

		pMemoryRequirements->size = myBuffer->size;
		pMemoryRequirements->alignment = ALIGNMENT * 2;

		uint32_t memoryTypes = 0;

		for (uint32_t i = 0; i < myDevice->instance->physicalDevice->memory.memoryTypeCount; ++i)
		{
			memoryTypes |= (1u << i);
		}
		pMemoryRequirements->memoryTypeBits = memoryTypes;
	}

	VkResult AllocateMemory(VkDevice device, const VkMemoryAllocateInfo * pAllocateInfo, const VkAllocationCallbacks * pAllocator, VkDeviceMemory * pMemory)
	{
		assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
		// per spec, must be greater than 0
		assert(pAllocateInfo->allocationSize > 0);

		GET_FROM_HANDLE(Device, myDevice, device);
		PhysicalDevice *pPhysDevice = myDevice->instance->physicalDevice;

		if (pAllocateInfo->allocationSize > (1ull << 31))
		{
			return VK_ERROR_OUT_OF_DEVICE_MEMORY;
		}

		DeviceMemory *memory = AllocateObject<DeviceMemory>(device, pAllocator);

		if (memory == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		memory->type = pPhysDevice->memory.memoryTypes[pAllocateInfo->memoryTypeIndex];
		memory->size = pAllocateInfo->allocationSize;
		memory->map = malloc(pAllocateInfo->allocationSize);

		*pMemory = DeviceMemory_to_handle(memory);

		return VK_SUCCESS;
	}

	VkResult MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(DeviceMemory, myMem, memory);

		if (myMem == NULL)
		{
			*ppData = NULL;
			return VK_SUCCESS;
		}

		assert(size <= myMem->size - offset);

		*ppData = (unsigned char *)myMem->map + offset;
		return VK_SUCCESS;
	}

	VkResult BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
	{
		GET_FROM_HANDLE(DeviceMemory, devMem, memory);
		GET_FROM_HANDLE(Buffer, myBuf, buffer);

		if (devMem)
		{
			myBuf->offset = memoryOffset;
		}
		else
		{
			myBuf->offset = 0;
		}

		myBuf->mem = devMem;

		return VK_SUCCESS;
	}

	VkResult CreateImage(VkDevice device, const VkImageCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImage * pImage)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
		assert(pCreateInfo->mipLevels > 0);
		assert(pCreateInfo->arrayLayers > 0);
		assert(pCreateInfo->samples > 0);
		assert(pCreateInfo->extent.width > 0);
		assert(pCreateInfo->extent.height > 0);
		assert(pCreateInfo->extent.depth > 0);

		Image *image = AllocateObject<Image>(device, pAllocator);

		if (image == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		memset(image, 0, sizeof(*image));
		image->imageType = pCreateInfo->imageType;
		image->format = pCreateInfo->format;
		image->size = pCreateInfo->extent.width * pCreateInfo->extent.height * vkutils::formatSize(image->format);
		image->extent = pCreateInfo->extent;
		image->levels = pCreateInfo->mipLevels;
		image->arraySize = pCreateInfo->arrayLayers;
		image->samples = pCreateInfo->samples;
		image->tiling = pCreateInfo->tiling;
		image->usage = pCreateInfo->usage;

		*pImage = Image_to_handle(image);
		return VK_SUCCESS;
	}

	void DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, image, pAllocator);
	}

	void GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements * pMemoryRequirements)
	{
		GET_FROM_HANDLE(Image, myImage, image);
		GET_FROM_HANDLE(Device, myDevice, device);

		PhysicalDevice *pPhysDevice = myDevice->instance->physicalDevice;
		uint32_t memTypes = (1ull << pPhysDevice->memory.memoryTypeCount) - 1;

		pMemoryRequirements->size = myImage->size;
		pMemoryRequirements->alignment = myImage->alignment;
		pMemoryRequirements->memoryTypeBits = memTypes;
	}

	void UnmapMemory(VkDevice device, VkDeviceMemory memory)
	{
		GET_FROM_HANDLE(DeviceMemory, mem, memory);

		assert(mem != NULL);

		free(mem->map);

		mem->map = NULL;
		mem->size = 0;
	}

	void FreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks * pAllocator)
	{
		if(memory == VK_NULL_HANDLE)
		{
			return;
		}

		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(DeviceMemory, mem, memory);

		assert(mem != NULL);

		if (mem->map)
		{
			UnmapMemory(device, memory);
		}

		if (pAllocator == NULL)
		{
			vkutils::Free(&myDevice->alloc, mem);
		}
		else
		{
			pAllocator->pfnFree(nullptr, mem);
		}
	}

	VkResult BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
	{
		GET_FROM_HANDLE(Image, myImage, image);
		GET_FROM_HANDLE(DeviceMemory, mem, memory);

		if (mem)
		{
			myImage->offset = memoryOffset;
		}
		else
		{
			myImage->offset = 0;
		}

		myImage->mem = mem;

		return VK_SUCCESS;
	}

	VkResult CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

		RenderPass *pRender = AllocateObject<RenderPass>(device, pAllocator);

		if (pRender == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		pRender->subpassCount = pCreateInfo->subpassCount;
		pRender->subpass[0] = pCreateInfo->pSubpasses[0];
		pRender->attachmentCount = pCreateInfo->attachmentCount;
		pRender->attachments = reinterpret_cast<VkAttachmentDescription *>(malloc(sizeof(*pRender->attachments) * pRender->attachmentCount));
		pRender->attachments[0] = pCreateInfo->pAttachments[0];

		*pRenderPass = RenderPass_to_handle(pRender);

		return VK_SUCCESS;
	}

	void DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks * pAllocator)
	{
		if(renderPass == VK_NULL_HANDLE)
		{
			return;
		}

		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(RenderPass, render, renderPass);

		if (pAllocator == NULL)
		{
			free(render->attachments);
			vkutils::Free(&myDevice->alloc, render);
		}
		else
		{
			free(render->attachments);
			pAllocator->pfnFree(nullptr, render);
		}
	}

	VkResult CreateImageView(VkDevice device, const VkImageViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImageView * pView)
	{
		GET_FROM_HANDLE(Image, image, pCreateInfo->image);

		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);

		ImageView *view = AllocateObject<ImageView>(device, pAllocator);

		if (view == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		view->image = image;
		view->aspectMask = pCreateInfo->subresourceRange.aspectMask;
		view->format = pCreateInfo->format;

		*pView = ImageView_to_handle(view);
		return VK_SUCCESS;
	}

	void DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, imageView, pAllocator);
	}

	VkResult CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkPipelineLayout * pPipelineLayout)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);

		PipelineLayout *pipelineLayout = AllocateObject<PipelineLayout>(device, pAllocator);

		if (pipelineLayout == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		pipelineLayout->setCount = pCreateInfo->setLayoutCount;

		*pPipelineLayout = PipelineLayout_to_handle(pipelineLayout);

		return VK_SUCCESS;
	}

	void DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, pipelineLayout, pAllocator);
	}

	VkResult CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator, VkPipeline * pPipelines)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		// We do not yet support pipelineCache being enabled.
		assert(pipelineCache == NULL);
		assert(pCreateInfos->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

		Pipeline *pipeline = AllocateObject<Pipeline>(device, pAllocator);
		myDevice->pipeline = pipeline;

		memset(pipeline, 0, sizeof(*pipeline));
		pipeline->device = myDevice;
		pipeline->layout = PipelineLayout_from_handle(pCreateInfos->layout);
		pipeline->layout->vertData.format = pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format;
		pipeline->layout->vertData.stride = pCreateInfos->pVertexInputState->pVertexBindingDescriptions->stride;
		pipeline->layout->viewportData.x = pCreateInfos->pViewportState->pViewports->x;
		pipeline->layout->viewportData.y = pCreateInfos->pViewportState->pViewports->y;
		pipeline->layout->viewportData.width = pCreateInfos->pViewportState->pViewports->width;
		pipeline->layout->viewportData.height = pCreateInfos->pViewportState->pViewports->height;
		pipeline->layout->viewportData.minDepth = pCreateInfos->pViewportState->pViewports->minDepth;
		pipeline->layout->viewportData.maxDepth = pCreateInfos->pViewportState->pViewports->maxDepth;

		VkPipelineShaderStageCreateInfo pStages[2];
		struct ShaderModule *modules[2];

		for (uint32_t i = 0; i < pCreateInfos->stageCount; ++i)
		{
			pStages[i] = pCreateInfos->pStages[i];
			modules[i] = ShaderModule_from_handle(pStages[i].module);
		}

		// Here is where we should compile the shaders.
		// Rather, we hard code the shaders for now.

		*pPipelines = Pipeline_to_handle(pipeline);

		return VK_SUCCESS;
	}

	void DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, pipeline, pAllocator);
	}

	VkResult CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFramebuffer * pFramebuffer)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

		Framebuffer *framebuffer = AllocateObject<Framebuffer>(device, pAllocator);

		if (framebuffer == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		framebuffer->layers = pCreateInfo->layers;
		framebuffer->attachmentCount = pCreateInfo->attachmentCount;
		framebuffer->width = pCreateInfo->width;
		framebuffer->height = pCreateInfo->height;
		framebuffer->renderpass = RenderPass_from_handle(pCreateInfo->renderPass);
		framebuffer->attachments = *ImageView_from_handle(pCreateInfo->pAttachments[0]);

		*pFramebuffer = Framebuffer_to_handle(framebuffer);

		return VK_SUCCESS;
	}

	void DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, framebuffer, pAllocator);
	}

	VkResult CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkCommandPool * pCommandPool)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);

		CommandPool *cmdPool = AllocateObject<CommandPool>(device, pAllocator);

		if (cmdPool == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		if (pAllocator == NULL)
		{
			cmdPool->alloc = myDevice->alloc;
		}
		else
		{
			cmdPool->alloc = *pAllocator;
		}

		cmdPool->queueFamilyIndex = pCreateInfo->queueFamilyIndex;

		*pCommandPool = CommandPool_to_handle(cmdPool);

		return VK_SUCCESS;
	}

	void DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, commandPool, pAllocator);
	}

	VkResult AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo * pAllocateInfo, VkCommandBuffer * pCommandBuffers)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(CommandPool, cmdPool, pAllocateInfo->commandPool);

		assert(pAllocateInfo->commandBufferCount > 0);
		assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);

		CommandBuffer *cmdBuf = AllocateObject<CommandBuffer>(device, nullptr);

		if (cmdBuf == NULL)
		{
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}

		cmdBuf->device = myDevice;
		cmdBuf->level = pAllocateInfo->level;
		cmdBuf->pool = cmdPool;
		cmdPool->buffers = cmdBuf;
		cmdBuf->state.vertBind = reinterpret_cast<VertexBinding *>(malloc(sizeof(VertexBinding)));
		cmdBuf->cmdAllocator = new backend::CommandAllocator;

		*pCommandBuffers = CommandBuffer_to_handle(cmdBuf);

		return VK_SUCCESS;
	}

	void FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(CommandPool, cmdPool, commandPool);
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, *pCommandBuffers);

		free(cmdBuf->state.vertBind);
		delete cmdBuf->cmdAllocator;
		vkutils::Free(&myDevice->alloc, cmdBuf);
	}

	VkResult BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo * pBeginInfo)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);
		assert(pBeginInfo->sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

		// Start recording

		return VK_SUCCESS;
	}

	void CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);
		GET_FROM_HANDLE(Pipeline, pipe, pipeline);

		cmdBuf->state.pipeline = pipe;

		backend::SetRenderPipelineCmd *setPipe = cmdBuf->cmdAllocator->Allocate<backend::SetRenderPipelineCmd>(backend::Command::SetPipeline);

		new (setPipe) backend::SetRenderPipelineCmd;
		setPipe->pipeline = pipe; 
	}

	void CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);
	}

	void CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, VkSubpassContents contents)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);
		GET_FROM_HANDLE(RenderPass, renderPass, pRenderPassBegin->renderPass);
		GET_FROM_HANDLE(Framebuffer, framebuffer, pRenderPassBegin->framebuffer);

		assert(pRenderPassBegin->sType == VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

		cmdBuf->state.renderPass = renderPass;
		cmdBuf->state.framebuffer = framebuffer;
		cmdBuf->state.renderArea = pRenderPassBegin->renderArea;

		backend::BeginRenderPassCmd *beginRender = cmdBuf->cmdAllocator->Allocate<backend::BeginRenderPassCmd>(backend::Command::BeginRenderPass);

		new (beginRender) backend::BeginRenderPassCmd;
		beginRender->renderPass = cmdBuf->state.renderPass;
		beginRender->framebuffer = cmdBuf->state.framebuffer;
	}

	void CmdEndRenderPass(VkCommandBuffer commandBuffer)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);

		backend::EndRenderPassCmd *endRender = cmdBuf->cmdAllocator->Allocate<backend::EndRenderPassCmd>(backend::Command::EndRenderPass);

		new (endRender) backend::EndRenderPassCmd;

		cmdBuf->state.framebuffer = NULL;
		cmdBuf->state.pipeline = NULL;
		cmdBuf->state.renderPass = NULL;
	}

	void CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);

		backend::SetVertexBufferCmd *setVert = cmdBuf->cmdAllocator->Allocate<backend::SetVertexBufferCmd>(backend::Command::SetVertex);

		new (setVert) backend::SetVertexBufferCmd;
		setVert->buffer = Buffer_from_handle(*pBuffers);
		setVert->offset = *pOffsets;

		VertexBinding *vb = cmdBuf->state.vertBind;

		vb->buffer = *Buffer_from_handle(*pBuffers);
		vb->offset = *pOffsets;
	}

	void CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);

		backend::DrawArraysCmd *draw = cmdBuf->cmdAllocator->Allocate<backend::DrawArraysCmd>(backend::Command::DrawArrays);

		new (draw) backend::DrawArraysCmd;
		draw->vertexCount = vertexCount;
		draw->instanceCount = instanceCount;
		draw->firstVertex = firstVertex;
		draw->firstInstance = firstInstance;
	}

	void CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy * pRegions)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);

		assert(regionCount == 1);

		backend::CopyImageToBufferCmd *cpyImage = cmdBuf->cmdAllocator->Allocate<backend::CopyImageToBufferCmd>(backend::Command::CopyImageBuffer);

		new (cpyImage) backend::CopyImageToBufferCmd;
		cpyImage->dstBuffer = Buffer_from_handle(dstBuffer);
		cpyImage->srcImage = Image_from_handle(srcImage);
	}

	VkResult EndCommandBuffer(VkCommandBuffer commandBuffer)
	{
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, commandBuffer);

		cmdBuf->cmdIterator = new backend::CommandIterator(std::move(*cmdBuf->cmdAllocator));

		return VK_SUCCESS;
	}

	VkResult FlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		assert(memoryRangeCount == 1);

		DeviceMemory mem = *DeviceMemory_from_handle(pMemoryRanges->memory);
		mem.map = NULL;
		mem.size = 0;
		mem.type.heapIndex = 0;
		mem.type.propertyFlags = 0;

		return VK_SUCCESS;
	}

	VkResult CreateFence(VkDevice device, const VkFenceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFence * pFence)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);

		Fence *fence = AllocateObject<Fence>(device, pAllocator);

		fence->submitted = false;

		*pFence = Fence_to_handle(fence);

		return VK_SUCCESS;
	}

	VkResult CreateASemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
	{
		assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);

		Semaphore *semaphore = AllocateObject<Semaphore>(device, pAllocator);

		semaphore->sType = pCreateInfo->sType;
		semaphore->flags = pCreateInfo->flags;

		*pSemaphore = Semaphore_to_handle(semaphore);

		return VK_SUCCESS;
	}

	void DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
	{
		DestroyObject(device, semaphore, pAllocator);
	}

	VkResult QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo * pSubmits, VkFence fence)
	{
		GET_FROM_HANDLE(Queue, myQueue, queue);
		GET_FROM_HANDLE(Fence, myFence, fence);

		assert(pSubmits->sType == VK_STRUCTURE_TYPE_SUBMIT_INFO);
		assert(submitCount == 1);
		assert(pSubmits->commandBufferCount == 1);

		// Get our command buffer to get access to our iterator to iterate over the commands
		GET_FROM_HANDLE(CommandBuffer, cmdBuf, *pSubmits->pCommandBuffers);

		Device *device = myQueue->device;

		cmdBuf->state.vertBind->resource = new sw::Resource(0);
		sw::Resource *resource = cmdBuf->state.vertBind->resource;

		device->swiftshaderDevice->setup();

		backend::Command type;
		while (cmdBuf->cmdIterator->NextCommandId(&type))
		{
			switch (type)
			{

			case backend::Command::BeginRenderPass:
				{
					backend::BeginRenderPassCmd* cmd = cmdBuf->cmdIterator->NextCommand<backend::BeginRenderPassCmd>();
					const Image *image = cmd->framebuffer->attachments.image;
					int width = image->extent.width;
					int height = image->extent.height;
					int depth = image->extent.depth;
					int formatSize = vkutils::formatSize(image->format);

					void *pixelBuf = image->mem->map;
					device->surface = new sw::Surface(width, height, depth, sw::Format::FORMAT_A8B8G8R8, pixelBuf, width * formatSize, width * height * formatSize);
					device->swiftshaderDevice->setRenderTarget(0, device->surface);

					device->swiftshaderDevice->setViewport(Viewport(0, 0, width, height, 0, depth));

					// TEST CODE
					sw::SliceRect rect(0, 0, width, height, 0);
					int color = 0xFFBF4020;

					device->swiftshaderDevice->clear(&color, sw::Format::FORMAT_A8B8G8R8, device->surface, rect, 0xF);
					// END TEST CODE
				}
			break;

			case backend::Command::CopyImageBuffer:
				{
					backend::CopyImageToBufferCmd *cpy = cmdBuf->cmdIterator->NextCommand<backend::CopyImageToBufferCmd>();

					device->surface->sync();
					memcpy(cpy->dstBuffer->mem->map, cpy->srcImage->mem->map, cpy->srcImage->mem->size);
				}
			break;

			case backend::Command::DrawArrays:
				{
					backend::DrawArraysCmd *draw = cmdBuf->cmdIterator->NextCommand<backend::DrawArraysCmd>();

					device->swiftshaderDevice->drawPrimitive(sw::DrawType::DRAW_TRIANGLELIST, 1);

					resource->destruct();
				}
			break;

			case backend::Command::EndRenderPass:
				{
					backend::EndRenderPassCmd *endRender = cmdBuf->cmdIterator->NextCommand<backend::EndRenderPassCmd>();
				}
			break;

			case backend::Command::SetPipeline:
				{
					backend::SetRenderPipelineCmd *setPipe = cmdBuf->cmdIterator->NextCommand<backend::SetRenderPipelineCmd>();
				}
			break;

			case backend::Command::SetVertex:
				{
					backend::SetVertexBufferCmd *setVert = cmdBuf->cmdIterator->NextCommand<backend::SetVertexBufferCmd>();

					const void *buffer = (char*)setVert->buffer->mem->map + setVert->offset;
					float vertFormat = vkutils::formatSize(device->pipeline->layout->vertData.format);
					int count = (setVert->buffer->size / vertFormat) / vertFormat;
					int vertStride = device->pipeline->layout->vertData.stride;

					device->swiftshaderDevice->setConstantColor(0, sw::Color<float>(1, 0, 1, 1));
					device->swiftshaderDevice->setFirstArgument(0, sw::TextureStage::SOURCE_CONSTANT);
					device->swiftshaderDevice->setStageOperation(0, sw::TextureStage::STAGE_SELECTARG1);
					device->swiftshaderDevice->setFirstArgumentAlpha(0, sw::TextureStage::SOURCE_CONSTANT);
					device->swiftshaderDevice->setStageOperationAlpha(0, sw::TextureStage::STAGE_SELECTARG1);

					sw::Stream attribute(resource, buffer, vertStride);

					attribute.type = sw::StreamType::STREAMTYPE_FLOAT;
					attribute.count = vertFormat;
					attribute.normalized = false;

					int stream = 0;
					device->swiftshaderDevice->setInputStream(stream, attribute);

				}
			break;
			}
		}

		device->surface->sync();
		delete device->surface;

		return VK_SUCCESS;
	}

	void DestroyDevice(VkDevice device, const VkAllocationCallbacks * pAllocator)
	{
		GET_FROM_HANDLE(Device, myDevice, device);

		delete myDevice->swiftshaderDevice;
		delete myDevice->context;

		if (pAllocator == NULL)
		{
			vkutils::Free(&myDevice->instance->alloc, myDevice);
		}

		else
		{
			pAllocator->pfnFree(nullptr, myDevice);
		}
	}

	VkResult WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence * pFences, VkBool32 waitAll, uint64_t timeout)
	{
		GET_FROM_HANDLE(Device, myDevice, device);
		GET_FROM_HANDLE(Fence, fence, *pFences);

		fence->submitted = true;
		return VK_SUCCESS;
	}

	void DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks * pAllocator)
	{
		DestroyObject(device, fence, pAllocator);
	}

	VkResult InvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges)
	{
		assert(memoryRangeCount == 1);
		return VK_SUCCESS;
	}


}
