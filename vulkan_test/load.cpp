#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <glm/glm.hpp>

#include "load.hpp"

void
process_vertex(std::vector<std::string> const & vertex_data,
	       std::vector<u32> & indices, std::vector<glm::vec2> const & raw_textures,
	       std::vector<glm::vec3> const & raw_normals,
	       std::vector<glm::vec2> & textures, std::vector<glm::vec3> & normals
	       , u32 & triangle_vertex)
{
    s32 current_vertex = std::stoi(vertex_data[0]) - 1;
    indices.push_back(current_vertex);

    if (raw_textures.size() > 0)
    {
	glm::vec2 current_tex = raw_textures[std::stoi(vertex_data[1]) - 1];
	textures[current_vertex] = current_tex;
    }

    if (raw_normals.size() > 0)
    {
	glm::vec3 current_normal = raw_normals[std::stoi(vertex_data[2]) - 1];
	f32 normal_y = current_normal.y;
	f32 normal_z = current_normal.z;

	normals[current_vertex] = current_normal;
    }

    triangle_vertex = current_vertex;
}
    
std::vector<std::string>
split(std::string const & str, char const splitter)
{
    std::vector<std::string> words;
    std::string current;
    std::istringstream iss(str);
    while (std::getline(iss, current, splitter)) words.push_back(current);

    return words;
}
    
void
break_face_line(const std::vector<std::string> &face_line_words,
		std::vector<u32> &indices, const std::vector<glm::vec2> &raw_textures,
		std::vector<glm::vec3> const &raw_normals,
		std::vector<glm::vec2> &textures, std::vector<glm::vec3> &normals,
		std::vector<glm::vec3> &tangents, std::vector<f32> &tangent_amounts,
		std::vector<glm::vec3> &vertices)
{
    std::array<std::vector<std::string>, 3> face_indices;

    std::array<u32, 3> triangle_vertices;

    for (u32 i = 0; i < 3; ++i)
    {
	face_indices[i] = split(face_line_words[i + 1], '/');
	process_vertex(face_indices[i], indices, raw_textures, raw_normals, textures, normals, triangle_vertices[i]);
    }

    glm::vec3 delta_pos1 = vertices[triangle_vertices[1]] - vertices[triangle_vertices[0]];
    glm::vec3 delta_pos2 = vertices[triangle_vertices[2]] - vertices[triangle_vertices[0]];

    glm::vec2 delta_uv1 = textures[triangle_vertices[1]] - textures[triangle_vertices[0]];
    glm::vec2 delta_uv2 = textures[triangle_vertices[2]] - textures[triangle_vertices[0]];

    f32 r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
    delta_pos1 *= delta_uv2.y;
    delta_pos2 *= delta_uv1.y;
    glm::vec3 tangent = delta_pos1 - delta_pos2;
    tangent *= r;

    tangents[triangle_vertices[0]] = tangent;
    tangents[triangle_vertices[1]] = tangent;
    tangents[triangle_vertices[2]] = tangent;

    ++tangent_amounts[triangle_vertices[0]];
    ++tangent_amounts[triangle_vertices[1]];
    ++tangent_amounts[triangle_vertices[2]];
}

void
create_model(std::vector<glm::vec3> &vertices
	     , std::vector<glm::vec3> &normals
	     , std::vector<glm::vec2> &texture_coords
	     , std::vector<u32> &indices
	     , std::vector<glm::vec3> &tangents
	     , Vulkan_API::Model *object
	     , const std::string &name
	     , Vulkan_API::GPU *gpu)
{
    enum :u32 {POSITION, NORMAL, UVS, TANGENT};
    
    persist constexpr u32 BINDING_AND_ATTRIBUTE_COUNT = 3;
    
    object->attribute_count = BINDING_AND_ATTRIBUTE_COUNT;
    object->attributes_buffer = (VkVertexInputAttributeDescription *)allocate_free_list(sizeof(VkVertexInputAttributeDescription) * object->attribute_count
											, Alignment(1));

    object->binding_count = BINDING_AND_ATTRIBUTE_COUNT;
    object->bindings = (Vulkan_API::Model_Binding *)allocate_free_list(sizeof(Vulkan_API::Model_Binding) * object->binding_count
								       , Alignment(1));

    Vulkan_API::Model_Binding *bindings = object->bindings;
    bindings[POSITION].begin_attributes_creation(object->attributes_buffer);
    bindings[POSITION].push_attribute(POSITION, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3));
    bindings[POSITION].end_attributes_creation();

    bindings[NORMAL].begin_attributes_creation(object->attributes_buffer);
    bindings[NORMAL].push_attribute(NORMAL, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3));
    bindings[NORMAL].end_attributes_creation();

    bindings[UVS].begin_attributes_creation(object->attributes_buffer);
    bindings[UVS].push_attribute(UVS, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2));
    bindings[UVS].end_attributes_creation();
    /*
    bindings[TANGENT].begin_attributes_creation(object->attributes_buffer);
    bindings[TANGENT].push_attribute(TANGENT, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3));
    bindings[TANGENT].end_attributes_creation();*/

    std::string buffer_name = "buffer." + name + ".buffers";

    Vulkan_API::Registered_Buffer buffers = Vulkan_API::register_object(init_const_str(buffer_name.c_str(), buffer_name.length())
									, sizeof(Vulkan_API::Buffer) * BINDING_AND_ATTRIBUTE_COUNT);

    Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(glm::vec3) * (u32)vertices.size(), vertices.data()}
								  , command_pool.p
								  , &buffers.p[POSITION]
								  , gpu);
    bindings[POSITION].buffer = buffers.p[POSITION].buffer;
    
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(glm::vec3) * (u32)normals.size(), normals.data()}
								  , command_pool.p
								  , &buffers.p[NORMAL]
								  , gpu);
    bindings[NORMAL].buffer = buffers.p[NORMAL].buffer;
    
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(glm::vec2) * (u32)texture_coords.size(), texture_coords.data()}
								  , command_pool.p
								  , &buffers.p[UVS]
								  , gpu);
    bindings[UVS].buffer = buffers.p[UVS].buffer;
    
    /*	Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(glm::vec3) * (u32)tangents.size(), tangents.data()}
	, command_pool.p
	, &buffers.p[TANGENT]
	, gpu);
	bindings[TANGENT].buffer = buffers.p[TANGENT].buffer;*/
    
    object->index_data.index_type = VK_INDEX_TYPE_UINT32;
    object->index_data.index_offset = 0;
    object->index_data.index_count = indices.size();
    
    std::string ibo_name = "buffer." + name + ".ibo";
    Vulkan_API::Registered_Buffer ibo = Vulkan_API::register_object(init_const_str(ibo_name.c_str(), ibo_name.length())
								    , sizeof(Vulkan_API::Buffer));
    
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{(u32)indices.size() * sizeof(u32), indices.data()}
								  , command_pool.p
								  , ibo.p
								  , gpu);
    
    object->index_data.index_buffer = ibo.p->buffer;
    
    object->create_vbo_list();
}
    
void
load_model_from_obj(const char *filename
		    , Vulkan_API::Model *dst
		    , const char *model_name
		    , Vulkan_API::GPU *gpu)
{
    std::ifstream file(filename);

    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texture_coords;
    std::vector<glm::vec3> tangents;
    std::vector<f32> tangent_amounts;

    /* won't be in the  correct order */
    std::vector<glm::vec2> raw_texture_coords;
    std::vector<glm::vec3> raw_normals;
    std::vector<u32> indices;

    std::string line;
    while (std::getline(file, line))
    {
	std::vector<std::string> words = split(line, ' ');

	if (words[0] == "v")
	{
	    glm::vec3 vertex;
	    for (u32 i = 0; i < 3; ++i) vertex[i] = std::stof(words[i + 1]);
	    vertices.push_back(vertex);
	}
	else if (words[0] == "vt")
	{
	    glm::vec2 texture_coord;
	    for (u32 i = 0; i < 2; ++i) texture_coord[i] = std::stof(words[i + 1]);
	    raw_texture_coords.push_back(glm::vec2(texture_coord.x, 1.0f - texture_coord.y));
	}
	else if (words[0] == "vn")
	{
	    glm::vec3 normal;
	    for (u32 i = 0; i < 3; ++i) normal[i] = std::stof(words[i + 1]);
	    raw_normals.push_back(normal);
	}
	else if (words[0] == "f")
	{
	    tangents.resize(vertices.size());
	    tangent_amounts.resize(vertices.size());
	    normals.resize(vertices.size());
	    texture_coords.resize(vertices.size());

	    break_face_line(words
			    , indices
			    , raw_texture_coords
			    , raw_normals
			    , texture_coords
			    , normals
			    , tangents
			    , tangent_amounts
			    , vertices);

	    break;
	}
    }

    while (std::getline(file, line))
    {
	std::vector<std::string> words = split(line, ' ');
	if (words[0] == "f")
	{
	    break_face_line(words
			    , indices
			    , raw_texture_coords
			    , raw_normals
			    , texture_coords
			    , normals
			    , tangents
			    , tangent_amounts
			    , vertices);
	}
    }

    for (u32 i = 0; i < tangents.size(); ++i)
    {
	tangents[i] /= tangent_amounts[i];
    }

    create_model(vertices
		 , normals
		 , texture_coords
		 , indices
		 , tangents
		 , dst
		 , model_name
		 , gpu);
}

internal u32
get_terrain_index(u32 x, u32 z, u32 depth_z)
{
    return(x + z * depth_z);
}

void
load_3D_terrain_mesh(u32 width_x
		     , u32 depth_z
		     , f32 random_displacement_factor
		     , Vulkan_API::Model *terrain_mesh_base_model_info
		     , Vulkan_API::Buffer *mesh_buffer_vbo
		     , Vulkan_API::Buffer *mesh_buffer_ibo
		     , Vulkan_API::GPU *gpu)
{
    assert(width_x & 0X1 && depth_z & 0X1);
    
    f32 *vtx = (f32 *)allocate_stack(sizeof(f32) * 2 * width_x * depth_z);
    u32 *idx = (u32 *)allocate_stack(sizeof(u32) * 10 * (((width_x - 1) * (depth_z - 1)) / 2));
    
    for (u32 z = 0; z < depth_z; ++z)
    {
	for (u32 x = 0; x < width_x; ++x)
	{
	    // TODO : apply displacement factor to make terrain less perfect
	    u32 index = (x + depth_z * z) * 2;
	    vtx[index] = (f32)x;
	    vtx[index + 1] = (f32)z;
	}	
    }

    u32 crnt_idx = 0;
    
    for (u32 z = 1; z < depth_z - 1; z += 2)
    {
        for (u32 x = 1; x < width_x - 1; x += 2)
	{
	    idx[crnt_idx++] = get_terrain_index(x, z, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x - 1, z - 1, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x - 1, z, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x - 1, z + 1, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x, z + 1, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x + 1, z + 1, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x + 1, z, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x + 1, z - 1, depth_z);
	    idx[crnt_idx++] = get_terrain_index(x, z - 1, depth_z);
	    idx[crnt_idx++] = 0xFFFFFFFF;
	}
    }
    
    // load data into buffers
    Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(f32) * 2 * width_x * depth_z, vtx}
							      , command_pool.p
							      , mesh_buffer_vbo
							      , gpu);
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(u32) * 8 * (((width_x - 1) * (depth_z - 1)) / 2), vtx}
							      , command_pool.p
							      , mesh_buffer_ibo
							      , gpu);

    terrain_mesh_base_model_info->attribute_count = 3;
    terrain_mesh_base_model_info->attributes_buffer = (VkVertexInputAttributeDescription *)allocate_free_list(sizeof(VkVertexInputAttributeDescription) * terrain_mesh_base_model_info->attribute_count);
    terrain_mesh_base_model_info->binding_count = 2;
    terrain_mesh_base_model_info->bindings = (Vulkan_API::Model_Binding *)allocate_free_list(sizeof(Vulkan_API::Model_Binding) * terrain_mesh_base_model_info->binding_count);
    enum :u32 {GROUND_BASE_XY_VALUES_BND = 0, HEIGHT_BND = 1, GROUND_BASE_XY_VALUES_ATT = 0, HEIGHT_ATT = 1};
    // buffer that holds only the x-z values of each vertex - the reason is so that we can create multiple terrain meshes without copying the x-z values each time
    terrain_mesh_base_model_info->bindings[GROUND_BASE_XY_VALUES_BND].begin_attributes_creation(terrain_mesh_base_model_info->attributes_buffer);
    terrain_mesh_base_model_info->bindings[GROUND_BASE_XY_VALUES_BND].push_attribute(GROUND_BASE_XY_VALUES_ATT, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2);
    terrain_mesh_base_model_info->bindings[GROUND_BASE_XY_VALUES_BND].end_attributes_creation();
    // buffer contains the y-values of each mesh and the colors of each mesh
    terrain_mesh_base_model_info->bindings[HEIGHT_BND].begin_attributes_creation(terrain_mesh_base_model_info->attributes_buffer);
    terrain_mesh_base_model_info->bindings[HEIGHT_BND].push_attribute(HEIGHT_ATT, VK_FORMAT_R32_SFLOAT, sizeof(f32));
    terrain_mesh_base_model_info->bindings[HEIGHT_BND].end_attributes_creation();
    
    pop_stack();
    pop_stack();
}

Terrain_Mesh_Instance
load_3D_terrain_mesh_instance(u32 width_x
			      , u32 depth_z
			      , Vulkan_API::Model *prototype
			      , Vulkan_API::Buffer *ys_buffer
			      , Vulkan_API::GPU *gpu)
{
    Terrain_Mesh_Instance ret = {};
    ret.model = prototype->copy();
    ret.ys = (f32 *)allocate_free_list(sizeof(f32) * width_x * depth_z);
    memset(ret.ys, 0, sizeof(f32) * width_x * depth_z);
    
    Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);    
    Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof(f32) * width_x * depth_z, ret.ys}
							      , command_pool.p
							      , ys_buffer
							      , gpu);
    ret.ys_gpu = *ys_buffer;
    enum :u32 {GROUND_BASE_XY_VALUES_BND = 0, HEIGHT_BND = 1, GROUND_BASE_XY_VALUES_ATT = 0, HEIGHT_ATT = 1};
    ret.model.bindings[HEIGHT_BND].buffer = ret.ys_gpu.buffer;

    ret.model.create_vbo_list();
    return(ret);
}
