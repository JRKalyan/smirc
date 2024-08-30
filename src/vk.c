#include "vk.h"

#include <dlfcn.h>


static void* s_shared_object;

// TODO: volk?
bool load_vulkan() {
    s_shared_object = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (s_shared_object == NULL) {
        return false;
    }

    // Lookup the fundamental functions
    const char* err = dlerror();
    vkCreateInstance = dlsym(s_shared_object, "vkCreateInstance");
    err = dlerror();
    if (err != NULL) {
        // TODO could propagate errmsg
        return false;
    }

    // Create instance
    // TODO ensure that I get missing field initializer warnings, and by practice always
    // use this way of initializing structs.
    const VkInstanceCreateInfo create_info = {
    };
    vkCreateInstance();

    // TODO error check these as well, can make a macro to do this
    vkGetInstanceProcAddr = dlsym(s_shared_object, "vkGetInstanceProcAddr");
    vkEnumerateInstanceExtensionProperties = dlsym(s_shared_object, "vkEnumerateInstanceExtensionProperties");


    return true;
}

void unload_vulkan() {
    dlclose(s_shared_object);
}

VkInstance create_instance() {
    // TODO call load_vulkan, create instance, load instance procedures
}

VkDevice create_device() {
    // TODO create logical device, load device procedures
}
