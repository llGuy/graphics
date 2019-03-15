#pragma once

#include "core.hpp"
//#include "vulkan.hpp"
#include <vulkan/vulkan.h>
#include "vulkan_handles.hpp"

namespace Vulkan_API
{

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

	u32 size;

	Registered_Object_Base(void) = default;
	
	Registered_Object_Base(void *p, const Constant_String &id, u32 size)
	    : p(p), id(id), size(size) {increase_shared_count(id);}

	~Registered_Object_Base(void) {if (p) {decrease_shared_count(id);}}
    };
    
    Registered_Object_Base
    register_object(const Constant_String &id
	     , u32 bytes_size);

    Registered_Object_Base
    get_object(const Constant_String &id);
    
    void
    remove_object(const Constant_String &id);

    // basically just a pointer
    template <typename T> struct Registered_Object
    {
	using My_Type = Registered_Object<T>;
	
	T *p;
	Constant_String id;

	u32 size;

	FORCEINLINE void
	destroy(void) {p = nullptr; decrease_shared_count(id);};

	// to use only if is array
	FORCEINLINE My_Type
	extract(u32 i) {return(My_Type(Registered_Object_Base(p + i, id, sizeof(T))));};

	FORCEINLINE Memory_Buffer_View<My_Type>
	separate(void)
	{
	    Memory_Buffer_View<My_Type> view;
	    allocate_memory_buffer(view, size);

	    for (u32 i = 0; i < size; ++i) view.buffer[i] = extract(i);

	    return(view);
 	}
	
	Registered_Object(void) = default;
	Registered_Object(void *p, const Constant_String &id, u32 size) = delete;
	Registered_Object(const My_Type &in) : p((T *)in.p), id(in.id), size(in.size / sizeof(T)) {increase_shared_count(id);};
	Registered_Object(const Registered_Object_Base &in) : p((T *)in.p), id(in.id), size(in.size / sizeof(T)) {increase_shared_count(id);}
	Registered_Object(Registered_Object_Base &&in) : p((T *)in.p), id(in.id), size(in.size / sizeof(T)) {in.p = nullptr;}
	My_Type &operator=(const My_Type &c) {this->p = c.p; this->id = c.id; this->size = c.size; increase_shared_count(id); return(*this);}
	My_Type &operator=(My_Type &&m)	{this->p = m.p; this->id = m.id; this->size = m.size; m.p = nullptr; return(*this);}

	~Registered_Object(void) {if (p) {decrease_shared_count(id);}}
    };
    
    using Registered_Graphics_Pipeline		= Registered_Object<struct Graphics_Pipeline>;
    using Registered_Render_Pass		= Registered_Object<struct Render_Pass>;
    using Registered_Buffer			= Registered_Object<struct Buffer>;
    using Registered_Descriptor_Set_Layout	= Registered_Object<VkDescriptorSetLayout>;
    using Registered_Descriptor_Set		= Registered_Object<struct Descriptor_Set>;
    using Registered_Model			= Registered_Object<struct Model>;
    using Registered_Command_Pool		= Registered_Object<VkCommandPool>;
    using Registered_Framebuffer		= Registered_Object<struct Framebuffer>;
    using Registered_Image2D			= Registered_Object<struct Image2D>; 

}
