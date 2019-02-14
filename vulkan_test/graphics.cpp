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

    instance_info.enabledExtensionCount = glfw_extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateInstance(&instance_info
			      , nullptr
			      , &vk.instance)
	     , "failed to create instance\n");

    pop_stack();
    pop_stack();

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
