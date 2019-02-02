
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <set>
#include <vector>
#include <cstring>
#include <limits>
#include <fstream>
#include <algorithm>
#include <optional.inc>
#include <iostream>
//#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

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
global VkPipelineLayout pipeline_layout;
global VkPipeline graphics_pipeline;

//#ifdef NDEBUG
//constexpr bool enable_validation_layers = false;
//#else
constexpr bool enable_validation_layers = true;
//#endif

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

internal std::vector<const char *> validation_layers{ "VK_LAYER_LUNARG_standard_validation" };

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

    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
	
    }

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
    create_info.pApplicationInfo = &app_info;

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
find_queue_families(VkPhysicalDevice device)
{
    queue_family_indices indices;

    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int32 i = 0;
    for (const auto &queue_family : queue_families)
    {
	if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
	{
	    indices.graphics_family = i;
	}
	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
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
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
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
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
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

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
	throw std::runtime_error("failed to create render pass");
    }
}

int
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
    init_render_pass();
    init_graphics_pipeline();

    std::flush(std::cout);

    while(!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
    }

    for (auto image_view : swapchain_image_views)
    {
	vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);
    vkDestroyDevice(device, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return(0);
}
