#include <string.h>
#include <map>
#include "utils.h"
#include "Context.h"

namespace vkutils
{
	PFN_vkVoidFunction findEntry(const char *name)
	{
		auto pFunc = vulkan::func_ptrs.find(std::string(name));

		if (pFunc == vulkan::func_ptrs.end())
		{
			return nullptr;
		}

		return pFunc->second;
	}

	void * Allocate(const VkAllocationCallbacks * pAlloc, size_t size, size_t align, VkSystemAllocationScope scope)
	{
		return pAlloc->pfnAllocation(pAlloc->pUserData, size, align, scope);
	}

	void * Alloc(const VkAllocationCallbacks * pParent, const VkAllocationCallbacks * pAlloc, size_t size, size_t align, VkSystemAllocationScope scope)
	{
		if (pAlloc)
		{
			return Allocate(pAlloc, size, align, scope);
		}
		else
		{
			return Allocate(pParent, size, align, scope);
		}
	}

	void Free(VkAllocationCallbacks * pAlloc, void * pData)
	{
		if (pData == NULL)
		{
			return;
		}

		pAlloc->pfnFree(pAlloc->pUserData, pData);
	}

}

