#include "vk.h"

#include <stdio.h> // TODO REMOVE
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <vulkan/vulkan_core.h>

#include "shader.h"


#define GET_INSTANCE_PROC_ADDR(name) \
    (PFN_##name)vk_info->vkGetInstanceProcAddr(vk->instance, #name)

#define SET_INSTANCE_PROC_ADDR(name) { \
    vk->name = GET_INSTANCE_PROC_ADDR(name);    \
    if (vk->name == NULL) { vk->vkDestroyInstance(vk->instance, vk->allocator); return false; }}

#define SET_DEVICE_PROC_ADDR(name) {     \
        device_context->name = (PFN_##name)vk->vkGetDeviceProcAddr(device->logical_device, #name); \
        if (device_context->name == NULL) { return false; } \
    }

bool vulkan_init(VulkanInstance* vk, const VulkanCreateInfo* vk_info) {

    vk->instance = NULL;
    vk->allocator = NULL;
    vk->vkCreateInstance = GET_INSTANCE_PROC_ADDR(vkCreateInstance);
    if (vk->vkCreateInstance == NULL) {
        return false;
    }

    // Create instance
    const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "smirc",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "none",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
        .pNext = NULL,
    };

    // TODO use -Wmissing-designated-field-initializers when it exists,
    // or linting of some other kind (clang-tidy)

    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = vk_info->layer_count,
        .ppEnabledLayerNames = vk_info->layer_names,
        .enabledExtensionCount = vk_info->extension_count,
        .ppEnabledExtensionNames = vk_info->extension_names,
    };

    // Create instance
    if (vk->vkCreateInstance(&create_info, vk->allocator, &vk->instance) != VK_SUCCESS) {
        return false;
    }

    // Load instance functions
    SET_INSTANCE_PROC_ADDR(vkDestroyInstance);
    SET_INSTANCE_PROC_ADDR(vkEnumeratePhysicalDevices);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceProperties);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceFeatures);
    SET_INSTANCE_PROC_ADDR(vkCreateDevice);
    SET_INSTANCE_PROC_ADDR(vkDestroyDevice);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceQueueFamilyProperties);
    SET_INSTANCE_PROC_ADDR(vkEnumerateDeviceExtensionProperties);
    SET_INSTANCE_PROC_ADDR(vkGetDeviceProcAddr);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceSupportKHR);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceFormatsKHR);
    SET_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfacePresentModesKHR);

    return true;
}

void vulkan_destroy(const VulkanInstance* vk) {
    vk->vkDestroyInstance(vk->instance, vk->allocator);
}

bool vulkan_shaders_init_from_fragment_spirv(
    const VulkanDeviceContext* context,
    const VkShaderModule vertex_module,
    ShaderSPIRV* spirv,
    VulkanShaders* shaders) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->size,
        .pCode = spirv->words,
        .flags = 0,
    };
    VkShaderModule fragment_module;
    const VkResult create_result = context->vkCreateShaderModule(
        context->d_device->logical_device,
        &create_info,
        context->d_device->d_vk->allocator,
        &fragment_module);

    if (create_result != VK_SUCCESS) {
        return false;
    }

    shaders->d_context = context;
    shaders->vertex_shader = vertex_module;
    shaders->fragment_shader = fragment_module;

    return true;
}
bool vulkan_shaders_init_from_vertex_module(
    const VulkanDeviceContext* context,
    const ShaderCompiler* compiler,
    const VkShaderModule vertex_module,
    VulkanShaders* shaders) {

    ShaderSPIRV spirv;
    const bool spirv_result = shader_spirv_init(
        "shaders/triangle.frag",
        SHADER_FRAGMENT,
        compiler,
        &spirv);

    if (spirv_result == false) {
        return false;
    }

    const bool init_result = vulkan_shaders_init_from_fragment_spirv(
        context,
        vertex_module,
        &spirv,
        shaders);

    shader_spirv_destroy(&spirv);

    return init_result;
}
bool vulkan_shaders_init_from_vertex_spirv(
    const VulkanDeviceContext* context,
    const ShaderCompiler* compiler,
    ShaderSPIRV* spirv,
    VulkanShaders* shaders) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->size,
        .pCode = spirv->words,
        .flags = 0,
    };
    VkShaderModule vertex_module;
    const VkResult create_result = context->vkCreateShaderModule(
        context->d_device->logical_device,
        &create_info,
        context->d_device->d_vk->allocator,
        &vertex_module);

    if (create_result != VK_SUCCESS) {
        return false;
    }

    const bool success = vulkan_shaders_init_from_vertex_module(
        context,
        compiler,
        vertex_module,
        shaders);

    if (success == false) {
        context->vkDestroyShaderModule(
            context->d_device->logical_device,
            vertex_module,
            context->d_device->d_vk->allocator);
        return false;
    }

    return true;
}
bool vulkan_shaders_init_from_compiler(
    const VulkanDeviceContext* context,
    ShaderCompiler* compiler,
    VulkanShaders* shaders) {

    ShaderSPIRV spirv;
    const bool spirv_result = shader_spirv_init(
        "shaders/triangle.vert",
        SHADER_VERTEX,
        compiler,
        &spirv);

    if (spirv_result == false) {
        return false;
    }

    const bool init_result = vulkan_shaders_init_from_vertex_spirv(
        context,
        compiler,
        &spirv,
        shaders);

    shader_spirv_destroy(&spirv);

    return init_result;
}

bool vulkan_shaders_init(const VulkanDeviceContext* context, VulkanShaders* shaders) {
    ShaderCompiler compiler;
    if (shader_compiler_init(&compiler) == false) {
        return false;
    }

    const bool result = vulkan_shaders_init_from_compiler(context, &compiler, shaders);

    shader_compiler_destroy(&compiler);

    return result;
}

void vulkan_shaders_destroy(const VulkanDeviceContext* context, VulkanShaders* shaders) {
    context->vkDestroyShaderModule(
        context->d_device->logical_device,
        shaders->vertex_shader,
        context->d_device->d_vk->allocator);
    context->vkDestroyShaderModule(
        context->d_device->logical_device,
        shaders->fragment_shader,
        context->d_device->d_vk->allocator);
}

void vulkan_device_surface_compat_destroy(VulkanDeviceSurfaceCompat* compat) {
    if (compat->surface_formats != NULL) {
        free(compat->surface_formats);
    }

    if (compat->present_modes) {
        free(compat->present_modes);
    }
}

bool vulkan_device_surface_compat_init(
    const VulkanInstance* vk,
    const VkPhysicalDevice physical_device,
    const VkSurfaceKHR surface,
    VulkanDeviceSurfaceCompat* compat) {

    VkResult result = vk->vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device,
        surface, 
        &compat->surface_format_count,
        NULL);

    if (result != VK_SUCCESS) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    compat->surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * compat->surface_format_count);
    if (compat->surface_formats == NULL) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    result = vk->vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device,
        surface, 
        &compat->surface_format_count,
        compat->surface_formats);
    if (result != VK_SUCCESS) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    // Present modes
    result = vk->vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device,
        surface, 
        &compat->present_mode_count,
        NULL);

    if (result != VK_SUCCESS) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    compat->present_modes = malloc(sizeof(VkPresentModeKHR) * compat->present_mode_count);
    if (compat->present_modes == NULL) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    result = vk->vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device,
        surface, 
        &compat->present_mode_count,
        compat->present_modes);

    if (result != VK_SUCCESS) {
        vulkan_device_surface_compat_destroy(compat);
        return false;
    }

    return true;
}


void vulkan_device_description_destroy(VulkanDeviceDescription* desc) {
    if (desc->extension_properties != NULL) {
        free(desc->extension_properties);
    }

    if (desc->queue_family_properties != NULL) {
        free(desc->queue_family_properties);
    }

    if (desc->surface_compat) {
        vulkan_device_surface_compat_destroy(&desc->maybe_compat);
    }
}

bool vulkan_device_description_init(
    const VulkanInstance* vk,
    const VkSurfaceKHR maybe_surface,
    const VkPhysicalDevice physical_device,
    VulkanDeviceDescription* desc)
{
    desc->extension_properties = NULL;
    desc->queue_family_properties = NULL;
    desc->surface_compat = false;

    desc->d_physical_device = physical_device;

    vk->vkGetPhysicalDeviceProperties(physical_device, &desc->device_properties);
    vk->vkGetPhysicalDeviceFeatures(physical_device, &desc->device_features);

    // Queue Family Properties
    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device,
        &desc->family_count,
        NULL);

    desc->queue_family_properties = malloc(sizeof(VkQueueFamilyProperties) * desc->family_count);
    if (desc->queue_family_properties == NULL) {
        vulkan_device_description_destroy(desc);
        return false;
    }

    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device,
        &desc->family_count,
        desc->queue_family_properties);

    // Extension Properties
    VkResult result = vk->vkEnumerateDeviceExtensionProperties(
        physical_device,
        NULL,
        &desc->extension_count,
        NULL);
    if (result != VK_SUCCESS) {
        vulkan_device_description_destroy(desc);
        return false;
    }

    desc->extension_properties = malloc(sizeof(VkExtensionProperties) * desc->extension_count);
    if (desc->extension_properties == NULL) {
        vulkan_device_description_destroy(desc);
        return false;
    }

    result = vk->vkEnumerateDeviceExtensionProperties(
        physical_device,
        NULL,
        &desc->extension_count,
        desc->extension_properties);

    if (result != VK_SUCCESS) {
        vulkan_device_description_destroy(desc);
        return false;
    }

    // Check for surface support first, setting surface_compat
    // Also caching first presentation capable queue for now,
    // eventually this should be exposed in API to higher level
    // language
    bool swapchain_ext_found = false;
    for (uint32_t k = 0; k < desc->extension_count; k++) {
        VkExtensionProperties* properties = desc->extension_properties + k;
        if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, properties->extensionName) == 0) {
            swapchain_ext_found = true;
            break;
        }
    }

    if (swapchain_ext_found == false) {
        // No extension means no surface compat.
        return true;
    }

    if (maybe_surface == VK_NULL_HANDLE) {
        return true;
    }

    // Optional Surface compatability information
    for (uint32_t i = 0; i < desc->family_count; i++) {
        VkBool32 supported = VK_FALSE;
        vk->vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device,
            i,
            maybe_surface,
            &supported);
        if (supported == VK_TRUE) {
            desc->present_family_index = i;
            desc->surface_compat = true;
            break;
        }
    }

    if (desc->surface_compat) {
        const bool result = vulkan_device_surface_compat_init(
            vk,
            physical_device,
            maybe_surface,
            &desc->maybe_compat);
        if (result == false) {
            desc->surface_compat = false; // Setting false for dtor, ugliness
            vulkan_device_description_destroy(desc);
            return false;
        }
    }

    return true;
}

bool physical_device_list_init_from_list(
    const VulkanInstance* vk,
    VkSurfaceKHR maybe_surface,
    VulkanPhysicalDeviceList* list, // uninitialized
    VkPhysicalDevice* physical_devices, // must be count length, uninitialized
    uint32_t count)
{
    if (vk->vkEnumeratePhysicalDevices(vk->instance, &count, physical_devices) != VK_SUCCESS) {
        return false;
    }

    VulkanDeviceDescription* descriptions = malloc(sizeof(VulkanDeviceDescription) * count);
    if (descriptions == NULL) {
        return false;
    }

    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];
        VulkanDeviceDescription* desc = descriptions + i;
        if (vulkan_device_description_init(vk, maybe_surface, physical_device, desc) == false) {
            list->count = i;
            list->physical_device_descriptions = descriptions;
            vulkan_physical_device_list_destroy(list);
            return false;
        }
    }

    list->count = count;
    list->physical_device_descriptions = descriptions;

    return true;
}

bool vulkan_physical_device_list_init(
    const VulkanInstance* vk,
    VkSurfaceKHR maybe_surface,
    VulkanPhysicalDeviceList* list) {

    uint32_t count = 0;
    if (vk->vkEnumeratePhysicalDevices(vk->instance, &count, NULL) != VK_SUCCESS) {
        return false;
    }

    if (count == 0) {
        return false;
    }

    VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * count);
    if (physical_devices == NULL) {
        return false;
    }
    else {
        bool success = physical_device_list_init_from_list(
            vk,
            maybe_surface,
            list,
            physical_devices,
            count);
        free(physical_devices);
        return success;
    }
}

void vulkan_physical_device_list_destroy(VulkanPhysicalDeviceList *list) {
    for (uint32_t i = 0; i < list->count; i++) {
        vulkan_device_description_destroy(list->physical_device_descriptions + i);
    }
    free(list->physical_device_descriptions);
}

bool vulkan_device_init(
    const VulkanInstance* vk,
    const VulkanPhysicalDeviceList* list,
    VulkanDevice* device)
{
    // Physical device selection. NOTE: Eventually, this should be exposed
    // to the extendable portion of SMIRC programs through the context API,
    // but for now we will select a device directly in C.
    device->d_physical_device = NULL;
    device->d_vk = vk;

    if (list->count == 0) {
        return false;
    }

    // Required device extensions
    #define REQ_EXT_SIZE 1
    const char* required_extensions[REQ_EXT_SIZE] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Iterate physical device descriptions
    for (uint32_t i = 0; i < list->count; i++) {
        const VulkanDeviceDescription* desc = list->physical_device_descriptions + i;

        // Check for supported extensions:
        bool all_extensions_found = true;
        for (uint32_t j = 0; j < REQ_EXT_SIZE; j++) {
            bool found = false;
            for (uint32_t k = 0; k < desc->extension_count; k++) {
                VkExtensionProperties* properties = desc->extension_properties + k;
                if (strcmp(required_extensions[j], properties->extensionName) == 0) {
                    found = true;
                    break;
                }
            }

            if (found == false) {
                all_extensions_found = false;
                break;
            }
        }

        if (all_extensions_found == false) {
            continue;
        }

        // Check for swap chain support support
        if (desc->surface_compat == false) {
            continue;
        }

        if (desc->maybe_compat.present_mode_count == 0) {
            continue;
        }

        if (desc->maybe_compat.surface_format_count == 0) {
            continue;
        }

        // Tentatively select present family based on cached
        // queue family that supports present mode
        device->present_family_index = desc->present_family_index;

        // Check for supported graphics queue
        for (uint32_t j = 0; j < desc->family_count; j++) {
            const VkQueueFamilyProperties* properties = desc->queue_family_properties + j; 
            if ((properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                // Select this device
                device->graphics_family_index = j;
                device->d_physical_device = desc->d_physical_device;
                device->d_device_description = desc;
                break;
            }
        }
    }

    if (device->d_physical_device == NULL) {
        return false;
    }


    printf("DEVICE: %s\n", device->d_device_description->device_properties.deviceName);
    // Create logical device

    // Queue Creation
    // Note: This also should be a forwarded to the higher level
    // API eventually.
    float queue_priority = 1.0f;

    uint32_t queue_create_info_count = 2;
    if (device->graphics_family_index == device->present_family_index) {
        queue_create_info_count = 1;
    }
    
    const VkDeviceQueueCreateInfo queue_create_infos[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .pQueuePriorities = &queue_priority,
            .queueCount = 1,
            .queueFamilyIndex = device->graphics_family_index,
            .flags = 0,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .pQueuePriorities = &queue_priority,
            .queueCount = 1,
            .queueFamilyIndex = device->present_family_index,
            .flags = 0,
        },
    };

    const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .enabledExtensionCount = REQ_EXT_SIZE,
        .ppEnabledExtensionNames = required_extensions,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_infos,
        .flags = 0,
        .pEnabledFeatures = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
    };

    const VkResult r = vk->vkCreateDevice(
        device->d_physical_device,
        &device_create_info,
        vk->allocator,
        &device->logical_device);

    return r == VK_SUCCESS;
}

void vulkan_device_destroy(const VulkanInstance* instance, const VulkanDevice* device) {
    instance->vkDestroyDevice(device->logical_device, instance->allocator);
}

bool vulkan_device_context_init(
    const VulkanInstance * vk,
    const VulkanDevice* device,
    VulkanDeviceContext* device_context) {
    device_context->d_device = device;
    SET_DEVICE_PROC_ADDR(vkGetDeviceQueue);
    SET_DEVICE_PROC_ADDR(vkCreateSwapchainKHR);
    SET_DEVICE_PROC_ADDR(vkDestroySwapchainKHR);
    SET_DEVICE_PROC_ADDR(vkGetSwapchainImagesKHR);
    SET_DEVICE_PROC_ADDR(vkCreateImageView);
    SET_DEVICE_PROC_ADDR(vkDestroyImageView);
    SET_DEVICE_PROC_ADDR(vkCreateShaderModule);
    SET_DEVICE_PROC_ADDR(vkDestroyShaderModule);
    SET_DEVICE_PROC_ADDR(vkCreateRenderPass);
    SET_DEVICE_PROC_ADDR(vkDestroyRenderPass);
    SET_DEVICE_PROC_ADDR(vkCreatePipelineLayout);
    SET_DEVICE_PROC_ADDR(vkDestroyPipelineLayout);
    SET_DEVICE_PROC_ADDR(vkCreateGraphicsPipelines);
    SET_DEVICE_PROC_ADDR(vkDestroyPipeline);
    SET_DEVICE_PROC_ADDR(vkCreateFramebuffer);
    SET_DEVICE_PROC_ADDR(vkDestroyFramebuffer);
    SET_DEVICE_PROC_ADDR(vkCreateCommandPool);
    SET_DEVICE_PROC_ADDR(vkDestroyCommandPool);
    SET_DEVICE_PROC_ADDR(vkAllocateCommandBuffers);
    SET_DEVICE_PROC_ADDR(vkResetCommandBuffer);
    SET_DEVICE_PROC_ADDR(vkBeginCommandBuffer);
    SET_DEVICE_PROC_ADDR(vkEndCommandBuffer);
    SET_DEVICE_PROC_ADDR(vkCmdBeginRenderPass);
    SET_DEVICE_PROC_ADDR(vkCmdEndRenderPass);
    SET_DEVICE_PROC_ADDR(vkCmdBindPipeline);
    SET_DEVICE_PROC_ADDR(vkCmdSetViewport);
    SET_DEVICE_PROC_ADDR(vkCmdSetScissor);
    SET_DEVICE_PROC_ADDR(vkCmdDraw);
    SET_DEVICE_PROC_ADDR(vkQueueSubmit);
    SET_DEVICE_PROC_ADDR(vkAcquireNextImageKHR);
    SET_DEVICE_PROC_ADDR(vkQueuePresentKHR);
    SET_DEVICE_PROC_ADDR(vkWaitForFences);
    SET_DEVICE_PROC_ADDR(vkCreateSemaphore);
    SET_DEVICE_PROC_ADDR(vkDestroySemaphore);
    SET_DEVICE_PROC_ADDR(vkCreateFence);
    SET_DEVICE_PROC_ADDR(vkDestroyFence);
    SET_DEVICE_PROC_ADDR(vkDeviceWaitIdle);
    SET_DEVICE_PROC_ADDR(vkResetFences);

    device_context->vkGetDeviceQueue(
        device->logical_device,
        device->graphics_family_index,
        0,
        &device_context->d_graphics_queue);

    device_context->vkGetDeviceQueue(
        device->logical_device,
        device->present_family_index,
        0,
        &device_context->d_present_queue);

    return true;
}

void vulkan_device_context_destroy()
{
}

bool vulkan_swapchain_init(
    const VulkanInstance* vk,
    const VulkanDevice* device,
    const VulkanDeviceContext* context,
    const VkSurfaceKHR surface,
    VulkanSwapchain* vk_swapchain)
{
    vk_swapchain->d_context = context;
    vk_swapchain->d_surface = surface;

    const VulkanDeviceDescription* desc = device->d_device_description;

    // Choose swap chain defaults
    // NOTE: higher level API should be able to choose eventually.

    // Choose image format
    const VkSurfaceFormatKHR * available_formats = desc->maybe_compat.surface_formats;
    const uint32_t format_count = desc->maybe_compat.surface_format_count;

    vk_swapchain->surface_format = *available_formats; // default to first format
    for (uint32_t i = 0; i < format_count; i++) {
        const VkSurfaceFormatKHR* surface_format = available_formats + i;
        if (surface_format->format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_format->colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            vk_swapchain->surface_format = *surface_format;
            break;
        }
    }

    const VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vk->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device->d_physical_device,
        surface,
        &capabilities);

    if (result != VK_SUCCESS) {
        return false;
    }

    vk_swapchain->image_extent = capabilities.currentExtent;

    printf("WIDTH: %d HEIGHT: %d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
    // TODO this is not updating with window size update.... what is the point of even using it?

    const uint32_t max_image_count = MAX_IMAGE < capabilities.maxImageCount
        ? MAX_IMAGE
        : capabilities.maxImageCount;
    const uint32_t image_count = capabilities.minImageCount < max_image_count
        ? capabilities.minImageCount + 1
        : capabilities.minImageCount;

    if (vk_swapchain->image_extent.width == 0 || vk_swapchain->image_extent.height == 0) {
        // Window may be minimized, can't abide by valid usage with zero
        // width/height.
        return false;
    }
    // TODO allegedly high DPI screens may cause platform surface capabilities
    // currentExtent to be incorrectly reported as native and not pixel sizes. I
    // see that as a bug, but if it persists than I can work around that if
    // needed (should test this).

    const uint32_t queue_family_indices[2] = {
        device->graphics_family_index,
        device->present_family_index,
    };
    const bool multi_queue = queue_family_indices[0] != queue_family_indices[1];

    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = vk_swapchain->surface_format.format,
        .imageColorSpace = vk_swapchain->surface_format.colorSpace,
        .imageExtent = vk_swapchain->image_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = multi_queue ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = multi_queue ? 2 : 0,
        .pQueueFamilyIndices = queue_family_indices,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    result = context->vkCreateSwapchainKHR(
        device->logical_device,
        &create_info,
        vk->allocator,
        &vk_swapchain->swapchain);

    if (result != VK_SUCCESS) {
        return false;
    }


    return true;
}

void vulkan_swapchain_destroy(
    const VulkanInstance* vk,
    const VulkanDevice* device,
    const VulkanDeviceContext* context,
    VulkanSwapchain* vk_swapchain)
{
    context->vkDestroySwapchainKHR(
        device->logical_device,
        vk_swapchain->swapchain,
        vk->allocator);
}

bool vulkan_image_view_list_init(
    const uint32_t count,
    const VulkanDeviceContext* context,
    const VkImage* images,
    const VkFormat format,
    VkImageViewList list)
{
    for (uint32_t i = 0; i < count; i++) {
        const VkComponentMapping components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };
        const VkImageSubresourceRange subresource_range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        const VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = *(images + i),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = components,
            .subresourceRange = subresource_range,
        };
        const VkResult result = context->vkCreateImageView(
            context->d_device->logical_device,
            &create_info,
            context->d_device->d_vk->allocator,
            list + i);
        if (result != VK_SUCCESS) {
            vulkan_image_view_list_destroy(i, context, list);
            return false;
        }
    }
    return true;
}

void vulkan_image_view_list_destroy(
    const uint32_t count,
    const VulkanDeviceContext* context,
    VkImageViewList list)
{
    for (uint32_t i = 0; i < count; i++) {
        context->vkDestroyImageView(
            context->d_device->logical_device,
            list[i],
            context->d_device->d_vk->allocator);
    }
}


bool vulkan_swapchain_images_init(
    const VulkanSwapchain* swapchain,
    VulkanSwapchainImages* images)
{
    images->d_swapchain = swapchain;

    // Retrieve image handles
    images->image_count = MAX_IMAGE;
    VkResult result = swapchain->d_context->vkGetSwapchainImagesKHR(
        swapchain->d_context->d_device->logical_device,
        swapchain->swapchain,
        &images->image_count,
        images->d_images);

    if (result != VK_SUCCESS) {
        return false;
    }

    // TODO: either malloc images array or stack alloc to max size the image views array
    // to be consistent. If we are mallocing images array then we _should_ stick to the
    // single resource rule and have VulkanSwapchainImageviews be a separate struct that
    // depends on VulkanSwapchainImages to keep the init / destroy clean.
    images->image_view_list = malloc(sizeof(VkImageView) * images->image_count);
    if (images->image_view_list == NULL) {
        return false;
    }

    const bool image_views_success = vulkan_image_view_list_init(
        images->image_count,
        images->d_swapchain->d_context,
        images->d_images,
        images->d_swapchain->surface_format.format,
        images->image_view_list);
    if (image_views_success == false) {
        free(images->image_view_list);
        return false;
    }

    return true;
}

void vulkan_swapchain_images_destroy(VulkanSwapchainImages* images) {
    vulkan_image_view_list_destroy(
        images->image_count,
        images->d_swapchain->d_context,
        images->image_view_list);
    free(images->image_view_list);
}

bool vulkan_render_pass_init(
    const VulkanDeviceContext *context,
    const VulkanSwapchainImages *swapchain_images,
    VulkanRenderPass* vk_render_pass)
{
    vk_render_pass->d_context = context;
    vk_render_pass->d_swapchain_images = swapchain_images;

    const VkAttachmentDescription attachment = {
        .format = swapchain_images->d_swapchain->surface_format.format,
        .samples = 1,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // TODO this is a bug
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    const VkAttachmentReference attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_reference,
        .pResolveAttachments = NULL,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
        .pDepthStencilAttachment = NULL,
    };


    // TODO use synch2 & dynamic rendering
    const VkSubpassDependency subpass_dependency = {
      .dependencyFlags = 0,
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstSubpass = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    const VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependency,
    };

    const VkResult result = context->vkCreateRenderPass(
        context->d_device->logical_device,
        &create_info,
        context->d_device->d_vk->allocator,
        &vk_render_pass->render_pass);

    return result == VK_SUCCESS;
}

void vulkan_render_pass_destroy(
    VulkanRenderPass* render_pass)
{
    const VulkanDeviceContext* context = render_pass->d_context;
    context->vkDestroyRenderPass(
        context->d_device->logical_device,
        render_pass->render_pass,
        context->d_device->d_vk->allocator);
}

bool vulkan_graphics_pipeline_init(
    const VulkanDeviceContext* context,
    const VulkanShaders* shaders,
    const VulkanRenderPass* vk_render_pass,
    VulkanGraphicsPipeline* graphics_pipeline) {

    graphics_pipeline->d_context = context;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL,
    };

    const VkResult result = context->vkCreatePipelineLayout(
        context->d_device->logical_device,
        &pipeline_layout_create_info,
        context->d_device->d_vk->allocator,
        &graphics_pipeline->pipeline_layout);
    if (result != VK_SUCCESS) {
        return false;
    }

    VkPipelineShaderStageCreateInfo shader_create_infos[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .pName = "main",
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaders->vertex_shader,
            .pSpecializationInfo = NULL,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .pName = "main",
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaders->fragment_shader,
            .pSpecializationInfo = NULL,
        },
    };

    const VkDynamicState dynamic_state[2] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_state,
        .flags = 0,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .scissorCount = 1,
        .pScissors = NULL, // dynamic
        .viewportCount = 1,
        .pViewports = NULL, // dynamic
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0,
        .depthBiasClamp = 0,
        .depthBiasSlopeFactor = 0,
        .lineWidth = 1.0,
    };

    VkPipelineMultisampleStateCreateInfo multisample_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colour_blend_attachment_state = {
        .colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    };

    VkPipelineColorBlendStateCreateInfo colour_blend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .blendConstants = {0},
        .attachmentCount = 1,
        .pAttachments = &colour_blend_attachment_state,
    };

    VkGraphicsPipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .stageCount = 2,
        .pStages = shader_create_infos,
        .pColorBlendState = &colour_blend_info,
        .pDepthStencilState = NULL,
        .pDynamicState = &dynamic_state_create_info,
        .pInputAssemblyState = &input_assembly_create_info,
        .pVertexInputState = &vertex_input_create_info,
        .pMultisampleState = &multisample_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pTessellationState = NULL,
        .renderPass = vk_render_pass->render_pass,
        .subpass = 0,
        .layout = graphics_pipeline->pipeline_layout,
        .basePipelineHandle = NULL,
        .basePipelineIndex = -1,
    };

    context->vkCreateGraphicsPipelines(
        context->d_device->logical_device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        context->d_device->d_vk->allocator,
        &graphics_pipeline->graphics_pipeline);

    return true;
}

void vulkan_graphics_pipeline_destroy(
    VulkanGraphicsPipeline* graphics_pipeline) {
    const VulkanDeviceContext* context = graphics_pipeline->d_context;
    context->vkDestroyPipeline(
        context->d_device->logical_device,
        graphics_pipeline->graphics_pipeline,
        context->d_device->d_vk->allocator);
    context->vkDestroyPipelineLayout(
        context->d_device->logical_device,
        graphics_pipeline->pipeline_layout, 
        context->d_device->d_vk->allocator);
}

bool vulkan_framebuffers_init(
    const VulkanRenderPass* render_pass,
    VulkanFramebuffers* vk_framebuffers)
{
    const VulkanDeviceContext* context = render_pass->d_context;
    const VulkanSwapchainImages* swapchain_images = render_pass->d_swapchain_images;
    const uint32_t image_count = swapchain_images->image_count;
    vk_framebuffers->d_swapchain_images = swapchain_images;
    vk_framebuffers->d_render_pass = render_pass;
    vk_framebuffers->d_context = context;
    vk_framebuffers->framebuffer_count = 0;

    const VkExtent2D image_extent = swapchain_images->d_swapchain->image_extent;

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageView image_view = swapchain_images->image_view_list[i];

        const VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = render_pass->render_pass,
            .attachmentCount = 1,
            .pAttachments = &image_view,
            .width = image_extent.width,
            .height = image_extent.height,
            .layers = 1,
        };

        // Question: why do width, height, layers need to be specified
        // when the image view should maintain that information already?
        // What happens if they disagree?

        const VkResult result = context->vkCreateFramebuffer(
            context->d_device->logical_device,
            &create_info,
            context->d_device->d_vk->allocator,
            vk_framebuffers->framebuffers + i);
            
        if (result != VK_SUCCESS) {
            // Destroy previously created framebuffers up to framebuffer count
            vulkan_framebuffers_destroy(vk_framebuffers);
            return false;
        }
        else {
            vk_framebuffers->framebuffer_count += 1;
        }
    }

    return true;
}

void vulkan_framebuffers_destroy(VulkanFramebuffers* vk_framebuffers)
{
    const PFN_vkDestroyFramebuffer vkDestroyFramebuffer =
        vk_framebuffers->
        d_context->
        vkDestroyFramebuffer;
    for (uint32_t i = 0; i < vk_framebuffers->framebuffer_count; i++) {
        vkDestroyFramebuffer(
            vk_framebuffers->d_context->d_device->logical_device,
            vk_framebuffers->framebuffers[i],
            vk_framebuffers->d_context->d_device->d_vk->allocator);
    }
}

void vulkan_commands_destroy(
    const VulkanDeviceContext* context,
    VulkanCommands* commands)
{
    context->vkDestroyCommandPool(
        context->d_device->logical_device,
        commands->command_pool,
        context->d_device->d_vk->allocator);
}

bool vulkan_commands_init(
    const VulkanDeviceContext* context,
    VulkanCommands* commands)
{
    // TODO 
    const VkCommandPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = context->d_device->graphics_family_index,
    };
    const VkResult command_result = context->vkCreateCommandPool(
        context->d_device->logical_device,
        &pool_create_info,
        context->d_device->d_vk->allocator,
        &commands->command_pool);

    if (command_result != VK_SUCCESS) {
        return false;
    }

    // Command Buffers

    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = commands->command_pool,
        .commandBufferCount = NUM_ACTIVE_BATCHES,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };

    const VkResult buffer_result = context->vkAllocateCommandBuffers(
        context->d_device->logical_device,
        &alloc_info,
        commands->command_buffers);

    if (buffer_result != VK_SUCCESS) {
        vulkan_commands_destroy(context, commands);
        return false;
    }

    return true;
}

bool vulkan_sync_objects_init(
    const VulkanDeviceContext* context,
    VulkanSyncObjects* sync_objects)
{
    VkDevice device = context->d_device->logical_device;
    const VkAllocationCallbacks* allocator = context->d_device->d_vk->allocator;

    const VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    const VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        .pNext = NULL,
    };

    for (uint32_t i = 0; i < NUM_ACTIVE_BATCHES; i++) {

        sync_objects->image_acquired[i] = VK_NULL_HANDLE;
        VkResult result = context->vkCreateSemaphore(
            device,
            &semaphore_create_info,
            allocator,
            &sync_objects->image_acquired[i]);

        if (result != VK_SUCCESS) {
            vulkan_sync_objects_destroy(context, sync_objects);
            return false;
        }

        sync_objects->draw_finished[i] = VK_NULL_HANDLE;
        result = context->vkCreateSemaphore(
            device,
            &semaphore_create_info,
            allocator,
            &sync_objects->draw_finished[i]);

        if (result != VK_SUCCESS) {
            vulkan_sync_objects_destroy(context, sync_objects);
            return false;
        }

        sync_objects->command_buffer_available[i] = VK_NULL_HANDLE;
        result = context->vkCreateFence(
            device,
            &fence_create_info,
            allocator,
            &sync_objects->command_buffer_available[i]);

        if (result != VK_SUCCESS) {
            vulkan_sync_objects_destroy(context, sync_objects);
            return false;
        }
    }

    return true;
}

void vulkan_sync_objects_destroy(
    const VulkanDeviceContext* context,
    VulkanSyncObjects* sync_objects)
{
    VkDevice device = context->d_device->logical_device;
    const VkAllocationCallbacks* allocator = context->d_device->d_vk->allocator;

    for (uint32_t i = 0; i < NUM_ACTIVE_BATCHES; i++) {

        if (sync_objects->image_acquired[i] == VK_NULL_HANDLE) {
            return;
        }

        context->vkDestroySemaphore(
            device,
            sync_objects->image_acquired[i],
            allocator);

        if (sync_objects->draw_finished[i] == VK_NULL_HANDLE) {
            return;
        }

        context->vkDestroySemaphore(
            device,
            sync_objects->draw_finished[i],
            allocator);

        if (sync_objects->command_buffer_available[i] == VK_NULL_HANDLE) {
            return;
        }

        context->vkDestroyFence(
            device,
            sync_objects->command_buffer_available[i],
            allocator);

    }

}


bool vulkan_draw_mutate(
    const VulkanDeviceContext* context,
    const VulkanSwapchain* vk_swapchain,
    const VulkanGraphicsPipeline* vk_graphics_pipeline,
    const VulkanCommands* commands,
    const VulkanSyncObjects* sync,
    const VulkanFramebuffers* framebuffers)
{
    VkResult result;
    // Wait for available command buffer
    VkDevice device = context->d_device->logical_device;
    context->vkWaitForFences(
        device,
        1,
        &sync->command_buffer_available[0],
        VK_TRUE,
        INT_MAX);

    context->vkResetFences(
        device,
        1,
        &sync->command_buffer_available[0]);

    // Request image for drawing
    uint32_t image_index;
    result = context->vkAcquireNextImageKHR(
        device,
        vk_swapchain->swapchain,
        INT_MAX,
        sync->image_acquired[0],
        VK_NULL_HANDLE,
        &image_index);

    if (result != VK_SUCCESS) {
        return false;
    }

    context->vkDeviceWaitIdle(context->d_device->logical_device);

    // Record Command buffer:
    VkCommandBuffer command_buffer = commands->command_buffers[0];
    context->vkResetCommandBuffer(command_buffer, 0);

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    context->vkBeginCommandBuffer(command_buffer, &begin_info);
    {
        VkClearValue clear_value = {
            .color = { .int32 = {0, 0, 0, 0} },
        };
        const VkRenderPassBeginInfo render_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = framebuffers->d_render_pass->render_pass,
            .renderArea = { .extent = vk_swapchain->image_extent },
            .framebuffer = framebuffers->framebuffers[image_index],
            .clearValueCount = 1,
            .pClearValues = &clear_value, 
        };
        context->vkCmdBeginRenderPass(
            command_buffer,
            &render_begin_info,
            VK_SUBPASS_CONTENTS_INLINE);

        context->vkCmdBindPipeline(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            vk_graphics_pipeline->graphics_pipeline);

        const VkRect2D scissor_rect = {
            .extent = vk_swapchain->image_extent,
            .offset = { .x = 0, .y = 0 },
        };

        context->vkCmdSetScissor(
            command_buffer,
            0,
            1,
            &scissor_rect);

        const VkViewport viewport = {
            .x = 0,
            .y = 0,
            .width = (float)vk_swapchain->image_extent.width,
            .height = (float)vk_swapchain->image_extent.height,
            .minDepth = 0.0,
            .maxDepth = 1.0,
        };
        context->vkCmdSetViewport(
            command_buffer,
            0,
            1,
            &viewport);

        context->vkCmdDraw(
            command_buffer,
            3,  // vertexCount
            1,  // indexCount
            0,  // firstVertex
            0); // firstInstance

        context->vkCmdEndRenderPass(command_buffer);
    }
    context->vkEndCommandBuffer(command_buffer);

    // Submission

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sync->image_acquired[0],
        .pWaitDstStageMask = &wait_stage,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sync->draw_finished[0],
    };
    result = context->vkQueueSubmit(
        context->d_graphics_queue,
        1,
        &submit_info,
        sync->command_buffer_available[0]);

    context->vkDeviceWaitIdle(context->d_device->logical_device);

    if (result != VK_SUCCESS) {
        return false;
    }

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sync->draw_finished[0],
        .pResults = NULL,
        .swapchainCount = 1,
        .pSwapchains = &vk_swapchain->swapchain,
        .pImageIndices = &image_index,
    };
    result = context->vkQueuePresentKHR(
        context->d_present_queue,
        &present_info);

    if (result != VK_SUCCESS) {
        return false;
    }

    context->vkDeviceWaitIdle(context->d_device->logical_device);

    return true;
}
