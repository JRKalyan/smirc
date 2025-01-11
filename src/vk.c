#include "vk.h"

#include <dlfcn.h>


static void* s_shared_object;

PFN_vkCreateInstance vkCreateInstance;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;

// TODO: volk?
bool load_vulkan() {
    s_shared_object = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (s_shared_object == NULL) {
        return false;
    }

    // TODO should be loading vkGetInstanceProcAddr and using that, no need for
    // dlsym on the other ones.
    const char* err = dlerror();
    vkGetInstanceProcAddr = dlsym(s_shared_object, "vkGetInstanceProcAddr");
    err = dlerror();
    if (err != NULL) {
        // TODO could propagate errmsg
        return false;
    }

    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");

    // Create instance
    // TODO ensure that I get missing field initializer warnings, and by practice always
    // use this way of initializing structs.
    const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "smirc",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "none",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pNext = NULL,
    };

    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 0, // TODO
        .ppEnabledExtensionNames = NULL,
    };

    VkInstance instance;
    vkCreateInstance(&create_info, NULL, &instance);

    return true;
}

void unload_vulkan() {
    dlclose(s_shared_object);
}

VkInstance create_instance() {
    // TODO call load_vulkan, create instance, load instance procedures
    return NULL;
}

VkDevice create_device() {
    // TODO create logical device, load device procedures
    return NULL;
}
