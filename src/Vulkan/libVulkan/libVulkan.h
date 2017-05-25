#ifndef LIB_VULKAN_H
#define LIB_VULKAN_H


#ifdef PRINT_API_EXPORTS
#define PRINT_API __declspec(dllexport)
#else
#define PRINT_API __declspec(dllimport)
#endif

namespace vulkan
{
	void test_print(void);
}

#endif
