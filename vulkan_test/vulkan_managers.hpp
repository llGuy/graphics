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
    
    Buffer *
    get_buffer(Buffer_Handle handle);

    Buffer *
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


    
    Descriptor_Set_Handle
    add_descriptor_set(const Constant_String &string);

    Descriptor_Set_Handle
    get_descriptor_set_handle(const Constant_String &string);
    
    Descriptor_Set *
    get_descriptor_set(Descriptor_Set_Handle handle);

    Descriptor_Set *
    get_descriptor_Set(const Constant_String &name);




    


    void
    init_manager(void);
    

    void
    decrease_shared_count(const Constant_String &id);

    void
    increase_shared_count(const Constant_String &id);    

    struct Registered_Object_Base
    {
	// if p == nullptr, the object was deleted
	void *p;
	Constant_String id;

	Registered_Object_Base(void) = default;
	
	Registered_Object_Base(void *p, const Constant_String &id)
	    : p(p), id(id) {increase_shared_count(id);}

	~Registered_Object_Base(void) {if (p) {decrease_shared_count(id);}}
    };
    
    Registered_Object_Base
    register_object(const Constant_String &id
		    , u32 bytes_size);

    Registered_Object_Base
    get_object(const Constant_String &id);

    void
    remove_object(const Constant_String &id);
    
    template <typename T> struct Registered_Object : Registered_Object_Base
    {
	using My_Type = Registered_Object<T>;
	
	T *p;
	Constant_String id;

	FORCEINLINE void
	destroy(void) {p = nullptr; decrease_shared_count(id);};
	
	Registered_Object(void) = default;
	Registered_Object(void *p, const Constant_String &id) = delete;
	Registered_Object(const My_Type &in) : p((T *)in.p), id(in.id) {increase_shared_count(id);};
	Registered_Object(const Registered_Object_Base &in) : p((T *)in.p), id(in.id) {increase_shared_count(id);}
	Registered_Object(Registered_Object_Base &&in) : p((T *)in.p), id(in.id) {in.p = nullptr;}
	My_Type &operator=(const My_Type &c) {this->p = c.p; this->id = c.id; increase_shared_count(id);}
	My_Type &operator=(My_Type &&m) {this->p = m.p; this->id = m.id; m.p = nullptr;}

	~Registered_Object(void) {if (p) {decrease_shared_count(id);}}
    };

}
