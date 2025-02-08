#include "context.h"

#define TITLE "SMIRC"
#define VERSION "0.0.0"

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "vk.h"

bool context_session_init(ContextSession session) {
    (void)session;

    SDL_SetAppMetadata(TITLE, VERSION, "");

    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    return SDL_Init(init_flags);
}

void context_session_destroy(ContextSession session) {
    (void)session;
    SDL_Quit();
}

bool LINKWindow_init(LINKWindow* link) {
    if (context_session_init(link->session) == false) {
        return false;
    }

    const SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;
    link->window = SDL_CreateWindow(TITLE, 500, 500, window_flags);
    if (link->window == NULL) {
        context_session_destroy(link->session);
        return false;
    }

    return true;
}

void LINKWindow_destroy(LINKWindow* link) {
    SDL_DestroyWindow(link->window);
    context_session_destroy(link->session);
}

bool LINKGetInstanceProcAddr_init(LINKGetInstanceProcAddr* link) {
    if (LINKWindow_init(&link->l_window) == false) {
        return false;
    }

    link->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();

    if (link->vkGetInstanceProcAddr == NULL) {
        LINKWindow_destroy(&link->l_window);
        return false;
    }

    return true;
}

void LINKGetInstanceProcAddr_destroy(LINKGetInstanceProcAddr* link) {
    // No need to destroy proc addr it is managed by SDL
    LINKWindow_destroy(&link->l_window);
}

bool LINKVulkanInstance_init(LINKVulkanInstance* link) {
    if (LINKGetInstanceProcAddr_init(&link->l_get_proc_addr) == false) {
        return false;
    }

    uint32_t extension_count = 0;
    const char* const* required_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);

    #ifdef NDEBUG
    const char* const* extensions = required_extensions;
    uint32_t layer_count = 0;
    const char * layer_names[] = {};
    #else
    #define EXT_MAX 20
    const char* extensions[EXT_MAX];
    if (extension_count >= EXT_MAX) {
        return false;
    }
    uint32_t i = 0;
    for (; i < extension_count; i++) {
        extensions[i] = required_extensions[i];
    }
    extensions[i] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    extension_count += 1;

    uint32_t layer_count = 1;
    const char * layer_names[] = {"VK_LAYER_KHRONOS_validation"};
    #endif

    const VulkanCreateInfo info = {
        .vkGetInstanceProcAddr = link->l_get_proc_addr.vkGetInstanceProcAddr,
        .extension_names = extensions,
        .extension_count = extension_count,
        .layer_names = layer_names,
        .layer_count = layer_count
    };

    if (vulkan_init(&link->vk, &info) == false) {
        LINKGetInstanceProcAddr_destroy(&link->l_get_proc_addr);
        return false;
    }

    return true;
}

void LINKVulkanInstance_destroy(LINKVulkanInstance* link) {
    vulkan_destroy(&link->vk);
    LINKGetInstanceProcAddr_destroy(&link->l_get_proc_addr);
}

bool LINKVulkanSurface_init(LINKVulkanSurface *link) {
    if (LINKVulkanInstance_init(&link->l_vk) == false) {
        return false;
    }

    const bool ret = SDL_Vulkan_CreateSurface(
        link->l_vk.l_get_proc_addr.l_window.window,
        link->l_vk.vk.instance,
        link->l_vk.vk.allocator,
        &link->surface);

    if (ret == false) {
        LINKVulkanInstance_destroy(&link->l_vk);
        return false;
    }

    return true;
};

void LINKVulkanSurface_destroy(LINKVulkanSurface* link) {
    SDL_Vulkan_DestroySurface(
        link->l_vk.vk.instance,
        link->surface,
        link->l_vk.vk.allocator);
    LINKVulkanInstance_destroy(&link->l_vk);
}

bool LINKVulkanPhysicalDeviceList_init(LINKVulkanPhysicalDeviceList* link) {
    if (LINKVulkanSurface_init(&link->l_surface) == false) {
        return false;
    }

    const bool ret = vulkan_physical_device_list_init(
        &link->l_surface.l_vk.vk,
        link->l_surface.surface,
        &link->physical_devices);
    if (ret == false) {
        LINKVulkanSurface_destroy(&link->l_surface);
        return false;
    }

    return true;
}

void LINKVulkanPhysicalDeviceList_destroy(LINKVulkanPhysicalDeviceList* link) {
    vulkan_physical_device_list_destroy(&link->physical_devices);
    LINKVulkanSurface_destroy(&link->l_surface);
}

bool LINKVulkanDevice_init(LINKVulkanDevice* link) {
    if (LINKVulkanPhysicalDeviceList_init(&link->l_physical_devices) == false) {
        return false;
    }

    bool success = vulkan_device_init(
        &link->l_physical_devices.l_surface.l_vk.vk,
        &link->l_physical_devices.physical_devices,
        &link->vd);
    if (success == false)
    {
        LINKVulkanPhysicalDeviceList_destroy(&link->l_physical_devices);
        return false;
    }

    return true;
}

void LINKVulkanDevice_destroy(LINKVulkanDevice* link) {
    vulkan_device_destroy(&link->l_physical_devices.l_surface.l_vk.vk, &link->vd);
    LINKVulkanPhysicalDeviceList_destroy(&link->l_physical_devices);
}

bool LINKVulkanDeviceContext_init(LINKVulkanDeviceContext* link) {
    if (LINKVulkanDevice_init(&link->l_vd) != true) {
        return false;
    }

    const VulkanInstance* vk = &link->l_vd.l_physical_devices.l_surface.l_vk.vk;
    const VulkanDevice* vd = &link->l_vd.vd;
    if (vulkan_device_context_init(vk, vd, &link->device_context) != true) {
        LINKVulkanDevice_destroy(&link->l_vd);
        return false;
    }

    return true;
}

void LINKVulkanDeviceContext_destroy(LINKVulkanDeviceContext* link) {
    vulkan_device_context_destroy();
    LINKVulkanDevice_destroy(&link->l_vd);
}

bool LINKVulkanShaders_init(LINKVulkanShaders* link) {
    if (LINKVulkanDeviceContext_init(&link->l_device_context) == false) {
        return false;
    }

    if (vulkan_shaders_init(&link->l_device_context.device_context, &link->shaders) == false) {
        LINKVulkanDeviceContext_destroy(&link->l_device_context);
        return false;
    }

    return true;
}

void LINKVulkanShaders_destroy(LINKVulkanShaders* link) {
    vulkan_shaders_destroy(&link->l_device_context.device_context, &link->shaders);
    LINKVulkanDeviceContext_destroy(&link->l_device_context);
}

bool LINKVulkanSwapchain_init(LINKVulkanSwapchain* link) {
    if (LINKVulkanShaders_init(&link->l_shaders) == false) {
        return false;
    }

    const VulkanDevice* device = &link->l_shaders.l_device_context.l_vd.vd;
    const VulkanInstance* vk = &link->l_shaders.l_device_context.l_vd.l_physical_devices.l_surface.l_vk.vk;
    const VulkanDeviceContext* context = &link->l_shaders.l_device_context.device_context;
    const VkSurfaceKHR surface = link->l_shaders.l_device_context.l_vd.l_physical_devices.l_surface.surface;
    const bool result = vulkan_swapchain_init(
        vk,
        device,
        context,
        surface,
        &link->vk_swapchain);

    if (result == false) {
        LINKVulkanShaders_destroy(&link->l_shaders);
        return false;
    }

    return true;
}

void LINKVulkanSwapchain_destroy(LINKVulkanSwapchain* link) {
    const VulkanDevice* device = &link->l_shaders.l_device_context.l_vd.vd;
    const VulkanInstance* vk = &link->l_shaders.l_device_context.l_vd.l_physical_devices.l_surface.l_vk.vk;
    const VulkanDeviceContext* context = &link->l_shaders.l_device_context.device_context;
    vulkan_swapchain_destroy(
        vk,
        device,
        context,
        &link->vk_swapchain);

    LINKVulkanShaders_destroy(&link->l_shaders);
}

bool LINKVulkanSwapchainImages_init(LINKVulkanSwapchainImages* link) {

    if (LINKVulkanSwapchain_init(&link->l_swapchain) == false) {
        return false;
    }

    const bool result = vulkan_swapchain_images_init(
        &link->l_swapchain.vk_swapchain,
        &link->swapchain_images);

    if (result == false) {
        LINKVulkanSwapchain_destroy(&link->l_swapchain);
        return false;
    }

    return true;
}

void LINKVulkanSwapchainImages_destroy(LINKVulkanSwapchainImages* link) {
    vulkan_swapchain_images_destroy(&link->swapchain_images);
    LINKVulkanSwapchain_destroy(&link->l_swapchain);
}


bool context_init(Context* context) {
    return LINKVulkanSwapchainImages_init(context);
}

void context_destroy(Context* context) {
    LINKVulkanSwapchainImages_destroy(context);
}

void context_sleep(Context* context, uint32_t ms) {
    (void)context;
    SDL_Delay(ms);
}
