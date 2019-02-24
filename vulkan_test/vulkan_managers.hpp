#pragma once

#include "core.hpp"
#include "vulkan.hpp"
#include "vulkan_handles.hpp"

namespace Vulkan_API
{

    extern Buffer_Handle
    add_buffer(const Constant_String &string);

    extern Buffer_Handle
    get_buffer_handle(const Constant_String &string);
    
    extern Vulkan_API::Buffer *
    get_buffer(Buffer_Handle handle);

    

    extern Render_Pass_Handle
    add_render_pass(const Constant_String &string);

    extern Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string);
	
    extern Vulkan_API::Render_Pass *
    get_render_pass(Render_Pass_Handle handle);

    
    
    extern Descriptor_Set_Layout_Handle
    add_descriptor_set_layout(const Constant_String &string);

    extern Descriptor_Set_Layout_Handle
    get_descriptor_set_layout_handle(const Constant_String &string);
	
    extern VkDescriptorSetLayout *
    get_descriptor_set_layout(Descriptor_Set_Layout_Handle handle);



    extern Model_Handle
    add_vulkan_model(const Constant_String &string);

    extern Model_Handle
    get_vulkan_model_handle(const Constant_String &string);
    
    extern Model *
    get_vulkan_model(Model_Handle handle);

}
