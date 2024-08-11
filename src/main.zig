const std = @import("std");

const util = @import("util.zig");
const print = util.print;
const printn = util.printn;

const c_import = @import("c_import.zig");
const c = c_import.c;

const vk = @import("vulkan_wrapper.zig");

const font = @import("font.zig");

const stf = @import("stf.zig");

const U32MAX = 0xFFFFFFFF;
const U64MAX = 0xFFFFFFFFFFFFFFFF;
const DEBUG = true; // TODO how the heck do I do std.builtin.mode now?

const Vertex = struct {
    colour: [3]f32,
    position: [3]f32,
    texture_position: [2]f32,
};

const UniformTransform = struct {
    scale: [2]f32,
};

const glyph_scale = 0.05;
const glyph_width = 2.0 * glyph_scale;
const glyph_height = 2.0 * glyph_scale;

fn createAlphabetVertices(comptime alphabet_size: comptime_int) [alphabet_size * 4]Vertex {
    var vertices: [alphabet_size * 4]Vertex = undefined;
    var i = 0;
    const y_size: comptime_float = 1.0 / @intToFloat(comptime_float, alphabet_size);
    while (i < alphabet_size) {
        const y_offset: comptime_float = i * y_size;
        vertices[i * 4 + 0] = .{ .position = .{ -1.0, -1.0, 1.0 }, .colour = .{ 1.0, 0.0, 0.0 }, .texture_position = .{ 0.0, y_offset } };
        vertices[i * 4 + 1] = .{ .position = .{ 1.0, -1.0, 1.0 }, .colour = .{ 0.0, 1.0, 0.0 }, .texture_position = .{ 1.0, y_offset } };
        vertices[i * 4 + 2] = .{ .position = .{ 1.0, 1.0, 1.0 }, .colour = .{ 0.0, 0.0, 1.0 }, .texture_position = .{ 1.0, y_offset + y_size } };
        vertices[i * 4 + 3] = .{ .position = .{ -1.0, 1.0, 1.0 }, .colour = .{ 1.0, 1.0, 1.0 }, .texture_position = .{ 0.0, y_offset + y_size } };

        i = i + 1;
    }
    return vertices;
}
const alphabet_vertices = createAlphabetVertices(26);

// Indices for drawing a quad from vertices 0-4:
// 0----1
// |    |
// |    |
// 3----2
const indices = [_]u32{
    // Quad 1
    0, 1, 2, 2, 3, 0,
};

//const texture_pixels_n = [_]f32{
//    0.0, 1.0, 0.0,
//    1.0, 0.0, 1.0,
//    1.0, 1.0, 1.0,
//    1.0, 0.0, 1.0,
//    1.0, 0.0, 1.0,
//};
//const texture_width = 3;
//const texture_height = 5;
const texture_pixels_n = font.font_texels;
const texture_width = 12;
const texture_height = 12 * 26;

// TODO windowing stuff can go in another file
var framebuffer_resized: bool = false;
fn framebufferResizeCallback(
    window: ?*c.GLFWwindow,
    width: c_int,
    height: c_int,
) callconv(.C) void {
    _ = window;
    _ = width;
    _ = height;
    framebuffer_resized = true;
}

fn recordCommandBuffer(
    command_buffer: c.VkCommandBuffer,
    framebuffer: c.VkFramebuffer,
    render_pass: c.VkRenderPass,
    render_extent: c.VkExtent2D,
    graphics_pipeline: c.VkPipeline,
    vertex_buffer: c.VkBuffer,
    index_buffer: c.VkBuffer,
    descriptor_set: c.VkDescriptorSet,
    pipeline_layout: c.VkPipelineLayout,
) bool {

    // Begin record command buffer
    const begin_info = c.VkCommandBufferBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = null,
        .flags = 0,
        .pInheritanceInfo = null,
    };

    if (c.vkBeginCommandBuffer(command_buffer, &begin_info) != c.VK_SUCCESS) {
        printn("Failed to begin command buffer");
        return false;
    }

    // Begin render pass
    const clear_value_colour = c.VkClearColorValue{ .float32 = .{ 0.0, 0.0, 0.0, 1.0 } };

    const clear_value = c.VkClearValue{
        .color = clear_value_colour,
    };
    const render_pass_begin_info = c.VkRenderPassBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = null,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = .{ .offset = .{ .x = 0, .y = 0 }, .extent = render_extent },
        .clearValueCount = 1,
        .pClearValues = &clear_value,
    };

    c.vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, c.VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    c.vkCmdBindPipeline(command_buffer, c.VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    // Bind
    const offsets = [_]c.VkDeviceSize{0};
    c.vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offsets);
    c.vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, c.VK_INDEX_TYPE_UINT32);
    c.vkCmdBindDescriptorSets(command_buffer, c.VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, null);

    // Draw
    //c.vkCmdDraw(command_buffer, vertices.len, 1, 0, 0);
    var i: u8 = 0;
    var x_offset: f32 = -1.0 + (glyph_width / 2.0);
    var y_offset: f32 = -1.0 + (glyph_height / 2.0);
    while (i < 26) {
        const push_constant: [4]f32 = .{
            x_offset, y_offset, 0.0, 0.0,
        };

        x_offset = x_offset + 2.0 * glyph_width;
        if ((x_offset + (glyph_width / 2.0)) > 1.0) {
            x_offset = -1.0 + (glyph_width / 2.0);
            y_offset = y_offset + glyph_height * 1.5;
        }

        c.vkCmdPushConstants(
            command_buffer,
            pipeline_layout,
            c.VK_SHADER_STAGE_VERTEX_BIT,
            0,
            @sizeOf(@TypeOf(push_constant)),
            &push_constant,
        );
        c.vkCmdDrawIndexed(command_buffer, 6, 1, 0, i * 4, 0);
        i = i + 1;
    }

    // End render pass
    c.vkCmdEndRenderPass(command_buffer);

    // End record Command buffer
    if (c.vkEndCommandBuffer(command_buffer) != c.VK_SUCCESS) {
        printn("Failed to end command buffer");
        return false;
    }

    return true;
}

pub fn main() anyerror!void {
    printn("😏");

    // STF Tree testing:
    {
        const ascii = "prologue((asd) 123)";
        var stfa: [ascii.len]u8 = undefined;
        for (ascii) |byte, i| {
            // special casing parentheses to be O0 and C0
            print(byte);
            if (byte == 40) {
                stfa[i] = 126; // Open0
            } else if (byte == 41) {
                stfa[i] = 255; // Close
            } else {
                stfa[i] = stf.ascii_to_stfa(byte);
            }
            print(" Maps to: stfa[");
            print(stfa[i]);
            print("] which is: ");
            printn(stf.stfa_to_ascii[stfa[i]]);
        }
        const test_parse = stf.parse(&stfa);
        print("STF COUNT: ");
        print(test_parse.items.len);
    }

    var loop_count: u64 = 0;

    //// Fixed buffer allocator for fun
    //var buffer: [64000]u8 = undefined;
    //var fba = std.heap.FixedBufferAllocator.init(&buffer);
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();

    // GLFW
    if (c.glfwInit() == c.GLFW_FALSE) {
        return;
    }

    if (c.glfwVulkanSupported() == c.GLFW_FALSE) {
        printn("Vulkan not supported");
        return;
    }

    // create a window
    c.glfwWindowHint(c.GLFW_CLIENT_API, c.GLFW_NO_API);
    var window = c.glfwCreateWindow(400, 400, "SMIRC", null, null);

    if (window == null) {
        printn("Failed to create window");
        return;
    }

    _ = c.glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    var instance: c.VkInstance = null;
    {
        const app_info = c.VkApplicationInfo{
            .sType = c.VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "smirc",
            .applicationVersion = c.VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "none",
            .engineVersion = c.VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = c.VK_API_VERSION_1_0,
            .pNext = null,
        };

        var required_glfw_ext_count: u32 = 0;
        const required_glfw_exts = c.glfwGetRequiredInstanceExtensions(&required_glfw_ext_count);
        const all_required_exts = allocator.alloc([*c]const u8, required_glfw_ext_count + (if (comptime DEBUG) 1 else 0)) catch @panic("oom");
        defer allocator.free(all_required_exts);
        {
            var i: u8 = 0;
            while (i < required_glfw_ext_count) : (i += 1) {
                all_required_exts[i] = required_glfw_exts[i];
            }
            if (comptime DEBUG) {
                all_required_exts[i] = c.VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            }
        }
        print("Required Ext Count: ");
        printn(all_required_exts.len);
        for (all_required_exts) |extension_name| {
            printn(extension_name);
        }
        const validation_layers: []const [*:0]const u8 = if (comptime DEBUG) &[_][*:0]const u8{"VK_LAYER_KHRONOS_validation"} else &[_][*:0]const u8{};

        const debug_messenger_create_info: c.VkDebugUtilsMessengerCreateInfoEXT = if (comptime DEBUG)
            vk.makeDebugMessengerCreateInfo()
        else
            undefined;

        const instance_create_info = c.VkInstanceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = if (comptime DEBUG) &debug_messenger_create_info else null,
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = @intCast(u32, all_required_exts.len),
            .ppEnabledExtensionNames = @ptrCast([*c]const [*c]const u8, all_required_exts.ptr),
            .enabledLayerCount = validation_layers.len,
            .ppEnabledLayerNames = @ptrCast([*c]const [*c]const u8, validation_layers.ptr),
            .flags = 0,
        };

        const result = c.vkCreateInstance(&instance_create_info, null, &instance);
        if (result != c.VK_SUCCESS) {
            print("failed to create instance: ");
            printn(result);
            return;
        }
    }
    defer c.vkDestroyInstance(instance, null);

    // Debug messenger
    var debug_messenger: c.VkDebugUtilsMessengerEXT = null;
    if (comptime DEBUG) {
        const proc_addr = c.vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        const vkCreateDebugUtilsMessengerExt = @ptrCast(c.PFN_vkCreateDebugUtilsMessengerEXT, proc_addr) orelse {
            printn("Missing debug utils");
            return;
        };

        const debug_messenger_create_info = vk.makeDebugMessengerCreateInfo();
        const result = vkCreateDebugUtilsMessengerExt(instance, &debug_messenger_create_info, null, &debug_messenger);
        if (result != c.VK_SUCCESS) {
            printn("Failed to create debug messenger");
            return;
        }
    }
    defer {
        if (comptime DEBUG) {
            const proc_addr = c.vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            const vkDestroyDebugUtilsMessengerExt = @ptrCast(c.PFN_vkDestroyDebugUtilsMessengerEXT, proc_addr) orelse @panic("Missing vkDestroyDebugUtilsMessengerEXT");
            vkDestroyDebugUtilsMessengerExt(instance, debug_messenger, null);
        }
    }

    // Surface
    var surface: c.VkSurfaceKHR = null;
    {
        const result = c.glfwCreateWindowSurface(instance, window, null, &surface);
        if (result != c.VK_SUCCESS) {
            var msg: [*c]const u8 = null;
            _ = c.glfwGetError(&msg);
            printn("Failed to create window surface:");
            printn(msg);
            return;
        }
    }
    defer c.vkDestroySurfaceKHR(instance, surface, null);

    // Physical Devices
    const required_device_extensions: [1][*:0]const u8 = .{c.VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    var physical_device: c.VkPhysicalDevice = null;
    var graphics_family_index: u32 = undefined;
    var presentation_family_index: u32 = undefined;
    var surface_formats: []c.VkSurfaceFormatKHR = undefined;
    var present_modes: []c.VkPresentModeKHR = undefined;
    {
        var physical_device_count: u32 = 0;
        if (c.vkEnumeratePhysicalDevices(instance, &physical_device_count, null) != c.VK_SUCCESS) {
            printn("Failed to enumerate physical devices");
            return;
        }

        var devices = allocator.alloc(c.VkPhysicalDevice, physical_device_count) catch @panic("oom");
        defer allocator.free(devices);

        if (c.vkEnumeratePhysicalDevices(instance, &physical_device_count, devices.ptr) != c.VK_SUCCESS) {
            printn("Failed to enumerate physical devices");
            return;
        }

        for (devices) |device| {
            var device_properties: c.VkPhysicalDeviceProperties = undefined;
            c.vkGetPhysicalDeviceProperties(device, &device_properties);

            // 1. Check if required device extensions are present
            var device_extension_count: u32 = 0;
            if (c.vkEnumerateDeviceExtensionProperties(device, null, &device_extension_count, null) != c.VK_SUCCESS) {
                printn("Failed to enumerate device extension properties");
                continue;
            }
            var device_extension_properties = allocator.alloc(c.VkExtensionProperties, device_extension_count) catch @panic("oom");
            defer allocator.free(device_extension_properties);
            if (c.vkEnumerateDeviceExtensionProperties(device, null, &device_extension_count, device_extension_properties.ptr) != c.VK_SUCCESS) {
                printn("Failed to enumerate device extension properties");
                continue;
            }

            const has_required_extensions = for (required_device_extensions) |required_ext_name| {
                const has_ext = for (device_extension_properties) |properties| {
                    const extension_name = @ptrCast([*:0]const u8, &properties.extensionName);
                    if (std.cstr.cmp(required_ext_name, extension_name) == 0) {
                        break true;
                    }
                } else false;
                if (!has_ext) break false;
            } else true;

            if (!has_required_extensions) {
                continue;
            }

            // 2. Swap Chain Support Details
            var surface_format_count: u32 = 0;
            var present_mode_count: u32 = 0;
            // Check for surface formats available
            if (c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count, null) != c.VK_SUCCESS) {
                printn("Failed for get physical device surface capabilities");
                continue;
            } else if (surface_format_count == 0) {
                printn("skipping device due to no surface format available");
                continue;
            }

            // Check for present modes available
            if (c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, null) != c.VK_SUCCESS) {
                printn("Failed vkGetPhysicalDeviceSurfacePresentModesKHR");
                continue;
            } else if (present_mode_count == 0) {
                printn("skipping device due to no present mode");
                continue;
            }

            // 3. Find the queue families with graphics & present capabilities
            var queue_family_count: u32 = 0;
            c.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, null);
            var queue_family_properties = allocator.alloc(c.VkQueueFamilyProperties, queue_family_count) catch @panic("oom");
            defer allocator.free(queue_family_properties);
            c.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties.ptr);

            var graphics_family_index_opt: ?u32 = null;
            var presentation_family_index_opt: ?u32 = null;
            for (queue_family_properties) |properties, i| {
                const index = @intCast(u32, i);
                if (0 != (properties.queueFlags & c.VK_QUEUE_GRAPHICS_BIT)) {
                    graphics_family_index_opt = index;
                }

                var surface_supported: c.VkBool32 = c.VK_FALSE;
                _ = c.vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &surface_supported);
                if (surface_supported == c.VK_TRUE) {
                    presentation_family_index_opt = index;
                }
                // TODO prefer queue families that have support for both if that is better for performance
                // Simple method is keep looking if they're different and break if they're the same
            }

            if (graphics_family_index_opt == null or presentation_family_index_opt == null) {
                continue;
            }

            // By now we have a valid device so let's choose it. In the future we can present
            // multiple options to the user.
            print("selecting: ");
            printn(device_properties.deviceName);
            physical_device = device;
            graphics_family_index = graphics_family_index_opt orelse unreachable;
            presentation_family_index = presentation_family_index_opt orelse unreachable;

            surface_formats = allocator.alloc(c.VkSurfaceFormatKHR, surface_format_count) catch @panic("oom");
            _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count, surface_formats.ptr);

            present_modes = allocator.alloc(c.VkPresentModeKHR, present_mode_count) catch @panic("oom");
            _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes.ptr);
            break;
        }

        if (physical_device == null) {
            printn("Failed to find suitable vulkan physical device");
            return;
        }
    }
    defer allocator.free(surface_formats);
    defer allocator.free(present_modes);

    const multi_queue = graphics_family_index != presentation_family_index;

    // Logical Device
    var logical_device: c.VkDevice = null;
    {
        const priorities: f32 = 1.0;

        const device_queue_create_infos: []c.VkDeviceQueueCreateInfo =
            if (multi_queue)
            &[_]c.VkDeviceQueueCreateInfo{
                .{
                    .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .queueFamilyIndex = graphics_family_index,
                    .queueCount = 1,
                    .pQueuePriorities = &priorities,
                },
                .{
                    .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .queueFamilyIndex = presentation_family_index,
                    .queueCount = 1,
                    .pQueuePriorities = &priorities,
                },
            }
        else
            &[_]c.VkDeviceQueueCreateInfo{
                .{
                    .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .queueFamilyIndex = graphics_family_index,
                    .queueCount = 1,
                    .pQueuePriorities = &priorities,
                },
            };

        const device_create_info = c.VkDeviceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .queueCreateInfoCount = @intCast(u32, device_queue_create_infos.len),
            .pQueueCreateInfos = device_queue_create_infos.ptr,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = null,
            .enabledExtensionCount = required_device_extensions.len,
            .ppEnabledExtensionNames = &required_device_extensions,
            .pEnabledFeatures = null,
        };

        const result = c.vkCreateDevice(physical_device, &device_create_info, null, &logical_device);
        if (result != c.VK_SUCCESS) {
            printn("Failed to create logical device");
            return;
        }
    }
    defer c.vkDestroyDevice(logical_device, null);

    // Graphics Queue
    var graphics_queue: c.VkQueue = null;
    {
        c.vkGetDeviceQueue(logical_device, graphics_family_index, 0, &graphics_queue);
    }

    // Presentation queue
    var present_queue: c.VkQueue = null;
    {
        c.vkGetDeviceQueue(logical_device, presentation_family_index, 0, &present_queue);
    }

    var command_pool: c.VkCommandPool = null;
    {
        const command_pool_create_info = c.VkCommandPoolCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = null,
            .flags = c.VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics_family_index,
        };
        if (c.vkCreateCommandPool(logical_device, &command_pool_create_info, null, &command_pool) != c.VK_SUCCESS) {
            printn("Failed to create command pool");
            return;
        }
    }
    defer c.vkDestroyCommandPool(logical_device, command_pool, null);

    // Vertex Buffer Creation
    const vertex_data_size: c.VkDeviceSize = @sizeOf(@TypeOf(alphabet_vertices));
    const vertex_buffer = try vk.emake_buffer(
        physical_device,
        logical_device,
        vertex_data_size,
        c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | c.VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    );
    defer {
        vk.destroy_buffer(logical_device, vertex_buffer);
    }

    // Fill vertex buffer using staging buffer
    {
        const staging_buffer = try vk.emake_buffer(
            physical_device,
            logical_device,
            vertex_data_size,
            c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        );
        defer {
            vk.destroy_buffer(logical_device, staging_buffer);
        }

        // Map memory
        var mapped_memory_addr: *@TypeOf(alphabet_vertices) = undefined;
        if (c.vkMapMemory(
            logical_device,
            staging_buffer.memory,
            0,
            vertex_data_size,
            0,
            @ptrCast([*c]?*anyopaque, &mapped_memory_addr),
        ) != c.VK_SUCCESS) {
            printn("Failed to map memory");
            return;
        }
        // NOTE: could memcpy
        for (alphabet_vertices) |vertex, i| {
            mapped_memory_addr.*[i] = vertex;
        }
        c.vkUnmapMemory(logical_device, staging_buffer.memory);

        // Perform transfer from staging buffer to vertex buffer
        {
            var transfer_command_buffer = vk.emake_oneTimeCommandBuffer(logical_device, command_pool) catch {
                printn("Failed to begin command buffer");
                return;
            };

            const copy_region = c.VkBufferCopy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = vertex_data_size,
            };

            c.vkCmdCopyBuffer(
                transfer_command_buffer,
                staging_buffer.vk_buffer,
                vertex_buffer.vk_buffer,
                1,
                &copy_region,
            );

            vk.edestroy_oneTimeCommandBuffer(
                logical_device,
                transfer_command_buffer,
                command_pool,
                graphics_queue,
            ) catch {
                printn("Failed to end command buffer");
                return;
            };
        }
    }

    // Index Buffer begin TODO staging buffer transfer -> device local can be subroutine
    const index_data_size: c.VkDeviceSize = @sizeOf(@TypeOf(indices));

    const index_buffer = try vk.emake_buffer(
        physical_device,
        logical_device,
        index_data_size,
        c.VK_BUFFER_USAGE_INDEX_BUFFER_BIT | c.VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    );
    defer {
        vk.destroy_buffer(logical_device, index_buffer);
    }

    // Fill index buffer using staging buffer
    {
        const staging_buffer = try vk.emake_buffer(
            physical_device,
            logical_device,
            index_data_size,
            c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        );
        defer {
            vk.destroy_buffer(logical_device, staging_buffer);
        }

        // Map memory
        var mapped_memory_addr: *@TypeOf(indices) = undefined;
        if (c.vkMapMemory(
            logical_device,
            staging_buffer.memory,
            0,
            index_data_size,
            0,
            @ptrCast([*c]?*anyopaque, &mapped_memory_addr),
        ) != c.VK_SUCCESS) {
            printn("Failed to map memory");
            return;
        }
        // NOTE: could memcpy
        for (indices) |index, i| {
            mapped_memory_addr.*[i] = index;
        }
        c.vkUnmapMemory(logical_device, staging_buffer.memory);

        {
            var transfer_command_buffer = vk.emake_oneTimeCommandBuffer(logical_device, command_pool) catch {
                printn("Failed to begin command buffer");
                return;
            };
            const copy_region = c.VkBufferCopy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = index_data_size,
            };

            c.vkCmdCopyBuffer(
                transfer_command_buffer,
                staging_buffer.vk_buffer,
                index_buffer.vk_buffer,
                1,
                &copy_region,
            );

            vk.edestroy_oneTimeCommandBuffer(
                logical_device,
                transfer_command_buffer,
                command_pool,
                graphics_queue,
            ) catch {
                printn("Failed to end command buffer");
                return;
            };
        }
    }

    var texture_image: vk.Image = undefined;
    {
        texture_image = vk.emake_image(
            logical_device,
            physical_device,
            graphics_family_index,
            c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            texture_width,
            texture_height,
        ) catch {
            printn("Failed to create image for texture");
            return;
        };
    }
    defer {
        vk.destroy_image(logical_device, texture_image);
    }

    // Transfer texture pixels into the created image:
    {
        // Transition layout to transfer dst optimal:
        // TODO could make this a function instead of duplicating it!
        const command_buffer = vk.emake_oneTimeCommandBuffer(logical_device, command_pool) catch {
            printn("Failed to create transfer command buffer");
            return;
        };

        const barrier = c.VkImageMemoryBarrier{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = null,
            .srcAccessMask = 0, // no happens-before operations
            .dstAccessMask = c.VK_ACCESS_TRANSFER_WRITE_BIT, // transfer writes must happen-after transition
            .oldLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = c.VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = c.VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = c.VK_QUEUE_FAMILY_IGNORED,
            .image = texture_image.vk_image,
            .subresourceRange = c.VkImageSubresourceRange{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        c.vkCmdPipelineBarrier(
            command_buffer,
            c.VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            c.VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, // dependency flags
            0, // global memory barrier count
            null, // global memory barriers
            0, // buffer memory barrier count
            null, // buffer memory barriers
            1,
            &barrier,
        );

        vk.edestroy_oneTimeCommandBuffer(logical_device, command_buffer, command_pool, graphics_queue) catch {
            printn("Failed to end one time command buffer");
            return;
        };
    }
    {
        // Fill in image memory first using a staging buffer filled with pixel data:
        const pixel_count = texture_width * texture_height;
        const texture_size = @sizeOf(f32) * pixel_count;
        printn(texture_size);
        const staging_buffer = try vk.emake_buffer(
            physical_device,
            logical_device,
            texture_size,
            c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        );
        defer {
            vk.destroy_buffer(logical_device, staging_buffer);
        }

        var mapped_memory_addr: *[pixel_count]f32 = undefined;
        if (c.vkMapMemory(
            logical_device,
            staging_buffer.memory,
            0,
            texture_size,
            0,
            @ptrCast([*c]?*anyopaque, &mapped_memory_addr),
        ) != c.VK_SUCCESS) {
            printn("Failed to map memory");
            return;
        }

        for (texture_pixels_n) |pixel, i| {
            mapped_memory_addr.*[i] = pixel;
        }
        c.vkUnmapMemory(logical_device, staging_buffer.memory);

        // Record command buffer
        const command_buffer = vk.emake_oneTimeCommandBuffer(logical_device, command_pool) catch {
            printn("Failed to create transfer command buffer");
            return;
        };

        const region = c.VkBufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0, // 0 means use the image extent for addressing into the buffer
            .bufferImageHeight = 0, // ditto
            .imageSubresource = c.VkImageSubresourceLayers{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = c.VkOffset3D{
                .x = 0,
                .y = 0,
                .z = 0,
            },
            .imageExtent = c.VkExtent3D{
                .width = texture_width,
                .height = texture_height,
                .depth = 1,
            },
        };
        c.vkCmdCopyBufferToImage(
            command_buffer,
            staging_buffer.vk_buffer,
            texture_image.vk_image,
            c.VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region,
        );

        vk.edestroy_oneTimeCommandBuffer(logical_device, command_buffer, command_pool, graphics_queue) catch {
            printn("Failed to end one time command buffer");
            return;
        };
    }
    {
        // layout transition to shader optimal
        const command_buffer = vk.emake_oneTimeCommandBuffer(logical_device, command_pool) catch {
            printn("Failed to create transfer command buffer");
            return;
        };

        const barrier = c.VkImageMemoryBarrier{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = null,
            .srcAccessMask = c.VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = c.VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = c.VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = c.VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = c.VK_QUEUE_FAMILY_IGNORED,
            .image = texture_image.vk_image,
            .subresourceRange = c.VkImageSubresourceRange{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        c.vkCmdPipelineBarrier(
            command_buffer,
            c.VK_PIPELINE_STAGE_TRANSFER_BIT,
            c.VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, // dependency flags
            0, // global memory barrier count
            null, // global memory barriers
            0, // buffer memory barrier count
            null, // buffer memory barriers
            1,
            &barrier,
        );

        vk.edestroy_oneTimeCommandBuffer(logical_device, command_buffer, command_pool, graphics_queue) catch {
            printn("Failed to end one time command buffer");
            return;
        };
    }

    // Texture Image View
    var texture_image_view: c.VkImageView = undefined;
    {
        const create_info = c.VkImageViewCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .image = texture_image.vk_image,
            .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
            .format = c.VK_FORMAT_R32_SFLOAT,
            .components = c.VkComponentMapping{
                .r = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = c.VkImageSubresourceRange{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        if (c.vkCreateImageView(logical_device, &create_info, null, &texture_image_view) != c.VK_SUCCESS) {
            print("Failed to create texture image view");
            return;
        }
    }
    defer {
        c.vkDestroyImageView(logical_device, texture_image_view, null);
    }

    // Image Sampler
    var texture_sampler: c.VkSampler = undefined;
    {
        const create_info = c.VkSamplerCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .magFilter = c.VK_FILTER_NEAREST,
            .minFilter = c.VK_FILTER_NEAREST,
            .mipmapMode = c.VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0,
            .anisotropyEnable = c.VK_FALSE,
            .maxAnisotropy = 1.0,
            .compareEnable = c.VK_FALSE,
            .compareOp = c.VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0,
            .maxLod = 0.0,
            .borderColor = c.VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = c.VK_FALSE,
        };
        if (c.vkCreateSampler(logical_device, &create_info, null, &texture_sampler) != c.VK_SUCCESS) {
            printn("Failed to create image sampler");
            return;
        }
    }
    defer c.vkDestroySampler(logical_device, texture_sampler, null);

    // Descriptor Set Layout
    var descriptor_set_layout: c.VkDescriptorSetLayout = null;
    {
        const layout_bindings = [_]c.VkDescriptorSetLayoutBinding{
            .{
                .binding = 0,
                .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = c.VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = null,
            },
            // TODO correct this
            .{
                .binding = 1,
                .descriptorType = c.VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = c.VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = null,
            },
        };

        const create_info = c.VkDescriptorSetLayoutCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .bindingCount = layout_bindings.len,
            .pBindings = &layout_bindings,
        };
        if (c.vkCreateDescriptorSetLayout(logical_device, &create_info, null, &descriptor_set_layout) != c.VK_SUCCESS) {
            printn("Descriptor Set layout failed");
            return;
        }
    }
    defer c.vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout, null);

    const uniform_size: c.VkDeviceSize = @sizeOf(UniformTransform);
    const uniform_buffer = try vk.emake_buffer(
        physical_device,
        logical_device,
        uniform_size,
        c.VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    );
    defer {
        vk.destroy_buffer(logical_device, uniform_buffer);
    }

    // Map memory:
    var mapped_uniform_memory_addr: *UniformTransform = undefined;
    {
        if (c.vkMapMemory(
            logical_device,
            uniform_buffer.memory,
            0,
            uniform_size,
            0,
            @ptrCast([*c]?*anyopaque, &mapped_uniform_memory_addr),
        ) != c.VK_SUCCESS) {
            printn("Failed to map uniform memory");
            return;
        }
    }

    // Descriptor Pool & sets
    var descriptor_pool: c.VkDescriptorPool = undefined;
    {
        const pool_sizes = [_]c.VkDescriptorPoolSize{
            .{
                .type = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            .{
                .type = c.VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
            },
        };
        const create_info = c.VkDescriptorPoolCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .maxSets = 1,
            .poolSizeCount = pool_sizes.len,
            .pPoolSizes = &pool_sizes,
        };
        if (c.vkCreateDescriptorPool(logical_device, &create_info, null, &descriptor_pool) != c.VK_SUCCESS) {
            printn("Descriptor pool creation failed");
            return;
        }
    }
    defer c.vkDestroyDescriptorPool(logical_device, descriptor_pool, null);

    var descriptor_set: c.VkDescriptorSet = undefined;
    {
        const allocate_info = c.VkDescriptorSetAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = null,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptor_set_layout,
        };
        if (c.vkAllocateDescriptorSets(logical_device, &allocate_info, &descriptor_set) != c.VK_SUCCESS) {
            printn("Allocate descriptor set failed");
            return;
        }
    }

    // Update Descriptor Set
    {
        const buffer_info = c.VkDescriptorBufferInfo{
            .buffer = uniform_buffer.vk_buffer,
            .offset = 0,
            .range = uniform_size,
        };
        const image_info = c.VkDescriptorImageInfo{
            .sampler = texture_sampler,
            .imageView = texture_image_view,
            .imageLayout = c.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        const descriptor_writes = [_]c.VkWriteDescriptorSet{
            .{
                .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = null,
                .dstSet = descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = null,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = null,
            },
            .{
                .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = null,
                .dstSet = descriptor_set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = c.VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info,
                .pBufferInfo = null,
                .pTexelBufferView = null,
            },
        };
        c.vkUpdateDescriptorSets(logical_device, descriptor_writes.len, &descriptor_writes, 0, null);
    }

    // Pick surface format TODO does this need to be re-done when swapchain is recreated?
    const surface_format = for (surface_formats) |surface_format| {
        if (surface_format.format == c.VK_FORMAT_B8G8R8A8_SRGB and surface_format.colorSpace == c.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            break surface_format;
        }
    } else surface_formats[0];

    // Pick present mode
    const present_mode = for (present_modes) |present_mode| {
        if (present_mode == c.VK_PRESENT_MODE_MAILBOX_KHR) {
            break present_mode;
        }
    } else c.VK_PRESENT_MODE_FIFO_KHR;

    // Draw command buffer
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
            printn("Failed to allocate command buffer");
            return;
        }
    }

    // Synchronization primitives
    var image_acquired_semaphore: c.VkSemaphore = null;
    {
        const semaphore_create_info = c.VkSemaphoreCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
        };
        if (c.vkCreateSemaphore(logical_device, &semaphore_create_info, null, &image_acquired_semaphore) != c.VK_SUCCESS) {
            printn("Failed to create semaphore");
            return;
        }
    }
    defer c.vkDestroySemaphore(logical_device, image_acquired_semaphore, null);

    var rendering_finished_semaphore: c.VkSemaphore = null;
    {
        const semaphore_create_info = c.VkSemaphoreCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
        };
        if (c.vkCreateSemaphore(logical_device, &semaphore_create_info, null, &rendering_finished_semaphore) != c.VK_SUCCESS) {
            printn("Failed to create semaphore");
            return;
        }
    }
    defer c.vkDestroySemaphore(logical_device, rendering_finished_semaphore, null);

    var command_buffer_fence: c.VkFence = null;
    {
        const fence_create_info = c.VkFenceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = null,
            .flags = c.VK_FENCE_CREATE_SIGNALED_BIT,
        };

        if (c.vkCreateFence(logical_device, &fence_create_info, null, &command_buffer_fence) != c.VK_SUCCESS) {
            printn("Failed to create fence");
            return;
        }
    }
    defer c.vkDestroyFence(logical_device, command_buffer_fence, null);

    // TODO handle minimization when framebuffer size is 0
    while (true) {

        // TODO there is LOTS we are doing here (at least command buffer/command pool stuff) that doesn't
        // depend on the swapchain that we don't need to redo when we resize the game. We should pull that
        // stuff out of this loop.

        // TODO do we need to do this with swapchain recreation?
        var surface_capabilities: c.VkSurfaceCapabilitiesKHR = undefined;
        if (c.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities) != c.VK_SUCCESS) {
            printn("Failed to get physical surface capabilities");
            return;
        }

        // Pick Swap Extent
        const swap_extent = blk: {
            if (surface_capabilities.currentExtent.width == U32MAX) {
                // From the spec: Surface size will be determined by the swapchain, so
                // we need to find a surface size that is appropriate.
                // Use glfw to get the pixel resolution of the window and clamp it by the
                // min/max extent:
                var buffer_width: i32 = undefined;
                var buffer_height: i32 = undefined;
                c.glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
                // TODO move this out of the u32max case above, since we need to check if it's 0 for minimization
                // handling, (if we need to do anything special with minimization)

                var extent_width = std.math.clamp(
                    @intCast(u32, buffer_width),
                    surface_capabilities.minImageExtent.width,
                    surface_capabilities.maxImageExtent.width,
                );
                var extent_height = std.math.clamp(
                    @intCast(u32, buffer_height),
                    surface_capabilities.minImageExtent.height,
                    surface_capabilities.maxImageExtent.height,
                );
                break :blk c.VkExtent2D{ .width = extent_width, .height = extent_height };
            } else {
                // Otherwise, we can use the size of the surface specified in currentExt
                break :blk surface_capabilities.currentExtent;
            }
        };

        // Create Swapchain
        var swap_chain: c.VkSwapchainKHR = null;
        {
            const max_image_count = surface_capabilities.maxImageCount;
            var image_count = surface_capabilities.minImageCount + 1;
            if (max_image_count > 0 and max_image_count < image_count) {
                image_count = max_image_count;
            }

            const queue_family_indices = [_]u32{ graphics_family_index, presentation_family_index };
            const create_info = c.VkSwapchainCreateInfoKHR{
                .sType = c.VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = null,
                .flags = 0,
                .surface = surface,
                .minImageCount = image_count,
                .imageFormat = surface_format.format,
                .imageColorSpace = surface_format.colorSpace,
                .imageExtent = swap_extent,
                .imageArrayLayers = 1,
                .imageUsage = c.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = if (multi_queue) c.VK_SHARING_MODE_CONCURRENT else c.VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = if (multi_queue) queue_family_indices.len else 0,
                .pQueueFamilyIndices = if (multi_queue) &queue_family_indices else null,
                .preTransform = surface_capabilities.currentTransform,
                .compositeAlpha = c.VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = present_mode,
                .clipped = c.VK_TRUE,
                .oldSwapchain = null,
            };
            if (c.vkCreateSwapchainKHR(logical_device, &create_info, null, &swap_chain) != c.VK_SUCCESS) {
                printn("Failed to create swapchain");
                return;
            }
        }
        defer c.vkDestroySwapchainKHR(logical_device, swap_chain, null);

        // Swapchain images & image views

        // Allocate mem for images & fill array
        var swap_chain_images: []c.VkImage = undefined;
        {
            var swap_chain_image_count: u32 = 0;
            _ = c.vkGetSwapchainImagesKHR(logical_device, swap_chain, &swap_chain_image_count, null);

            // TODO instead of panicking, we can handle this error and quit gracefully with this setup
            swap_chain_images = allocator.alloc(c.VkImage, swap_chain_image_count) catch @panic("oom");

            if (c.vkGetSwapchainImagesKHR(logical_device, swap_chain, &swap_chain_image_count, swap_chain_images.ptr) != c.VK_SUCCESS) {
                print("Failed to retrieve swap chain images");
                return;
            }
        }
        defer allocator.free(swap_chain_images);

        // Allocate mem for image views
        var image_views: []c.VkImageView = undefined;
        {
            image_views = allocator.alloc(c.VkImageView, swap_chain_images.len) catch @panic("oom");
        }
        defer allocator.free(image_views);

        // Create image views
        {
            for (swap_chain_images) |image, i| {
                // TODO create image view could be function instead of duplicated code
                const create_info = c.VkImageViewCreateInfo{
                    .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .image = image,
                    .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
                    .format = surface_format.format,
                    .components = c.VkComponentMapping{
                        .r = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange = c.VkImageSubresourceRange{
                        .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };
                if (c.vkCreateImageView(logical_device, &create_info, null, &image_views[i]) != c.VK_SUCCESS) {
                    print("Failed to create image view");
                    var j: usize = 0;
                    while (j < i) : (j += 1) {
                        c.vkDestroyImageView(logical_device, image_views[j], null);
                    }
                    return;
                }
            }
        }
        defer {
            for (image_views) |image_view| {
                c.vkDestroyImageView(logical_device, image_view, null);
            }
        }

        // Shaders TODO should create a scope for these as they're not needed for the duration of the other
        // resources at this level.
        var vertex_shader = vk.createShaderModule(logical_device, "vert.spv");
        if (vertex_shader == null) {
            printn("Failed to create vertex shader");
            return;
        }
        defer c.vkDestroyShaderModule(logical_device, vertex_shader, null);

        var fragment_shader = vk.createShaderModule(logical_device, "frag.spv");
        if (fragment_shader == null) {
            printn("Failed to create frag shader");
            return;
        }
        defer c.vkDestroyShaderModule(logical_device, fragment_shader, null);

        // Render Pass
        var render_pass: c.VkRenderPass = null;
        {
            const attachment = c.VkAttachmentDescription{
                .flags = 0,
                .format = surface_format.format,
                .samples = c.VK_SAMPLE_COUNT_1_BIT,
                .loadOp = c.VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = c.VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = c.VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = c.VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = c.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };

            const attachment_ref = c.VkAttachmentReference{
                .attachment = 0,
                .layout = c.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };

            const subpass = c.VkSubpassDescription{
                .flags = 0,
                .pipelineBindPoint = c.VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = null,
                .colorAttachmentCount = 1,
                .pColorAttachments = &attachment_ref,
                .pResolveAttachments = null,
                .pDepthStencilAttachment = null,
                .preserveAttachmentCount = 0,
                .pPreserveAttachments = null,
            };

            // TODO figure out subpass dependencies

            const render_pass_create_info = c.VkRenderPassCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .attachmentCount = 1,
                .pAttachments = &attachment,
                .subpassCount = 1,
                .pSubpasses = &subpass,
                .dependencyCount = 0,
                .pDependencies = null,
            };
            if (c.vkCreateRenderPass(logical_device, &render_pass_create_info, null, &render_pass) != c.VK_SUCCESS) {
                printn("Failed to create render pass");
                return;
            }
        }
        defer c.vkDestroyRenderPass(logical_device, render_pass, null);

        // Pipeline layout
        var pipeline_layout: c.VkPipelineLayout = null;
        {
            const push_constant_range = c.VkPushConstantRange{
                .stageFlags = c.VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = @sizeOf([4]f32),
            };
            const pipeline_layout_create_info = c.VkPipelineLayoutCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .setLayoutCount = 1,
                .pSetLayouts = &descriptor_set_layout,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &push_constant_range,
            };
            if (c.vkCreatePipelineLayout(logical_device, &pipeline_layout_create_info, null, &pipeline_layout) != c.VK_SUCCESS) {
                printn("Failed to create pipeline layout");
                return;
            }
        }
        defer c.vkDestroyPipelineLayout(logical_device, pipeline_layout, null);

        // Pipeline
        var graphics_pipeline: c.VkPipeline = null;
        {
            // Shader Stages
            const shader_stage_create_infos = [_]c.VkPipelineShaderStageCreateInfo{
                .{
                    .sType = c.VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .stage = c.VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertex_shader,
                    .pName = "main",
                    .pSpecializationInfo = null,
                },
                .{
                    .sType = c.VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .stage = c.VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragment_shader,
                    .pName = "main",
                    .pSpecializationInfo = null,
                },
            };

            // Vertex input
            const vertex_input_binding_description = c.VkVertexInputBindingDescription{
                .binding = 0,
                .stride = @sizeOf(Vertex),
                .inputRate = c.VK_VERTEX_INPUT_RATE_VERTEX,
            };

            const vertex_attribute_binding_description = [_]c.VkVertexInputAttributeDescription{
                .{
                    .location = 0,
                    .binding = 0,
                    .format = c.VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = @offsetOf(Vertex, "position"),
                },
                .{
                    .location = 1,
                    .binding = 0,
                    .format = c.VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = @offsetOf(Vertex, "colour"),
                },
                .{
                    .location = 2,
                    .binding = 0,
                    .format = c.VK_FORMAT_R32G32_SFLOAT,
                    .offset = @offsetOf(Vertex, "texture_position"),
                },
            };

            const vertex_input_state_create_info = c.VkPipelineVertexInputStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &vertex_input_binding_description,
                .vertexAttributeDescriptionCount = vertex_attribute_binding_description.len,
                .pVertexAttributeDescriptions = &vertex_attribute_binding_description,
            };

            // Input assembly
            const input_assembly_state_create_info = c.VkPipelineInputAssemblyStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .topology = c.VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = c.VK_FALSE,
            };

            // Viewport
            const scissor_rect = c.VkRect2D{
                .offset = .{ .x = 0, .y = 0 },
                .extent = swap_extent,
            };

            const viewport = c.VkViewport{
                .x = 0,
                .y = 0,
                .width = @intToFloat(f32, swap_extent.width),
                .height = @intToFloat(f32, swap_extent.height),
                .minDepth = 0.0,
                .maxDepth = 1.0,
            };
            const viewport_state_create_info = c.VkPipelineViewportStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor_rect,
            };

            const rasterization_state_create_info = c.VkPipelineRasterizationStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .depthClampEnable = c.VK_FALSE,
                .rasterizerDiscardEnable = c.VK_FALSE,
                .polygonMode = c.VK_POLYGON_MODE_FILL,
                .cullMode = c.VK_CULL_MODE_BACK_BIT,
                .frontFace = c.VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = c.VK_FALSE,
                .depthBiasConstantFactor = 0.0,
                .depthBiasClamp = 0.0,
                .depthBiasSlopeFactor = 0.0,
                .lineWidth = 1.0,
            };

            const multisample_state_create_info = c.VkPipelineMultisampleStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .rasterizationSamples = c.VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = c.VK_FALSE,
                .minSampleShading = 1.0,
                .pSampleMask = null,
                .alphaToCoverageEnable = c.VK_FALSE,
                .alphaToOneEnable = c.VK_FALSE,
            };

            // Colour Blending
            const colour_blend_attachment_state = c.VkPipelineColorBlendAttachmentState{
                .blendEnable = c.VK_FALSE,
                .srcColorBlendFactor = c.VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = c.VK_BLEND_FACTOR_ONE,
                .colorBlendOp = c.VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = c.VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = c.VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = c.VK_BLEND_OP_ADD,
                .colorWriteMask = c.VK_COLOR_COMPONENT_R_BIT | c.VK_COLOR_COMPONENT_G_BIT | c.VK_COLOR_COMPONENT_B_BIT | c.VK_COLOR_COMPONENT_A_BIT,
            };

            const colour_blend_state_create_info = c.VkPipelineColorBlendStateCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .logicOpEnable = c.VK_FALSE,
                .logicOp = c.VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &colour_blend_attachment_state,
                .blendConstants = [_]f32{0.0} ** 4,
            };

            const pipeline_create_info = c.VkGraphicsPipelineCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = null,
                .flags = 0,
                .stageCount = 2,
                .pStages = &shader_stage_create_infos,
                .pVertexInputState = &vertex_input_state_create_info,
                .pInputAssemblyState = &input_assembly_state_create_info,
                .pTessellationState = null,
                .pViewportState = &viewport_state_create_info,
                .pRasterizationState = &rasterization_state_create_info,
                .pMultisampleState = &multisample_state_create_info,
                .pDepthStencilState = null,
                .pColorBlendState = &colour_blend_state_create_info,
                .pDynamicState = null,
                .layout = pipeline_layout,
                .renderPass = render_pass,
                .subpass = 0,
                .basePipelineHandle = null,
                .basePipelineIndex = -1,
            };

            if (c.vkCreateGraphicsPipelines(logical_device, null, 1, &pipeline_create_info, null, &graphics_pipeline) != c.VK_SUCCESS) {
                printn("Failed to create graphics pipeline");
                return;
            }
        }
        defer c.vkDestroyPipeline(logical_device, graphics_pipeline, null);

        // Framebuffers
        var framebuffers: []c.VkFramebuffer = undefined;
        {
            var framebuffer_count: usize = 0;
            var framebuffers_mem = allocator.alloc(c.VkFramebuffer, image_views.len) catch @panic("oom");

            for (image_views) |image_view, i| {
                const framebuffer_create_info = c.VkFramebufferCreateInfo{
                    .sType = c.VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext = null,
                    .flags = 0,
                    .renderPass = render_pass,
                    .attachmentCount = 1,
                    .pAttachments = &image_view,
                    .width = swap_extent.width,
                    .height = swap_extent.height,
                    .layers = 1,
                };
                if (c.vkCreateFramebuffer(logical_device, &framebuffer_create_info, null, &framebuffers_mem[i]) != c.VK_SUCCESS) {
                    printn("Failed to create framebuffer");
                    break;
                }
                framebuffer_count = framebuffer_count + 1;
            }

            if (framebuffer_count < image_views.len) {
                for (framebuffers_mem) |framebuffer, i| {
                    if (i < framebuffer_count) {
                        c.vkDestroyFramebuffer(logical_device, framebuffer, null);
                    } else {
                        break;
                    }
                }
                allocator.free(framebuffers_mem);
                return;
            } else {
                // Pass ownership outside of this block by setting framebuffers
                framebuffers = framebuffers_mem;
            }
        }
        defer {
            for (framebuffers) |framebuffer| {
                c.vkDestroyFramebuffer(logical_device, framebuffer, null);
            }
            allocator.free(framebuffers);
        }

        // TODO PREV COMMAND POOL LOCATION

        // Render Loop
        const close = while (c.glfwWindowShouldClose(window) == c.GLFW_FALSE) {
            loop_count = loop_count + 1;
            c.glfwPollEvents();

            // Draw
            {
                if (c.vkWaitForFences(logical_device, 1, &command_buffer_fence, c.VK_TRUE, U64MAX) != c.VK_SUCCESS) {
                    printn("Failed for wait for fence");
                    return;
                }

                // Acquire image
                var image_index: u32 = undefined;
                const acquire_result = c.vkAcquireNextImageKHR(logical_device, swap_chain, U64MAX, image_acquired_semaphore, null, &image_index);
                if (acquire_result == c.VK_ERROR_OUT_OF_DATE_KHR) {
                    break false;
                } else if (acquire_result != c.VK_SUCCESS and acquire_result != c.VK_SUBOPTIMAL_KHR) {
                    printn("Failed to acquire image");
                    return;
                }

                if (c.vkResetCommandBuffer(command_buffer, 0) != c.VK_SUCCESS) {
                    printn("Failed to reset command buffer");
                    return;
                }

                // Update Uniform
                {
                    //const period = 9999;
                    //const scale: f32 = 1.0 * (@intToFloat(f32, loop_count % period) / period);
                    const uniform_transform = UniformTransform{ .scale = [_]f32{ glyph_scale, glyph_scale } };
                    mapped_uniform_memory_addr.* = uniform_transform;
                }

                // Draw Commands
                const result = recordCommandBuffer(
                    command_buffer,
                    framebuffers[image_index],
                    render_pass,
                    swap_extent,
                    graphics_pipeline,
                    vertex_buffer.vk_buffer,
                    index_buffer.vk_buffer,
                    descriptor_set,
                    pipeline_layout,
                );
                if (!result) {
                    return;
                }

                const wait_stage: c.VkPipelineStageFlags = c.VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                const submit_info = c.VkSubmitInfo{
                    .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .pNext = null,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &image_acquired_semaphore,
                    .pWaitDstStageMask = &wait_stage,
                    .commandBufferCount = 1,
                    .pCommandBuffers = &command_buffer,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = &rendering_finished_semaphore,
                };

                if (c.vkResetFences(logical_device, 1, &command_buffer_fence) != c.VK_SUCCESS) {
                    printn("Failed to reset fences");
                    return;
                }
                if (c.vkQueueSubmit(graphics_queue, 1, &submit_info, command_buffer_fence) != c.VK_SUCCESS) {
                    printn("Failed queue submit");
                    return;
                }

                // Present
                const present_info = c.VkPresentInfoKHR{
                    .sType = c.VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                    .pNext = null,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = &rendering_finished_semaphore,
                    .swapchainCount = 1,
                    .pSwapchains = &swap_chain,
                    .pImageIndices = &image_index,
                    .pResults = null,
                };
                const present_result = c.vkQueuePresentKHR(present_queue, &present_info);
                if (present_result == c.VK_ERROR_OUT_OF_DATE_KHR or present_result == c.VK_SUBOPTIMAL_KHR) {
                    break false;
                } else if (present_result != c.VK_SUCCESS) {
                    printn("Present failed");
                    return;
                }
            }

            // Allegedly out of date swapchain / present will not always be returned when the frame resized,
            // so this will handle resetting in those cases.
            if (framebuffer_resized) {
                framebuffer_resized = false;
                break false;
            }
        } else true;

        _ = c.vkDeviceWaitIdle(logical_device);

        if (close) {
            break;
        }
    }

    printn("Shutting down");

    c.glfwDestroyWindow(window);
    c.glfwTerminate();

    return;
}
