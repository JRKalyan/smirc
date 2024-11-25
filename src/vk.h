#ifndef SMIRC_VK
#define SMIRC_VK

#include <stdbool.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

bool load_vulkan();
void unload_vulkan();

extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;



#endif
