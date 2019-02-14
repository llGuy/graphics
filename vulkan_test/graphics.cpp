#define GLFW_INCLUDE_VULKAN
#include <cstring>
#include <cassert>
#include "core.hpp"
#include "graphics.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

extern_impl Vulkan_State vk = {};

void
Vulkan_GPU::find_families(void)
{
    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(hardware
					     , &queue_family_count
					     , nullptr);

    VkQueueFamilyProperties *queue_properties = (VkQueueFamilyProperties *)allocate_stack(sizeof(VkQueueFamilyProperties) * queue_family_count
											  , 1
											  , "queue_family_list_allocation");
    vkGetPhysicalDeviceQueueFamilyProperties(hardware
					     , &queue_family_count
					     , queue_properties);

    for (uint32 i = 0
	     ; i < queue_family_count
	     ; ++i)
    {
	if (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queue_properties[i].queueCount > 0)
	{
	    queue_families.graphics_family = i;
	}

	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(hardware
					     , i
					     , vk.surface
					     , &present_support);
	
	if (queue_properties[i].queueCount > 0 && present_support)
	{
	    queue_families.present_family = i;
	}
	
	if (queue_families.complete())
	{
	    break;
	}
    }

    pop_stack();
}

internal bool
check_if_physical_device_supports_extensions(VkPhysicalDevice gpu)
{
    persist constexpr uint32 required_extension_count = 1;
    persist const char *required_physical_device_extensions[required_extension_count]
    {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(gpu
					 , nullptr
					 , &extension_count
					 , nullptr);

    VkExtensionProperties *extension_properties = (VkExtensionProperties *)allocate_stack(sizeof(VkExtensionProperties) * extension_count
											  , 1
											  , "gpu_extension_properties_list_allocation");
    uint32 required_extensions_left = required_extension_count;
    for (uint32 i
	     ; i < extension_count && required_extensions_left > 0
	     ; ++i)
    {
	for (uint32 j = 0
		 ; j < required_extension_count
		 ; ++j)
	{
	    if (!strcmp(extension_properties[i].extensionName, required_physical_device_extensions[j]))
	    {
		--required_extensions_left;
	    }
	}
    }
    pop_stack();

    return(!required_extensions_left);
}

struct Swapchain_Details
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *present_modes;
};

internal Swapchain_Details
get_swapchain_support(VkPhysicalDevice gpu)
{
    Swapchain_Details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, vk.surface, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, vk.surface, &format_count, nullptr);

    // TODO() 
}

internal bool
check_if_physical_device_is_suitable(Vulkan_GPU *gpu)
{
    gpu->find_families();

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(gpu->hardware
				  , &device_properties);
    
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(gpu->hardware
				, &device_features);

    bool swapchain_supported = check_if_physical_device_supports_extensions(gpu->hardware);

    bool swapchain_usable = false;
    if (swapchain_supported)
    {
	// TODO() check for swapchain details...
    }
    return(false);
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_proc(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
		  , VkDebugUtilsMessageTypeFlagsEXT message_type
		  , const VkDebugUtilsMessengerCallbackDataEXT *message_data
		  , void *user_data)
{
    OUTPUT_DEBUG_LOG("validation layer - %s\n", message_data->pMessage);

    return VK_FALSE;
}

extern_impl void
init_vk(void)
{
    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    instance_info.pApplicationInfo = &app_info;
    
    // get validation support
    persist constexpr uint32 requested_layer_count = 1;
    persist const char *requested_layers[requested_layer_count] { "VK_LAYER_LUNARG_standard_validation" };

    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count
				       , nullptr);

    VkLayerProperties *properties = (VkLayerProperties *)allocate_stack(sizeof(VkLayerProperties) * layer_count
									, 1
									, "validation_layer_list_allocation");
    vkEnumerateInstanceLayerProperties(&layer_count
				       , properties);

    for (uint32 r = 0; r < requested_layer_count; ++r)
    {
	bool found_layer = false;
	for (uint32 l = 0; l < layer_count; ++l)
	{
	    if (!strcmp(properties[l].layerName, requested_layers[r])) found_layer = true;
	}

	if (!found_layer) assert(false);
    }

    // if found then add to the instance information
    instance_info.enabledLayerCount = requested_layer_count;
    instance_info.ppEnabledLayerNames = requested_layers;

    // get extensions needed
    uint32 glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    const char **extensions = (const char **)allocate_stack(sizeof(const char *) * glfw_extension_count + 1
							    , 1
							    , "extension_layer_list_allocation");

    memcpy(extensions, glfw_extensions, sizeof(const char *) * 2);
    
    // add the debug utils extension to the list
    extensions[glfw_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    instance_info.enabledExtensionCount = glfw_extension_count + 1;
    instance_info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateInstance(&instance_info
			      , nullptr
			      , &vk.instance)
	     , "failed to create instance\n");

    pop_stack();
    pop_stack();

    // setup debugger
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
	                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
	                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
	                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = vulkan_debug_proc;
    debug_info.pUserData = nullptr;

    PFN_vkCreateDebugUtilsMessengerEXT vk_create_debug_utils_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
    assert(vk_create_debug_utils_messenger != nullptr);
    VK_CHECK(vk_create_debug_utils_messenger(vk.instance, &debug_info, nullptr, &vk.debug_messenger));
    
    // create the surface
    VK_CHECK(glfwCreateWindowSurface(vk.instance
				     , window
				     , nullptr
				     , &vk.surface));

    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , nullptr);
    
    VkPhysicalDevice *devices = (VkPhysicalDevice *)allocate_stack(sizeof(VkPhysicalDevice) * device_count
								   , 1
								   , "physical_device_list_allocation");
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , devices);

    OUTPUT_DEBUG_LOG("available physical hardware devices count : %d\n", device_count);

    for (uint32 i = 0
	     ; i < device_count
	     ; ++i)
    {
	Vulkan_GPU gpu;
	gpu.hardware = devices[i];
	
	// check if device is suitable
	if (check_if_physical_device_is_suitable(&gpu))
	{
	    
	}
    }

    pop_stack();
}
