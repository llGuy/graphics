#pragma once

#include "core.hpp"
#include "vulkan.hpp"

/* all rendering objects (render passes, buffers, images, etc...) are accessed
   through 32 bit handles (with object respective names) however, if the handle is 
   not known in a certain scope, it can be accessed through a "[name]"_hash identifier 
   which will serve as a lookup into a hash table mapping string hashes to object handles
   a handle cache is maintained for extra speed (however if that extra speed is negligible, 
   in the future, all objects will simply be accessed through these strings) */

namespace Rendering
{        

    /* all vulkan object handle types : handle types are just indices into an array */
    using Vulkan_Buffer_Handle = uint32;
    using Vulkan_Pipeline_Handle = uint32;
    using Vulkan_Image_Handle = uint32;
    using Vulkan_Image_View_Handle = uint32;
    using Vulkan_Render_Pass_Handle = uint32;
    using Vulkan_Descriptor_Set_Layout_Handle = uint32;
    using Vulkan_Descriptor_Set_Handle = uint32;
    using Vulkan_Model_Handle = uint32;

    // describes the binding of a buffer to a model VAO
    struct Vulkan_Model_Binding
    {
	struct Attribute
	{
	    uint32 binding;
	    uint32 location;
	    VkFormat format;
	    uint32 offset;
	};

	// buffer that stores all the attributes
	Vulkan_Buffer_Handle buffer;
	uint32 binding;
	VkVertexInputRate input_rate;

	Attribute *attribute_list = nullptr;
	uint32 stride = 0;
	
	void
	begin_attributes_creation(Attribute *attribute_list)
	{
	    this->attribute_list = attribute_list;
	}
	
	void
	push_attribute(uint32 location, VkFormat format, uint32 size)
	{
	    Attribute attribute = {};
	    attribute.binding = binding;
	    attribute.location = location;
	    attribute.format = format;
	    attribute.offset = stride;

	    stride += size;
	}

	void
	end_attributes_creation(void)
	{
	    attribute_list = nullptr;
	}
    };
    
    // describes the attributes and bindings of the model
    struct Vulkan_Model
    {
	// model bindings
	uint32 binding_size;
	Vulkan_Model_Binding *bindings;
    };

    // describes the material of the object being rendered
    struct Vulkan_Material
    {
	
    };
    
    extern Vulkan_Buffer_Handle
    add_buffer(const Constant_String &string);

    extern Vulkan_Buffer_Handle
    get_buffer_handle(const Constant_String &string);
    
    extern Vulkan_API::Buffer *
    get_buffer(Vulkan_Buffer_Handle handle);

    

    extern Vulkan_Render_Pass_Handle
    add_render_pass(const Constant_String &string);

    extern Vulkan_Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string);
	
    extern Vulkan_API::Render_Pass *
    get_render_pass(Vulkan_Buffer_Handle handle);


    
    extern Vulkan_Descriptor_Set_Layout_Handle
    add_descriptor_set_layout(const Constant_String &string);

    extern Vulkan_Descriptor_Set_Layout_Handle
    get_descriptor_set_layout_handle(const Constant_String &string);
	
    extern VkDescriptorSetLayout *
    get_descriptor_set_layout(Vulkan_Buffer_Handle handle);

    
    
    struct Rendering_Objects_Handle_Cache
    {
	Vulkan_Render_Pass_Handle test_render_pass;
	Vulkan_Descriptor_Set_Layout_Handle descriptor_set_layout;
    };
    
    extern void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache);

}
