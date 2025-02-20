#ifndef SMIRC_VK_H
#define SMIRC_VK_H

#include <stdbool.h>
#include <stdint.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// This file is organized in dependency order: a dependency object is followed
// by it's dependant objects.

// Conventions:
// -------------------
// - Members that point to objects which must outlive struct prefixed with 'd_'
// - Optional objects prefixed with 'maybe_'.
// - Not formalizing everything yet, but there's other stuff going on.

typedef struct VulkanCreateInfo {
    // Lifetime Dependencies:
    // Vulkan Loader shared library
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    const char *const * extension_names;  
    const uint32_t extension_count;
    const char *const * layer_names;  
    const uint32_t layer_count;
} VulkanCreateInfo;

// Instance wrapper
typedef struct VulkanInstance {
    // Init Dependencies:
    // VulkanCreateInfo

    // Lifetime Dependencies:
    // Vulkan Loader shared library
    
    VkInstance instance;

    // TODO allocator:
    // If we are maintaining that sibling dependencies can be simultaneously
    // manipulated (read-only parent references, write access to children),
    // then a global allocator needs to be simultaneously accessible, or
    // else we should have child allocators that can be distinct between
    // siblings.
    const VkAllocationCallbacks* allocator;

    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
} VulkanInstance;

typedef struct {
    // Init Dependencies:
    // VulkanInstance
    // VkPhysicalDevice
    // VkSurfaceKHR

    VkSurfaceFormatKHR* surface_formats; // managed resource
    uint32_t surface_format_count;
    VkPresentModeKHR* present_modes; // managed resource (breaking rule)
    uint32_t present_mode_count;
    VkSurfaceCapabilitiesKHR surface_capabilities;
} VulkanDeviceSurfaceCompat;

typedef struct {
    // Init Dependencies:
    // VulkanInstance
    // VkPhysicalDevice
    // Maybe VkSurfaceKHR

    // Lifetime dependencies
    VkPhysicalDevice d_physical_device;

    uint32_t family_count;
    VkQueueFamilyProperties* queue_family_properties; // managed resource

    uint32_t extension_count;
    VkExtensionProperties* extension_properties; // managed resource

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;

    // Optional temp index location for first present-capable queue family
    // depends on surface_compay
    uint32_t present_family_index; 

    // Optional structure for surface compat details
    bool surface_compat;
    VulkanDeviceSurfaceCompat maybe_compat; // managed

    // NOTE: maybe add a SurfaceCompatList dependency to the
    // typed dependency chain instead of an optional
    // member here. VulkanDevice depends on this, which
    // depends on list of device descriptions (filtered
    // to separate list of compatible descriptions)
    // This allows future dependency chains having device
    // descriptions without requiring surface as a dependency,
    // for other vulkan usage than display.

} VulkanDeviceDescription;

typedef struct {
    // Init Dependencies:
    // VulkanInstance
    // Maybe VkSurfaceKHR
    VulkanDeviceDescription* physical_device_descriptions;
    uint32_t count;
} VulkanPhysicalDeviceList;


typedef struct VulkanDevice {
    // Lifetime Dependencies:
    // VulkanPhysicalDeviceList
    const VulkanDeviceDescription* d_device_description;
    const VulkanInstance* d_vk;
    VkPhysicalDevice d_physical_device; // Owned by dependency (Instance)

    VkDevice logical_device;
    uint32_t graphics_family_index;
    uint32_t present_family_index;
} VulkanDevice;


typedef struct VulkanDeviceContext {
    // Lifetime Dependencies:
    const VulkanDevice* d_device;
    VkQueue d_graphics_queue; // Owned by Logical Device
    VkQueue d_present_queue; // Owned by Logical Device

    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkDestroyImageView vkDestroyImageView;
    PFN_vkCreateShaderModule vkCreateShaderModule;
    PFN_vkDestroyShaderModule vkDestroyShaderModule;
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkDestroyRenderPass vkDestroyRenderPass;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkDestroyPipeline vkDestroyPipeline;
    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkDestroyCommandPool vkDestroyCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkResetCommandBuffer vkResetCommandBuffer;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdDraw vkCmdDraw;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkDestroySemaphore vkDestroySemaphore;
    PFN_vkCreateFence vkCreateFence;
    PFN_vkDestroyFence vkDestroyFence;
    PFN_vkResetFences vkResetFences;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
} VulkanDeviceContext;

typedef struct VulkanShaders {
    // Lifetime Dependencies:
    const VulkanDeviceContext* d_context;
    
    VkShaderModule vertex_shader;
    VkShaderModule fragment_shader;
} VulkanShaders;

typedef struct VulkanSwapchain {
    // Lifetime Dependencies:
    const VulkanDeviceContext* d_context;
    VkSurfaceKHR d_surface;

    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surface_format;
    VkExtent2D image_extent;
} VulkanSwapchain;

typedef VkImageView* VkImageViewList;
#define MAX_IMAGE 10
typedef struct VulkanSwapchainImages {
    // Lifetime Dependencies
    const VulkanSwapchain* d_swapchain;
    VkImage d_images[MAX_IMAGE]; 
    uint32_t image_count;

    VkImageViewList image_view_list;

} VulkanSwapchainImages;

typedef struct VulkanRenderPass {
    // Lifetime Dependencies
    const VulkanDeviceContext* d_context;
    const VulkanSwapchainImages* d_swapchain_images;

    VkRenderPass render_pass;
} VulkanRenderPass;

typedef struct VulkanGraphicsPipeline {
    // Lifetime Dependencies:
    // - VulkanShaders
    //   TODO shader modules are actually init dependencies
    //   not lifetime dependencies. Can rework this to save
    //   on memory, but I won't for now.
    const VulkanDeviceContext* d_context;
    
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;
} VulkanGraphicsPipeline;

typedef struct VulkanFramebuffers {
    // Lifetime dependencies
    const VulkanDeviceContext* d_context;
    const VulkanSwapchainImages* d_swapchain_images;
    const VulkanRenderPass* d_render_pass;

    uint32_t framebuffer_count;
    VkFramebuffer framebuffers[MAX_IMAGE];
} VulkanFramebuffers;

typedef struct VulkanSyncObjects {
    // Lifetime Dependencies
    // VulkanDeviceContext

    VkSemaphore image_acquired;
    VkSemaphore draw_finished;
    VkFence command_buffer_available;
} VulkanSyncObjects;

typedef struct VulkanCommands {
    // Lifetime Dependencies:
    // VulkanDeviceContext

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
} VulkanCommands;

// TODO mutation scopes:
// Some exposed tasks will have effects / mutate some portion of the resource
// graph, not just types here but implicit resources defined by the vulkan
// spec. Task composition must ensure that mutable scopes don't overlap with
// other scopes. Parallelism can be achieved with mutation limited to
// non-intersecting resource sets, or without mutation.
// The dependency graph does not currently encode the mutation scopes of SMIRC
// tasks that will be exposed, it's an open question as to whether or not the
// type structure can encode that or not (in addition to the initialization
// dependency order). It could be understood as a separate set of resource nodes
// in the graph that are affected by any one task, if needed.


// TODO remove unnecessary parameters if they can be
bool vulkan_init(VulkanInstance*, const VulkanCreateInfo *const vk_info);
void vulkan_destroy(const VulkanInstance*);

bool vulkan_physical_device_list_init(
    const VulkanInstance* vk,
    const VkSurfaceKHR maybe_surface,
    VulkanPhysicalDeviceList* list);
void vulkan_physical_device_list_destroy(VulkanPhysicalDeviceList* list);

bool vulkan_device_init(
    const VulkanInstance* vk,
    const VulkanPhysicalDeviceList* physical_devices,
    VulkanDevice* device);
void vulkan_device_destroy(
    const VulkanInstance* vk,
    const VulkanDevice* device);

bool vulkan_device_context_init(
    const VulkanInstance* vk,
    const VulkanDevice* device,
    VulkanDeviceContext* device_context);
void vulkan_device_context_destroy();

bool vulkan_shaders_init(const VulkanDeviceContext* context, VulkanShaders* shaders);
void vulkan_shaders_destroy(const VulkanDeviceContext* context, VulkanShaders* shaders);

bool vulkan_swapchain_init(
    const VulkanInstance* vk,
    const VulkanDevice* device,
    const VulkanDeviceContext* context,
    const VkSurfaceKHR surface,
    VulkanSwapchain* swapchain);
void vulkan_swapchain_destroy(
    const VulkanInstance* vk,
    const VulkanDevice* device,
    const VulkanDeviceContext* context,
    VulkanSwapchain* swapchain);

bool vulkan_image_view_list_init(
    const uint32_t count,
    const VulkanDeviceContext* device,
    const VkImage* images,
    const VkFormat format,
    VkImageViewList list);

void vulkan_image_view_list_destroy(
    const uint32_t count,
    const VulkanDeviceContext* device,
    VkImageViewList list);

bool vulkan_swapchain_images_init(
    const VulkanSwapchain* swapchain,
    VulkanSwapchainImages* images);
void vulkan_swapchain_images_destroy(VulkanSwapchainImages* images);

bool vulkan_graphics_pipeline_init(
    const VulkanDeviceContext* context,
    const VulkanShaders* shaders,
    const VulkanRenderPass* render_pass,
    VulkanGraphicsPipeline* graphics_pipeline);
void vulkan_graphics_pipeline_destroy(
    VulkanGraphicsPipeline *graphics_pipeline);

bool vulkan_render_pass_init(
    const VulkanDeviceContext *context,
    const VulkanSwapchainImages *swapchain_images,
    VulkanRenderPass* vk_render_pass);
void vulkan_render_pass_destroy(
    VulkanRenderPass* vk_render_pass);

bool vulkan_framebuffers_init(
    const VulkanRenderPass* vk_render_pass,
    VulkanFramebuffers* vk_framebuffers);
void vulkan_framebuffers_destroy(VulkanFramebuffers* vk_framebuffers);

bool vulkan_commands_init(
    const VulkanDeviceContext* context,
    VulkanCommands* commands);
void vulkan_commands_destroy(
    const VulkanDeviceContext* context,
    VulkanCommands* commands);

bool vulkan_sync_objects_init(
    const VulkanDeviceContext* context,
    VulkanSyncObjects* sync_objects);
void vulkan_sync_objects_destroy(
    const VulkanDeviceContext* context,
    VulkanSyncObjects* sync_objects);


// TODO We don't mutate the host objects, but we
// are mutating backing vulkan objects of these params,
// this must be codefied somewhere when implementing
// parallelism.
bool vulkan_draw_mutate(
    const VulkanDeviceContext* context,
    const VulkanSwapchain* vk_swapchain,
    const VulkanGraphicsPipeline* vk_graphics_pipeline,
    const VulkanCommands* commands,
    const VulkanSyncObjects* sync,
    const VulkanFramebuffers* framebuffers);
#endif
