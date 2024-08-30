#ifndef SMIRC_VK
#define SMIRC_VK

#include <stdbool.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

bool load_vulkan();
void unload_vulkan();

// TODO can manually specify the functions I use or use a script to generate such code
// declare functions here as alternative to the prototypes in vulkan.h that I omit,

PFN_vkCreateInstance vkCreateInstance;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;



#endif
