#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

typedef uint32_t uint32;

#define internal static
#define global static
#define persist static

global struct glfw_state
{
    GLFWwindow *window;
} window_state;

global struct api_state
{
    VkInstance instance;
} vk_state;

namespace impl
{
    internal std::vector<const char *>
    get_required_extentions(void)
    {
	uint32 glfw_extension_count = 0;
	const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    }
}


internal void
vk_create_state(void)
{
    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
    auto glfw_extensions = impl::get_required_extentions();

    instance_info.enabledExtensionCount = glfw_extensions.size();
    instance_info.ppEnabledExtensionNames = glfw_extensions.data();
}

int
main(int argc
     , char *argv[])
{
    return(0);
}
