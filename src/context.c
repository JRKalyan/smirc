#include "context.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#define TITLE "SMIRC"
#define VERSION "0.0.0"

#include <stdlib.h>

// REMOVE
#include <time.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "vk.h"

typedef enum LINKType {
    LINK_NONE,
    LINK_SWAPCHAIN,
} LINKType;

// TODO, this only exists temporarily for visualizing
// what should be generated code based on dependency
// specification.
// LINKSwapchain functions below show an example of what it can be
// replaced with when needing to recreate a node in the
// dependency graph (chain for now). I don't want to specify these
// manually when recreation of some of these nodes is unnecessary
// for now, so this is a standin that will be false.
#define STOP_HERE(stop) (stop != stop)


bool context_session_init(ContextSession session) {
    (void)session;

    SDL_SetAppMetadata(TITLE, VERSION, "");

    if (SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1") == false) {
        return false;
    }

    const SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    return SDL_Init(init_flags);
}

void context_session_destroy(ContextSession session) {
    (void)session;
    SDL_Quit();
}

bool LINKWindow_init(LINKWindow* link, LINKType stop) {
    if (STOP_HERE(stop) == false && context_session_init(link->session) == false) {
        return false;
    }

    const SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    link->window = SDL_CreateWindow(TITLE, 500, 500, window_flags);
    if (link->window == NULL) {
        if (STOP_HERE(stop) == false) {
            context_session_destroy(link->session);
        }
        return false;
    }

    return true;
}

void LINKWindow_destroy(LINKWindow* link, LINKType stop) {
    SDL_DestroyWindow(link->window);
    if (STOP_HERE(stop) == false) {
        context_session_destroy(link->session);
    }
}

bool LINKGetInstanceProcAddr_init(LINKGetInstanceProcAddr* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKWindow_init(&link->l_window, stop) == false) {
        return false;
    }

    link->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();

    if (link->vkGetInstanceProcAddr == NULL) {
        if (STOP_HERE(stop) == false) {
            LINKWindow_destroy(&link->l_window, stop);
        }
        return false;
    }

    return true;
}

void LINKGetInstanceProcAddr_destroy(LINKGetInstanceProcAddr* link, LINKType stop) {
    // No need to destroy proc addr it is managed by SDL
    if (STOP_HERE(stop) == false) {
        LINKWindow_destroy(&link->l_window, stop);
    }
}

bool LINKVulkanInstance_init(LINKVulkanInstance* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKGetInstanceProcAddr_init(&link->l_get_proc_addr, stop) == false) {
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
        if (STOP_HERE(stop) == false) {
            LINKGetInstanceProcAddr_destroy(&link->l_get_proc_addr, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanInstance_destroy(LINKVulkanInstance* link, LINKType stop) {
    vulkan_destroy(&link->vk);
    if (STOP_HERE(stop) == false) {
        LINKGetInstanceProcAddr_destroy(&link->l_get_proc_addr, stop);
    }
}

bool LINKVulkanSurface_init(LINKVulkanSurface *link, LINKType stop) {
    if (LINKVulkanInstance_init(&link->l_vk, stop) == false) {
        return false;
    }

    const bool ret = SDL_Vulkan_CreateSurface(
        link->l_vk.l_get_proc_addr.l_window.window,
        link->l_vk.vk.instance,
        link->l_vk.vk.allocator,
        &link->surface);

    if (ret == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanInstance_destroy(&link->l_vk, stop);
        }
        return false;
    }

    return true;
};

void LINKVulkanSurface_destroy(LINKVulkanSurface* link, LINKType stop) {
    SDL_Vulkan_DestroySurface(
        link->l_vk.vk.instance,
        link->surface,
        link->l_vk.vk.allocator);
    if (STOP_HERE(stop) == false) {
        LINKVulkanInstance_destroy(&link->l_vk, stop);
    }
}

bool LINKVulkanPhysicalDeviceList_init(LINKVulkanPhysicalDeviceList* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanSurface_init(&link->l_surface, stop) == false) {
        return false;
    }

    const bool ret = vulkan_physical_device_list_init(
        &link->l_surface.l_vk.vk,
        link->l_surface.surface,
        &link->physical_devices);
    if (ret == false) {
        LINKVulkanSurface_destroy(&link->l_surface, stop);
        return false;
    }

    return true;
}

void LINKVulkanPhysicalDeviceList_destroy(LINKVulkanPhysicalDeviceList* link, LINKType stop) {
    vulkan_physical_device_list_destroy(&link->physical_devices);
    if (STOP_HERE(stop) == false) {
        LINKVulkanSurface_destroy(&link->l_surface, stop);
    }
}

bool LINKVulkanDevice_init(LINKVulkanDevice* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanPhysicalDeviceList_init(&link->l_physical_devices, stop) == false) {
        return false;
    }

    bool success = vulkan_device_init(
        &link->l_physical_devices.l_surface.l_vk.vk,
        &link->l_physical_devices.physical_devices,
        &link->vd);
    if (success == false)
    {
        if (STOP_HERE(stop) == false) {
            LINKVulkanPhysicalDeviceList_destroy(&link->l_physical_devices, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanDevice_destroy(LINKVulkanDevice* link, LINKType stop) {
    vulkan_device_destroy(&link->l_physical_devices.l_surface.l_vk.vk, &link->vd);
    if (STOP_HERE(stop) == false) {
        LINKVulkanPhysicalDeviceList_destroy(&link->l_physical_devices, stop);
    }
}

bool LINKVulkanDeviceContext_init(LINKVulkanDeviceContext* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanDevice_init(&link->l_vd, stop) != true) {
        return false;
    }

    const VulkanInstance* vk = &link->l_vd.l_physical_devices.l_surface.l_vk.vk;
    const VulkanDevice* vd = &link->l_vd.vd;
    if (vulkan_device_context_init(vk, vd, &link->device_context) != true) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanDevice_destroy(&link->l_vd, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanDeviceContext_destroy(LINKVulkanDeviceContext* link, LINKType stop) {
    vulkan_device_context_destroy();
    if (STOP_HERE(stop) == false) {
        LINKVulkanDevice_destroy(&link->l_vd, stop);
    }
}

bool LINKVulkanShaders_init(LINKVulkanShaders* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanDeviceContext_init(&link->l_device_context, stop) == false) {
        return false;
    }

    if (vulkan_shaders_init(&link->l_device_context.device_context, &link->shaders) == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanDeviceContext_destroy(&link->l_device_context, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanShaders_destroy(LINKVulkanShaders* link, LINKType stop) {
    vulkan_shaders_destroy(&link->l_device_context.device_context, &link->shaders);
    if (STOP_HERE(stop) == false) {
        LINKVulkanDeviceContext_destroy(&link->l_device_context, stop);
    }
}

bool LINKVulkanSwapchain_init(LINKVulkanSwapchain* link, LINKType stop) {
    if ((stop != LINK_SWAPCHAIN) && LINKVulkanShaders_init(&link->l_shaders, stop) == false) {
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
        if ((stop != LINK_SWAPCHAIN) == false) {
            LINKVulkanShaders_destroy(&link->l_shaders, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanSwapchain_destroy(LINKVulkanSwapchain* link, LINKType stop) {
    const VulkanDevice* device = &link->l_shaders.l_device_context.l_vd.vd;
    const VulkanInstance* vk = &link->l_shaders.l_device_context.l_vd.l_physical_devices.l_surface.l_vk.vk;
    const VulkanDeviceContext* context = &link->l_shaders.l_device_context.device_context;
    vulkan_swapchain_destroy(
        vk,
        device,
        context,
        &link->vk_swapchain);

    if (stop != LINK_SWAPCHAIN) {
        LINKVulkanShaders_destroy(&link->l_shaders, stop);
    }
}

bool LINKVulkanSwapchainImages_init(LINKVulkanSwapchainImages* link, LINKType stop) {

    if (STOP_HERE(stop) == false && LINKVulkanSwapchain_init(&link->l_swapchain, stop) == false) {
        return false;
    }

    const bool result = vulkan_swapchain_images_init(
        &link->l_swapchain.vk_swapchain,
        &link->swapchain_images);

    if (result == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanSwapchain_destroy(&link->l_swapchain, stop);
        }
        return false;
    }

    return true;
}

void LINKVulkanSwapchainImages_destroy(LINKVulkanSwapchainImages* link, LINKType stop) {
    vulkan_swapchain_images_destroy(&link->swapchain_images);
    if (STOP_HERE(stop) == false) {
        LINKVulkanSwapchain_destroy(&link->l_swapchain, stop);
    }
}

bool LINKVulkanGraphicsPipeline_init(LINKVulkanGraphicsPipeline* link, LINKType stop) {

    
    if (STOP_HERE(stop) == false && LINKVulkanSwapchainImages_init(&link->l_swapchain_images, stop) == false) {
        return false;
    }

    const LINKVulkanShaders* l_shaders = &link->l_swapchain_images
        .l_swapchain
        .l_shaders;
    const VulkanDeviceContext* context = &l_shaders->l_device_context
        .device_context;
    const bool result = vulkan_graphics_pipeline_init(
        context,
        &l_shaders->shaders,
        link->l_swapchain_images.swapchain_images.d_swapchain,
        &link->pipeline);

    if (result == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanSwapchainImages_destroy(&link->l_swapchain_images, stop);
        }
    }

    return result;
}

void LINKVulkanGraphicsPipeline_destroy(LINKVulkanGraphicsPipeline* link, LINKType stop) {
    vulkan_graphics_pipeline_destroy(&link->pipeline);
    if (STOP_HERE(stop) == false) {
        LINKVulkanSwapchainImages_destroy(&link->l_swapchain_images, stop);
    }
}

bool LINKVulkanSyncObjects_init(LINKVulkanSyncObjects* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanGraphicsPipeline_init(&link->l_graphics_pipeline, stop) == false) {
        return false;
    }

    const bool result = vulkan_sync_objects_init(
        link->l_graphics_pipeline.pipeline.d_context,
        &link->sync_objects);

    if (result == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanGraphicsPipeline_destroy(&link->l_graphics_pipeline, stop);
        }
    }
    return result;
}

void LINKVulkanSyncObjects_destroy(LINKVulkanSyncObjects* link, LINKType stop) {
    vulkan_sync_objects_destroy(
        link->l_graphics_pipeline.pipeline.d_context,
        &link->sync_objects);
    if (STOP_HERE(stop) == false) {
        LINKVulkanGraphicsPipeline_destroy(&link->l_graphics_pipeline, stop);
    }
}

bool LINKVulkanCommands_init(LINKVulkanCommands* link, LINKType stop) {
    if (STOP_HERE(stop) == false && LINKVulkanSyncObjects_init(&link->l_sync_objects, stop) == false) {
        return false;
    }

    const bool result = vulkan_commands_init(
        link->l_sync_objects.l_graphics_pipeline.pipeline.d_context,
        &link->commands);

    if (result == false) {
        if (STOP_HERE(stop) == false) {
            LINKVulkanSyncObjects_destroy(&link->l_sync_objects, stop);
        }
    }

    return result;
}

void LINKVulkanCommands_destroy(LINKVulkanCommands* link, LINKType stop) {
    vulkan_commands_destroy(
        link->l_sync_objects.l_graphics_pipeline.pipeline.d_context,
        &link->commands);
    if (STOP_HERE(stop) == false) {
        LINKVulkanSyncObjects_destroy(&link->l_sync_objects, stop);
    }
}

bool context_init(Context* context) {
    return LINKVulkanCommands_init(context, LINK_NONE);
}

void context_destroy(Context* context) {
    LINKVulkanCommands_destroy(context, LINK_NONE);
}

void context_sleep(Context* context, uint32_t ms) {
    (void)context;
    SDL_Delay(ms);
}

bool context_swapchain_recreate(Context* context) {
    LINKVulkanCommands_destroy(context, LINK_SWAPCHAIN);
    return LINKVulkanCommands_init(context, LINK_SWAPCHAIN);
}

bool context_draw(Context* context) {
    const VulkanDeviceContext* device_context = context->l_sync_objects
        .l_graphics_pipeline
        .pipeline.d_context;
    const VulkanGraphicsPipeline* graphics_pipeline = &context->l_sync_objects
        .l_graphics_pipeline
        .pipeline;
    const LINKVulkanSwapchainImages* l_swapchain_images = &context->l_sync_objects
        .l_graphics_pipeline
        .l_swapchain_images;
    const VulkanSwapchainImages* swapchain_images = &l_swapchain_images->swapchain_images;
    const VulkanSwapchain* swapchain = &l_swapchain_images->l_swapchain.vk_swapchain;
    const VulkanCommands* commands = &context->commands;
    const VulkanSyncObjects* sync_objects = &context->l_sync_objects
        .sync_objects;

    // TODO clean all this shit up, recreate swapchain when I should, and handle using
    // a sub optimal swapchain.
    uint32_t i = 0;
    clock_t start = clock(); // TODO use RTC, as I'm trying to measure GPU perf
    #define NUM_FRAMES_TIMED 20
    while (i < NUM_FRAMES_TIMED) {
        // polling events... not sure if needed to change window size.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
        }
        context_swapchain_recreate(context);
        const bool result = vulkan_draw_mutate(
            device_context,
            swapchain,
            graphics_pipeline,
            commands,
            sync_objects,
            swapchain_images);
        if (result == false) {
            printf("FAILED!!!!!!!!\n");
            device_context->vkDeviceWaitIdle(device_context->d_device->logical_device);
            return false;
        }
        i++;
    }
    clock_t elapsed = clock() - start;
    double timer = (double)elapsed / CLOCKS_PER_SEC;
    printf("FRAME COUNT: %d frames\n", NUM_FRAMES_TIMED);
    printf("TOTAL TIME:  %.3f sec\n", timer);
    printf("FRAME RATE:  %.3f fps\n", NUM_FRAMES_TIMED / timer);
    printf("FRAME TIME:  %.3f ms\n", (timer * 1000) / NUM_FRAMES_TIMED);

    device_context->vkDeviceWaitIdle(device_context->d_device->logical_device);

    return true;
}
