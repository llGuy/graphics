#pragma once

#include "core.hpp"
#include "vulkan.hpp"
#include "vulkan_handles.hpp"

namespace Vulkan_API
{

    Buffer_Handle
    add_buffer(const Constant_String &string);

    Buffer_Handle
    get_buffer_handle(const Constant_String &string);
    
    Vulkan_API::Buffer *
    get_buffer(Buffer_Handle handle);

    

    Render_Pass_Handle
    add_render_pass(const Constant_String &string);

    Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string);
	
    Vulkan_API::Render_Pass *
    get_render_pass(Render_Pass_Handle handle);

    
    
    Descriptor_Set_Layout_Handle
    add_descriptor_set_layout(const Constant_String &string);

    Descriptor_Set_Layout_Handle
    get_descriptor_set_layout_handle(const Constant_String &string);
	
    VkDescriptorSetLayout *
    get_descriptor_set_layout(Descriptor_Set_Layout_Handle handle);



    Model_Handle
    add_model(const Constant_String &string);

    Model_Handle
    get_model_handle(const Constant_String &string);
    
    Model *
    get_model(Model_Handle handle);



    Graphics_Pipeline_Handle
    add_graphics_pipeline(const Constant_String &string);

    Graphics_Pipeline_Handle
    get_graphics_pipeline_handle(const Constant_String &string);
    
    Graphics_Pipeline *
    get_graphics_pipeline(Graphics_Pipeline_Handle handle);

}
