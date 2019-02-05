#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <set>
#include <array>
#include <vector>
#include <cstring>
#include <limits>
#include <fstream>
#include <algorithm>
#include <optional.inc>
#include <iostream>
#include <chrono>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <glm/gtx/transform.hpp>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define internal static
#define global static
#define persist static

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 550
#define MAX_FRAMES_IN_FLIGHT 2

global VkInstance vulkan_instance;

global VkSurfaceKHR surface;

global GLFWwindow *window;
global VkDebugUtilsMessengerEXT debug_messenger;

global VkPhysicalDevice physical_device = VK_NULL_HANDLE;
global VkDevice device;

global VkQueue graphics_queue;
global VkQueue present_queue;

global VkSwapchainKHR swapchain;
global std::vector<VkImage> swapchain_images;
global VkExtent2D swapchain_extent; // resolution
global VkFormat swapchain_image_format;
global std::vector<VkImageView> swapchain_image_views;

global VkRenderPass render_pass;

global VkDescriptorPool descriptor_pool;
global std::vector<VkDescriptorSet> descriptor_sets;

global VkDescriptorSetLayout descriptor_set_layout;
global VkPipelineLayout pipeline_layout;
global VkPipeline graphics_pipeline;
global std::vector<VkFramebuffer> swapchain_framebuffers;
global VkCommandPool command_pool;
global std::vector<VkCommandBuffer> command_buffers;
global std::vector<VkSemaphore> image_available_semaphores;
global std::vector<VkSemaphore> render_finished_semaphores;
global std::vector<VkFence> in_flight_fences;

global VkBuffer vertex_buffer;
global VkDeviceMemory vertex_buffer_memory;
global VkBuffer index_buffer;
global VkDeviceMemory index_buffer_memory;
global std::vector<VkBuffer> uniform_buffers;
global std::vector<VkDeviceMemory> uniform_buffers_memory;

constexpr bool enable_validation_layers = true;

internal void
init_glfwwindow(void)
{
    if (!glfwInit())
    {
	throw std::runtime_error("failed to initialize glfw");
    }
    
    glfwWindowHint(GLFW_CLIENT_API , GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH
			      , WINDOW_HEIGHT
			      , "Vulkan"
			      , nullptr
			      , nullptr);
}

internal std::vector<const char *> validation_layers
{
    "VK_LAYER_LUNARG_standard_validation" // validation layer for a whole range of useful validation layers
};

internal bool
check_validation_support(void)
{
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for(const char *layer_name : validation_layers)
    {
	bool layer_found = false;
	for (const auto &layer_properties : available_layers)
	{
	    if (strcmp(layer_name, layer_properties.layerName) == 0)
	    {
		layer_found = true;
		break;
	    }
	}
	if (!layer_found)
	{
	    return false;
	}
    }

    return true;
}

internal std::vector<const char *>
get_required_extensions(void)
{
    uint32 glfw_extension_count = 0;
    const char **glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers)
    {
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
	       , VkDebugUtilsMessageTypeFlagsEXT message_type
	       , const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data
	       , void *p_user_data)
{
    std::cout << "validation layer: " << p_callback_data->pMessage << std::endl;

    std::flush(std::cout);

    return VK_FALSE;
}

internal VkResult
create_debug_utils_messenger_ext(VkInstance instance
				 , const VkDebugUtilsMessengerCreateInfoEXT *p_create_info
				 , const VkAllocationCallbacks *p_allocator
				 , VkDebugUtilsMessengerEXT *p_debug_messenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance
									   , "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
	return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }
    else
    {
	return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

internal void
setup_debug_messenger(void)
{
    if (!enable_validation_layers) return;

    VkDebugUtilsMessengerCreateInfoEXT create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;

    if (create_debug_utils_messenger_ext(vulkan_instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to set up debug messenger");
    }
}

internal void
init_vkinstance(void)
{
    if (!check_validation_support() && enable_validation_layers)
    {
	throw std::runtime_error("validation layer requested but not available");
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info; // this is optional : it just optimizes stuff

    auto glfw_extensions = get_required_extensions();
    
    create_info.enabledExtensionCount = (uint32)(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();

    if (enable_validation_layers)
    {
	create_info.enabledLayerCount = validation_layers.size();
	create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else create_info.enabledLayerCount = 0;
    
    VkResult result = vkCreateInstance(&create_info
				       , nullptr
				       , &vulkan_instance);

    if (result != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create instance");
    }

    uint32 extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    printf("available extensions:\n");
    for (const auto &extension : extensions)
    {
	printf("\t%s\n", extension.extensionName);
    }
}

struct queue_family_indices
{
    std::optional<uint32> graphics_family;
    std::optional<uint32> present_family;

    bool is_complete(void)
    {
	return graphics_family.has_value() && present_family.has_value();
    }
};

internal queue_family_indices
find_queue_families(VkPhysicalDevice phdevice)
{
    queue_family_indices indices;

    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phdevice, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phdevice, &queue_family_count, queue_families.data());

    int32 i = 0;
    for (const auto &queue_family : queue_families)
    {
	if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
	{
	    indices.graphics_family = i;
	}
	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(phdevice, i, surface, &present_support);
	if (queue_family.queueCount > 0 && present_support)
	{
	    indices.present_family = i;
	}
	if (indices.is_complete())
	{
	    break;
	}
	++i;
    }

    return indices;
}

global const std::vector<const char *> device_extensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

internal bool 
check_device_extension_support(VkPhysicalDevice ph_device)
{
    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(ph_device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(ph_device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
    for (const auto &extension : available_extensions)
    {
	required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

struct swapchain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

internal swapchain_support_details
query_swapchain_support(VkPhysicalDevice device)
{
    swapchain_support_details details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (format_count != 0)
    {
	details.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
	details.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

internal VkSurfaceFormatKHR
choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
    {
	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }
    for (const auto &available_format : available_formats)
    {
	if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	{
	    return available_format;
	}
    }
    return available_formats[0];
}

internal VkPresentModeKHR
choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes)
{
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto &available_present_mode : available_present_modes)
    {
	if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
	{
	    return available_present_mode;
	}
	else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	{
	    best_mode = available_present_mode;
	}
    }

    return best_mode;
}

internal VkExtent2D 
choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
    {
	return capabilities.currentExtent;
    }
    else
    {
	VkExtent2D actual_extent = { WINDOW_WIDTH, WINDOW_HEIGHT };
	actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
	actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

	return actual_extent;
    }
}

internal bool
is_device_suitable(VkPhysicalDevice device)
{
    queue_family_indices indices = find_queue_families(device);

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    bool extension_supported = check_device_extension_support(device);

    bool swapchain_adequate = false;
    if (extension_supported)
    {
	swapchain_support_details swapchain_support = query_swapchain_support(device);
	swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    return extension_supported && swapchain_adequate
	&& (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	&& device_features.geometryShader
	&& indices.is_complete();
}

internal void
pick_physical_device(void)
{
    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(vulkan_instance, &device_count, nullptr);
    
    if (device_count == 0)
    {
	throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(vulkan_instance, &device_count, devices.data());

    for (const auto &device : devices)
    {
	if (is_device_suitable(device))
	{
	    physical_device = device;
	    break;
	}
    }

    if (physical_device == VK_NULL_HANDLE)
    {
	throw std::runtime_error("failed to find a suitable GPU");
    }
}

internal void
create_logical_device(void)
{
    queue_family_indices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    float queue_priority = 1.0f;
    for (uint32 queue_family : unique_queue_families)
    {
	VkDeviceQueueCreateInfo queue_create_info = {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queue_family;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = queue_create_infos.size();
    
    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = (uint32)(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    
    if (enable_validation_layers)
    {
	create_info.enabledLayerCount = (uint32)(validation_layers.size());
	create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
	create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

internal void
init_surface(void)
{
    if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create window surface");
    }
}

internal void
init_swapchain(void)
{
    swapchain_support_details swapchain_support = query_swapchain_support(physical_device);
    
    VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);

    uint32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
    {
	image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queue_family_indices indices = find_queue_families(physical_device);
    uint32 queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

    if (indices.graphics_family != indices.present_family)
    {
	create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	create_info.queueFamilyIndexCount = 2;
	create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());

    swapchain_extent = extent;
    swapchain_image_format = surface_format.format;
}

internal void
init_image_views(void)
{
    swapchain_image_views.resize(swapchain_images.size());
    
    for (uint32 i = 0
	     ; i < swapchain_images.size()
	     ; ++i)
    {
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = swapchain_images[i];

	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = swapchain_image_format;

	create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;
	
	if (vkCreateImageView(device, &create_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS)
	{
	    throw std::runtime_error("failed to create image views");
	}
    }
}

internal std::vector<char>
read_file(const char *file_name)
{
    std::ifstream file(std::string(file_name), std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
    {
	printf("error opening %s", file_name);
	throw std::runtime_error("failed to open file");
    }
    
    uint32 file_size = (uint32)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

internal VkShaderModule
create_shader_module(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info = {};

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32 *>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create shader module");
    }

    return shader_module;
}

struct vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription
    get_binding_description(void)
    {
	VkVertexInputBindingDescription binding_description = {};

	binding_description.binding = 0;
	binding_description.stride = sizeof(vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    get_attribute_descriptions(void)
    {
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = {};

	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(vertex, pos);

	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(vertex, color);

	return attribute_descriptions;
    }
};

const std::vector<vertex> vertices =
{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint32> indices =
{
    0, 1, 2, 2, 3, 0
};

internal uint32
find_memory_type(uint32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32 i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
	if (type_filter & (1 << i)
	    && (mem_properties.memoryTypes[i].propertyFlags) == properties)
	{
	    return i;
	}
    }

    throw std::runtime_error("failed to find suitable memory type");
}

internal void
create_buffer(VkBuffer *write_buffer
	   , VkDeviceSize size
	   , VkBufferUsageFlags usage
	   , VkMemoryPropertyFlags properties
	   , VkDeviceMemory *buffer_memory)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage; // GL_ARRAY_BUFFER 
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used by graphics queue
    buffer_info.flags = 0;

    if (vkCreateBuffer(device, &buffer_info, nullptr, write_buffer) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create vbo");
    }

    // need to assign memory (glBufferData)
    VkMemoryRequirements mem_requirements;
    // returns VkMemoryRequirements : size, alignment (depends on usage and flags)
    // and memory type bits - bit field of the memory types that are suitable for the buffer
    vkGetBufferMemoryRequirements(device, *write_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits
						  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
						  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &alloc_info, nullptr, buffer_memory) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device, *write_buffer, *buffer_memory, 0);

    void *data;
    vkMapMemory(device, *buffer_memory, 0, buffer_info.size, 0, &data);

    memcpy(data, vertices.data(), (uint32)buffer_info.size);
    
    vkUnmapMemory(device, *buffer_memory);
}

internal void
copy_buffer(VkBuffer *__restrict src_buffer
	    , VkBuffer *__restrict dst_buffer
	    , VkDeviceSize size)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer copy_command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &copy_command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(copy_command_buffer, &begin_info);

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(copy_command_buffer, *src_buffer, *dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(copy_command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &copy_command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &copy_command_buffer);
}

internal void
create_vbo(void)
{
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(&staging_buffer
		  , buffer_size
		  , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		  , &staging_buffer_memory);

    void *data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (uint32)buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    create_buffer(&vertex_buffer
		  , buffer_size
		  , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		  , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		  , &vertex_buffer_memory);

    copy_buffer(&staging_buffer
		, &vertex_buffer
		, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

internal void
create_ibo(void)
{
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_buffer(&staging_buffer
		  , buffer_size
		  , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		  , &staging_memory);

    void *data;
    vkMapMemory(device, staging_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), (uint32)buffer_size);
    vkUnmapMemory(device, staging_memory);

    create_buffer(&index_buffer
		  , buffer_size
		  , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		  , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		  , &index_buffer_memory);

    copy_buffer(&staging_buffer, &index_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_memory, nullptr);
}

struct uniform_buffer_object
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

internal void
init_description_layout(void)
{
    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create descriptor set layout");
    }
}

internal void
create_descriptor_pool(void)
{
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = (uint32)swapchain_images.size();

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    pool_info.maxSets = (uint32)swapchain_images.size();

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create descriptor pool");
    }
}

internal void
create_descriptor_sets(void)
{
    std::vector<VkDescriptorSetLayout> layouts(swapchain_images.size(), descriptor_set_layout);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = (uint32)swapchain_images.size();
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(swapchain_images.size());

    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create descriptor sets");
    }

    for (uint32 i = 0; i < swapchain_images.size(); ++i)
    {
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = uniform_buffers[i];
	buffer_info.offset = 0;
	buffer_info.range = sizeof(uniform_buffer_object);

	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = descriptor_sets[i];
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;

	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write.descriptorCount = 1;

	descriptor_write.pBufferInfo = &buffer_info;
	descriptor_write.pImageInfo = nullptr;
	descriptor_write.pTexelBufferView = nullptr;
	
	vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
    }
}

internal void
create_ubos(void)
{
    VkDeviceSize buffer_size = sizeof(uniform_buffer_object);
    
    uniform_buffers.resize(swapchain_images.size());
    uniform_buffers_memory.resize(swapchain_images.size());

    for (uint32 i = 0; i < swapchain_images.size(); ++i)
    {
	create_buffer(&uniform_buffers[i]
		      , buffer_size
		      , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		      , &uniform_buffers_memory[i]);
    }
}

internal void
update_ubo(uint32 current_image)
{
    persist auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    uniform_buffer_object ubo = {};
    ubo.model = glm::rotate(glm::radians(90.0f)
			    , glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f)
			   , glm::vec3(0.0f)
			   , glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(60.0f)
				, swapchain_extent.width / (float)swapchain_extent.height
				, 0.1f
				, 10.0f);

    ubo.proj[1][1] *= -1;

    void *data;
    vkMapMemory(device, uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniform_buffers_memory[current_image]);
}

internal void
init_graphics_pipeline(void)
{
    auto vsh_bytecode = read_file("shaders/vert.spv");
    auto fsh_bytecode = read_file("shaders/frag.spv");

    VkShaderModule vsh_shader_module = create_shader_module(vsh_bytecode);
    VkShaderModule fsh_shader_module = create_shader_module(fsh_bytecode);

    VkPipelineShaderStageCreateInfo vsh_shader_stage_info = {};
    vsh_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsh_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vsh_shader_stage_info.module = vsh_shader_module;
    vsh_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fsh_shader_stage_info = {};
    fsh_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsh_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsh_shader_stage_info.module = fsh_shader_module;
    fsh_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages_info[] { vsh_shader_stage_info, fsh_shader_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto binding_description = vertex::get_binding_description();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = (uint32)attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent.width;
    viewport.height = (float)swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // VK_BLEND_FACTOR_SRC_ALPHA
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkDynamicState dynamic_states[]
    {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineLayout pipeline_layout; // TODO()
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages_info;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = nullptr;

    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create graphics pipeline");
    }
    
    vkDestroyShaderModule(device, vsh_shader_module, nullptr);
    vkDestroyShaderModule(device, fsh_shader_module, nullptr);
}

internal void
init_framebuffers(void)
{
    swapchain_framebuffers.resize(swapchain_image_views.size());
    
    for (uint32 i = 0; i < swapchain_image_views.size(); ++i)
    {
	VkImageView attachments[]
	{
	    swapchain_image_views[i]
	};

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = render_pass;
	framebuffer_info.attachmentCount = 1;
	framebuffer_info.pAttachments = attachments;
	framebuffer_info.width = swapchain_extent.width;
	framebuffer_info.height = swapchain_extent.height;
	framebuffer_info.layers = 1;

	if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS)
	{
	    throw std::runtime_error("failed to create framebuffer");
	}
    }
}

internal void
init_render_pass(void)
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create render pass");
    }
}

internal void
init_command_pool(void)
{
    queue_family_indices queue_family_indices = find_queue_families(physical_device);

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
    pool_info.flags = 0; // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT or ...CREATE_RESET_COMMAND_BUFFER_BIT

    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create command pool");
    }
}

internal void
init_command_buffers(void)
{
    command_buffers.resize(swapchain_framebuffers.size());

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32)command_buffers.size();
    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to allocate command buffers");
    }

    for (uint32 i = 0; i < command_buffers.size(); ++i)
    {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	begin_info.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS)
	{
	    throw std::runtime_error("failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = swapchain_framebuffers[i];
    
	render_pass_info.renderArea.offset = {0, 0};
	render_pass_info.renderArea.extent = swapchain_extent;

	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkBuffer vertex_buffers[] = {vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

	vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(command_buffers[i]
				, VK_PIPELINE_BIND_POINT_GRAPHICS
				, pipeline_layout
				, 0, 1
				, &descriptor_sets[i], 0, nullptr);

	vkCmdDrawIndexed(command_buffers[i], (uint32)indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(command_buffers[i]);
	if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
	{
	    throw std::runtime_error("failed to record command buffers");
	}
    }
}

internal void
create_semaphores(void)
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
	if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS
	    || vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS
	    || vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
	{
	    throw std::runtime_error("failed to create semaphores for a frame");
	}
    }
}

global uint32 current_frame = 0;

internal void
draw_frame(void)
{
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, std::numeric_limits<uint64>::max());
    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    uint32 image_index;
    vkAcquireNextImageKHR(device
			  , swapchain
			  , std::numeric_limits<uint64>::max() // disable timeout
			  , image_available_semaphores[current_frame]
			  , VK_NULL_HANDLE
			  , &image_index);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[image_index];

    VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    
    VkSwapchainKHR swapchains[] = { swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    vkQueuePresentKHR(present_queue, &present_info);

    update_ubo(current_frame);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int32
main(int32 argc
     , char * argv[])
{
    init_glfwwindow();

    init_vkinstance();
    init_surface();
    setup_debug_messenger();
    pick_physical_device();
    create_logical_device();
    init_swapchain();
    init_image_views();
    init_render_pass();
    
    init_description_layout();

    init_graphics_pipeline();
    init_framebuffers();
    init_command_pool();
    create_vbo();
    create_ibo();
    create_ubos();
    create_descriptor_pool();
    create_descriptor_sets();
    init_command_buffers();
    create_semaphores();

    std::flush(std::cout);

    while(!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
	draw_frame();

	vkQueueWaitIdle(present_queue);
    }

    vkDeviceWaitIdle(device);

    for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
	vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
	vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
	vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    vkDestroyCommandPool(device, command_pool, nullptr);

    for (auto framebuffer : swapchain_framebuffers)
    {
	vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for (auto image_view : swapchain_image_views)
    {
	vkDestroyImageView(device, image_view, nullptr);
    }

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    for (uint32 i = 0; i < swapchain_images.size(); ++i)
    {
	vkDestroyBuffer(device, uniform_buffers[i], nullptr);
	vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);

    vkDestroyBuffer(device, index_buffer, nullptr);
    vkFreeMemory(device, index_buffer_memory, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return(0);
}
