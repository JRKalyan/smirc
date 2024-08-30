const util = @import("util.zig");
const print = util.print;
const printn = util.printn;

const c_import = @import("c_import.zig");
const c = c_import.c;

// CONVENTIONS:
// 'emake' prefix indicates a function that returns a resource that must be
// cleaned with the corresponding 'c_' prefixed function, unless it returns
// an error

fn debugCallback(
    message_severity: c.VkDebugUtilsMessageSeverityFlagBitsEXT,
    message_type: c.VkDebugUtilsMessageTypeFlagsEXT,
    callback_data: [*c]const c.VkDebugUtilsMessengerCallbackDataEXT,
    user_data: ?*anyopaque,
) callconv(.C) c.VkBool32 {
    _ = message_severity;
    _ = message_type;
    _ = user_data;
    printn("DEBUG CALLBACK:");
    printn(callback_data.*.pMessage);
    return c.VK_FALSE;
}

pub fn makeDebugMessengerCreateInfo() c.VkDebugUtilsMessengerCreateInfoEXT {
    return c.VkDebugUtilsMessengerCreateInfoEXT{
        .sType = c.VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = c.VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | c.VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | c.VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = null,
        .pNext = null,
        .flags = 0,
    };
}

// Shader Modules
// --------------
pub fn createShaderModule(
    logical_device: c.VkDevice,
    comptime path: []const u8,
) c.VkShaderModule {
    var shader_module: c.VkShaderModule = undefined;

    const bytes = @embedFile(path);

    // Fix alignment by creating an array of u32 using the file bytes
    if (comptime bytes.len % 4 != 0) {
        @compileError("Shader bytecode not a multiple of 4");
    }
    var byte_code: [bytes.len / 4]u32 = undefined;
    comptime {
        const code_ptr = @ptrCast([*]u8, &byte_code);
        @memcpy(code_ptr, bytes, bytes.len);
    }

    const create_info = c.VkShaderModuleCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = null,
        .flags = 0,
        .codeSize = bytes.len,
        .pCode = &byte_code,
    };

    if (c.vkCreateShaderModule(logical_device, &create_info, null, &shader_module) != c.VK_SUCCESS) {
        return null;
    }

    return shader_module;
}

// Buffer Creation
// ---------------
pub const Buffer = struct {
    vk_buffer: c.VkBuffer,
    memory: c.VkDeviceMemory,
};
// TODO can I union error sets in a better way?
pub const BufferCreationError = error{
    VkCreateBufferError,
    NoSuitableMemoryType,
    VkAllocateMemoryError,
    VkBindBufferMemoryError,
};
pub const VkBufferError = error{
    VkCreateBufferError,
};
pub const DeviceMemoryError = error{
    NoSuitableMemoryType,
    VkAllocateMemoryError,
};

pub fn destroy_buffer(logical_device: c.VkDevice, buffer: Buffer) void {
    destroy_deviceMemory(logical_device, buffer.memory);
    destroy_vkBuffer(logical_device, buffer.vk_buffer);
}

fn emake_vkBuffer(
    logical_device: c.VkDevice,
    size: c.VkDeviceSize,
    usage_flags: c.VkBufferUsageFlags,
) VkBufferError!c.VkBuffer {
    var vk_buffer: c.VkBuffer = undefined;
    {
        const buffer_create_info = c.VkBufferCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .size = size,
            .usage = usage_flags,
            //.usage = c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = null,
        };
        if (c.vkCreateBuffer(logical_device, &buffer_create_info, null, &vk_buffer) != c.VK_SUCCESS) {
            return VkBufferError.VkCreateBufferError;
        }
    }
    return vk_buffer;
}

fn destroy_vkBuffer(logical_device: c.VkDevice, vk_buffer: c.VkBuffer) void {
    c.vkDestroyBuffer(logical_device, vk_buffer, null);
}

pub fn emake_buffer(
    physical_device: c.VkPhysicalDevice,
    logical_device: c.VkDevice,
    size: c.VkDeviceSize,
    usage_flags: c.VkBufferUsageFlags,
    required_property_flags: c.VkMemoryPropertyFlags,
) BufferCreationError!Buffer {
    var success = false;
    var vk_buffer = try emake_vkBuffer(logical_device, size, usage_flags);
    defer {
        if (!success) {
            destroy_vkBuffer(logical_device, vk_buffer);
        }
    }

    var buffer_memory_requirements: c.VkMemoryRequirements = undefined;
    c.vkGetBufferMemoryRequirements(logical_device, vk_buffer, &buffer_memory_requirements);

    const buffer_memory = try emake_deviceMemory(
        logical_device,
        physical_device,
        buffer_memory_requirements,
        required_property_flags,
    );
    defer {
        if (!success) {
            destroy_deviceMemory(logical_device, buffer_memory);
        }
    }
    // TODO can I generalize a way to do try (emake_obj) defer {if not success {destroy_obj}}

    // Bind buffer memory
    if (c.vkBindBufferMemory(logical_device, vk_buffer, buffer_memory, 0) != c.VK_SUCCESS) {
        return BufferCreationError.VkBindBufferMemoryError;
    }

    success = true;
    return Buffer{
        .vk_buffer = vk_buffer,
        .memory = buffer_memory,
    };
}

// TODO make an imageMemory version of this function or generalize so that we can get
// image memory requirements
// TODO rename to resourceMem or something else (just device mem perhaps)
fn emake_deviceMemory(
    logical_device: c.VkDevice,
    physical_device: c.VkPhysicalDevice,
    memory_requirements: c.VkMemoryRequirements,
    required_property_flags: u32,
) DeviceMemoryError!c.VkDeviceMemory {
    var physical_device_memory_properties: c.VkPhysicalDeviceMemoryProperties = undefined;
    c.vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    // choose memory type from available physical device memory types
    var i: u5 = 0;
    const memory_type_index: u32 = while (i < physical_device_memory_properties.memoryTypeCount) : (i += 1) {
        if (memory_requirements.memoryTypeBits & (@intCast(u32, 1) << i) == 0) {
            // Memory type not supported by buffer
            continue;
        }

        if (physical_device_memory_properties.memoryTypes[i].propertyFlags & required_property_flags != required_property_flags) {
            // This memory type does not have the required properties
            continue;
        }

        break i;
    } else {
        return DeviceMemoryError.NoSuitableMemoryType;
    };

    const allocate_info = c.VkMemoryAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = null,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };

    var buffer_memory: c.VkDeviceMemory = undefined;
    if (c.vkAllocateMemory(logical_device, &allocate_info, null, &buffer_memory) != c.VK_SUCCESS) {
        return DeviceMemoryError.VkAllocateMemoryError;
    }
    return buffer_memory;
}

fn destroy_deviceMemory(logical_device: c.VkDevice, buffer_memory: c.VkDeviceMemory) void {
    c.vkFreeMemory(logical_device, buffer_memory, null);
}

// Textures
// -----------
pub const ImageCreationError = error{
    VkCreateImageError,
    NoSuitableMemoryType,
    VkAllocateMemoryError,
    VkBindImageMemoryError,
};

const VkImageCreationError = error{
    VkCreateImageError,
};

pub const Image = struct {
    vk_image: c.VkImage,
    memory: c.VkDeviceMemory,
};

pub fn emake_image(
    logical_device: c.VkDevice,
    physical_device: c.VkPhysicalDevice,
    queue_family_index: u32,
    required_property_flags: c.VkMemoryPropertyFlags,
    width: u32,
    height: u32,
) ImageCreationError!Image {
    var success = false;
    // TODO use existing buffer memory function

    var vk_image = try emake_vkImage(logical_device, queue_family_index, width, height);
    defer {
        if (!success) {
            destroy_vkImage(logical_device, vk_image);
        }
    }

    // Memory requirements
    var image_memory_requirements: c.VkMemoryRequirements = undefined;
    c.vkGetImageMemoryRequirements(logical_device, vk_image, &image_memory_requirements);

    var memory = try emake_deviceMemory(
        logical_device,
        physical_device,
        image_memory_requirements,
        required_property_flags,
    );
    defer {
        if (!success) {
            destroy_deviceMemory(logical_device, memory);
        }
    }

    if (c.vkBindImageMemory(logical_device, vk_image, memory, 0) != c.VK_SUCCESS) {
        return ImageCreationError.VkBindImageMemoryError;
    }

    success = true;
    return Image{
        .vk_image = vk_image,
        .memory = memory,
    };
}

fn emake_vkImage(
    logical_device: c.VkDevice,
    queue_family_index: u32,
    width: u32,
    height: u32,
) VkImageCreationError!c.VkImage {
    var vk_image: c.VkImage = undefined;

    const create_info = c.VkImageCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = null,
        .flags = 0,
        .imageType = c.VK_IMAGE_TYPE_2D,
        .format = c.VK_FORMAT_R32_SFLOAT, // TODO pass this in as well. This might not be the right format
        .extent = c.VkExtent3D{
            .width = width,
            .height = height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = c.VK_SAMPLE_COUNT_1_BIT,
        .tiling = c.VK_IMAGE_TILING_LINEAR,
        .usage = c.VK_IMAGE_USAGE_TRANSFER_DST_BIT | c.VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queue_family_index,
        .initialLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (c.vkCreateImage(logical_device, &create_info, null, &vk_image) != c.VK_SUCCESS) {
        return VkImageCreationError.VkCreateImageError;
    }

    return vk_image;
}

fn destroy_vkImage(logical_device: c.VkDevice, image: c.VkImage) void {
    c.vkDestroyImage(logical_device, image, null);
}

pub fn destroy_image(logical_device: c.VkDevice, image: Image) void {
    destroy_deviceMemory(logical_device, image.memory);
    destroy_vkImage(logical_device, image.vk_image);
}

// TODO
// One time submit command buffers
// -------------------------------

pub const OneTimeCommandBufferError = error{
    AllocationError,
    VkBeginCommandBufferError,
};

// TODO fixup
pub fn emake_oneTimeCommandBuffer(
    logical_device: c.VkDevice,
    command_pool: c.VkCommandPool,
) OneTimeCommandBufferError!c.VkCommandBuffer {
    var success = false;
    var command_buffer: c.VkCommandBuffer = null;
    {
        const command_buffer_alloc_info = c.VkCommandBufferAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = null,
            .commandPool = command_pool,
            .level = c.VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        if (c.vkAllocateCommandBuffers(logical_device, &command_buffer_alloc_info, &command_buffer) != c.VK_SUCCESS) {
            return OneTimeCommandBufferError.AllocationError;
        }
    }
    defer {
        if (!success) {
            c.vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
        }
    }

    const begin_info = c.VkCommandBufferBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = null,
        .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = null,
    };

    if (c.vkBeginCommandBuffer(command_buffer, &begin_info) != c.VK_SUCCESS) {
        return OneTimeCommandBufferError.VkBeginCommandBufferError;
    }

    success = true;
    return command_buffer;
}

pub const DestroyOneTimeCommandBufferError = error{
    VkEndCommandBufferError,
    VkQueueSubmitError,
    VkQueueWaitIdleError,
};

// TODO call and check the error of, add arguments so this compiles
pub fn edestroy_oneTimeCommandBuffer(
    logical_device: c.VkDevice,
    command_buffer: c.VkCommandBuffer,
    command_pool: c.VkCommandPool,
    queue: c.VkQueue,
) DestroyOneTimeCommandBufferError!void {
    // End record Command buffer
    if (c.vkEndCommandBuffer(command_buffer) != c.VK_SUCCESS) {
        return DestroyOneTimeCommandBufferError.VkEndCommandBufferError;
    }

    const submit_info = c.VkSubmitInfo{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = null,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = null,
        .pWaitDstStageMask = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = null,
    };

    if (c.vkQueueSubmit(queue, 1, &submit_info, null) != c.VK_SUCCESS) {
        return DestroyOneTimeCommandBufferError.VkQueueSubmitError;
    }

    if (c.vkQueueWaitIdle(queue) != c.VK_SUCCESS) {
        return DestroyOneTimeCommandBufferError.VkQueueWaitIdleError;
    }

    c.vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
}
