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

    Vulkan_API::Buffer *
    get_buffer(const Constant_String &name);

    

    Render_Pass_Handle
    add_render_pass(const Constant_String &string);

    Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string);
	
    Vulkan_API::Render_Pass *
    get_render_pass(Render_Pass_Handle handle);

    Vulkan_API::Render_Pass *
    get_render_pass(const Constant_String &name);

    
    
    Descriptor_Set_Layout_Handle
    add_descriptor_set_layout(const Constant_String &string);

    Descriptor_Set_Layout_Handle
    get_descriptor_set_layout_handle(const Constant_String &string);
	
    VkDescriptorSetLayout *
    get_descriptor_set_layout(Descriptor_Set_Layout_Handle handle);

    VkDescriptorSetLayout *
    get_descriptor_set_layout(const Constant_String &handle);



    Model_Handle
    add_model(const Constant_String &string);

    Model_Handle
    get_model_handle(const Constant_String &string);
    
    Model *
    get_model(Model_Handle handle);

    Model *
    get_model(const Constant_String &name);



    Graphics_Pipeline_Handle
    add_graphics_pipeline(const Constant_String &string);

    Graphics_Pipeline_Handle
    get_graphics_pipeline_handle(const Constant_String &string);
    
    Graphics_Pipeline *
    get_graphics_pipeline(Graphics_Pipeline_Handle handle);

    Graphics_Pipeline *
    get_graphics_pipeline(const Constant_String &handle);



    Command_Pool_Handle
    add_command_pool(const Constant_String &string);

    Command_Pool_Handle
    get_command_pool_handle(const Constant_String &string);
    
    VkCommandPool *
    get_command_pool(Command_Pool_Handle handle);

    VkCommandPool *
    get_command_pool(const Constant_String &handle);



    Image2D_Handle
    add_image2D(const Constant_String &string);

    Image2D_Handle
    get_image2D_handle(const Constant_String &string);
    
    Image2D *
    get_image2D(Image2D_Handle handle);

    Image2D *
    get_image2D(const Constant_String &name);



    Framebuffer_Handle
    add_framebuffer(const Constant_String &string);

    Framebuffer_Handle
    get_framebuffer_handle(const Constant_String &string);
    
    Framebuffer *
    get_framebuffer(Image2D_Handle handle);

    Framebuffer *
    get_framebuffer(const Constant_String &name);

}
