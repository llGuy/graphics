#ifndef _MODEL_H_
#define _MODEL_H_

#define SHIFT(n) 1 << n

#include <vector>
#include <iterator>
#include <algorithm>
#include "core.hpp"
#include "resources.hpp"

enum Model_Component :uint16
{
    VERTEX_POSITION
    , VERTEX_COLOR
    , VERTEX_NORMAL
    , VERTEX_UVS
    , VERTEX_TANGENT
    , VERTEX_INVALID
};

#define VERTEX_POSITION_SIZE sizeof(float32) * 3
#define VERTEX_COLOR_SIZE sizeof(float32) * 3
#define VERTEX_NORMAL_SIZE sizeof(float32) * 3
#define VERTEX_UVS_SIZE sizeof(float32) * 2
#define VERTEX_TANGENT_SIZE sizeof(float32) * 3
#define VERTEX_EXTRA_V4_SIZE sizeof(float32) * 4

internal VkFormat component_format_map[VERTEX_INVALID]
{
    // format  (v3_f32)
    VK_FORMAT_R32G32B32_SFLOAT
    // color   (v3_f32)
    , VK_FORMAT_R32G32B32_SFLOAT
    // normal  (v3_f32)
    , VK_FORMAT_R32G32B32_SFLOAT
    // uvs     (v2_f32)
    , VK_FORMAT_R32G32_SFLOAT
    // tangent (v3_f32)
    , VK_FORMAT_R32G32B32_SFLOAT
};

internal uint16 component_size_map[VERTEX_INVALID]
{
    VERTEX_POSITION_SIZE
    , VERTEX_COLOR_SIZE
    , VERTEX_NORMAL_SIZE
    , VERTEX_UVS_SIZE
    , VERTEX_TANGENT_SIZE
};

struct Model_Binding_Meta
{
    uint32 binding;
    uint16 components_flags;
    uint16 components_count;

    buffer_handle buffer;

    VkVertexInputBindingDescription
    get_binding_description(void)
    {
	// calculate stride
	uint32 stride = 0;
	for (uint16 i = 0
		 ; i < components_count
		 ; ++i)
	{
	    if (components_flags & SHIFT(i))
	    {
		stride += component_size_map[i];
	    }
	}

	VkVertexInputBindingDescription binding_info = {};
	binding_info.binding = binding;
	binding_info.stride = stride;
	binding_info.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	return(binding_info);
    }

    std::vector<VkVertexInputAttributeDescription>
    get_attribute_descriptions(void)
    {
	std::vector<VkVertexInputAttributeDescription> descriptions;
	descriptions.resize(components_count);
	
	auto calculate_offset_in_buffer = [this](uint16 component_index) -> uint32
	{
	    if (!component_index) return 0;

	    uint32 accumulated = component_size_map[0];

	    for (uint32 i = 1
		     ; i < component_index
		     ; ++i)
	    {
		if (components_flags & SHIFT(i))
		{
		    accumulated += component_size_map[i];
		}
	    }
	};

	uint32 count = 0;

	for (uint16 i = 0
		 ; i < VERTEX_INVALID
		 ; ++i)
	{
	    if (components_flags & SHIFT(i))
	    {
		VkVertexInputAttributeDescription attribute = {};
		attribute.binding = binding;
		attribute.location = i;
		attribute.format = component_format_map[i];
		attribute.offset = calculate_offset_in_buffer(i);

		descriptions[count++] = attribute;
	    }
	}

	return(descriptions);
    }
};

struct Model
{
    // binding information
    std::vector<Model_Binding_Meta> descriptions_meta;
    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    
    std::vector<VkVertexInputAttributeDescription>
    get_all_attributes(void)
    {
	auto get_attribute_descriptions_size = [this]()
	{
	    uint32 size = 0;
	    for (uint32 i = 0
		     ; i < descriptions_meta.size()
		     ; ++i) size += descriptions_meta[i].components_count;
	    return size;
	};
	
	std::vector<VkVertexInputAttributeDescription> descriptions;
	descriptions.reserve(get_attribute_descriptions_size());

	for (uint32 i = 0
		 ; i < descriptions_meta.size()
		 ; ++i)
	{
	    auto current_attributes = descriptions_meta[i].get_attribute_descriptions();
	    std::copy(current_attributes.begin(), current_attributes.end()
		      , std::back_inserter(descriptions));
	}

	return descriptions;
    }
};

#endif
