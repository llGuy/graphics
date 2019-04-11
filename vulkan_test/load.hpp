#pragma once

#include "vulkan.hpp"

// to use later on in the project
enum Vertex_Attribute_Bits {POSITION = 1 << 0
			    , NORMAL = 1 << 1
			    , UVS = 1 << 2
			    , COLOR = 1 << 3};
    
void
load_model_from_obj(const char *filename
		    , Vulkan_API::Model *dst
		    , const char *model_name
		    , Vulkan_API::GPU *gpu);

struct Terrain_Mesh_Height_Data
{
    
};

void
load_3D_terrain_mesh(u32 width_x
		     , u32 depth_z
		     , f32 random_displacement_factor
		     , Vulkan_API::GPU *gpu);