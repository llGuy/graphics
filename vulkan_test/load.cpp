#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <glm/glm.hpp>

#include "load.hpp"

namespace Load
{
    void
    process_vertex(std::vector<std::string> const & vertex_data,
		   std::vector<u32> & indices, std::vector<glm::vec2> const & raw_textures,
		   std::vector<glm::vec3> const & raw_normals,
		   std::vector<glm::vec2> & textures, std::vector<glm::vec3> & normals)
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
	    //current_normal.y = -normal_y;
	    //current_normal.z = normal_z;
	    //current_normal.y *= -1;
	    normals[current_vertex] = current_normal;
	}
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
    break_face_line(std::vector<std::string> const & face_line_words,
		    std::vector<u32> & indices, std::vector<glm::vec2> const & raw_textures,
		    std::vector<glm::vec3> const & raw_normals,
		    std::vector<glm::vec2> & textures, std::vector<glm::vec3> & normals)
    {
	std::array<std::vector<std::string>, 3> face_indices;
	for (u32 i = 0; i < 3; ++i)
	{
	    face_indices[i] = split(face_line_words[i + 1], '/');
	    process_vertex(face_indices[i], indices, raw_textures, raw_normals, textures, normals);
	}
    }

    void
    create_model(std::vector<glm::vec3> & vertices
		 , std::vector<glm::vec3> & normals
		 , std::vector<glm::vec2> & texture_coords
		 , std::vector<u32> & indices
		 , Vulkan_API::Model *model
		 , const char *mdl_name)
    {
	
    } 
    
    void
    load_model_from_obj(Vulkan_API::Model *dst
			, const char *filename
			, const char *mdl_name)
    {
	std::ifstream file(filename);

	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texture_coords;

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
		normals.resize(vertices.size());
		texture_coords.resize(vertices.size());

		break_face_line(words, indices, raw_texture_coords, raw_normals, texture_coords, normals);

		break;
	    }
	}

	while (std::getline(file, line))
	{
	    std::vector<std::string> words = split(line, ' ');
	    if (words[0] == "f")
	    {
		break_face_line(words, indices, raw_texture_coords, raw_normals, texture_coords, normals);
	    }
	}

	create_model(vertices, normals, texture_coords, indices, dst, mdl_name);
    }
}
