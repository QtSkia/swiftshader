#ifndef UTILS_H
#define UTILS_H

#include <unordered_map>
#include <string>
#include <Vulkan\vulkan.h>

namespace vulkan
{
#define ALIGNMENT 8

#define DEFINE_HANDLE_CASTS(__type, __VkType)                              \
                                                                           \
   static inline struct __type *                                           \
   __type ## _from_handle(__VkType _handle)                                \
   {                                                                       \
      return (struct __type *) _handle;                                    \
   }                                                                       \
                                                                           \
   static inline __VkType                                                  \
   __type ## _to_handle(struct __type *_obj)                               \
   {                                                                       \
      return (__VkType) _obj;                                              \
   }

#define DEFINE_NONDISP_HANDLE_CASTS(__type, __VkType)                      \
                                                                           \
   static inline struct __type *                                           \
   __type ## _from_handle(__VkType _handle)                                \
   {                                                                       \
      return (struct __type *)(uintptr_t) _handle;                         \
   }                                                                       \
                                                                           \
   static inline __VkType                                                  \
   __type ## _to_handle(struct __type *_obj)                               \
   {                                                                       \
      return (__VkType)(uintptr_t) _obj;                                   \
   }

#define GET_FROM_HANDLE(__type, __name, __handle) \
   struct __type *__name = __type ## _from_handle(__handle)


	DEFINE_HANDLE_CASTS(Instance, VkInstance)
	DEFINE_HANDLE_CASTS(PhysicalDevice, VkPhysicalDevice)
	DEFINE_HANDLE_CASTS(Device, VkDevice)

	DEFINE_NONDISP_HANDLE_CASTS(Sampler, VkSampler)
}


namespace vkutils
{
	PFN_vkVoidFunction findEntry(const char *name);
	void *Allocate(const VkAllocationCallbacks *pAlloc, size_t size, size_t align, VkSystemAllocationScope scope);
	void *Alloc(const VkAllocationCallbacks *pParent, const VkAllocationCallbacks *pAlloc, size_t size, size_t align, VkSystemAllocationScope scope);
	void Free(VkAllocationCallbacks *pAlloc, void *pData);
}


#endif
