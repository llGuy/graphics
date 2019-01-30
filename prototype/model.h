#ifndef _MODEL_H_
#define _MODEL_H_

#include "int.h"
#include <vector>
#include <glm/glm.hpp>
#include "container.h"
#include "opengl_api.h"
#include <unordered_map>

struct model_components
{
    int32 VBOIndex;
    int32 IBOIndex;
    int32 NBOIndex;
    int32 UVBOIndex;
    int32 TBOIndex;
    int32 VerticesListIndex;
};

struct model_data
{
    model_components Components;

    uint32 ID;

    GLenum Primitive { GL_TRIANGLES };

    ogl_vertex_array VAO;
};

using vertex_list = std::vector<float>;

template<typename T> struct model_component
{
    uint32 ID;
    T Component;
};

extern struct model_data_base
{
    /* Structure-of-Array */
    std::vector<model_component<ogl_vbo>> VBOs;  // vertex buffers
    std::vector<model_component<ogl_ibo>> IBOs;  // index buffers
    std::vector<model_component<ogl_vbo>> NBOs;  // normal buffers
    std::vector<model_component<ogl_vbo>> TBOs;  // tangent buffers
    std::vector<model_component<ogl_vbo>> UVBOs; // uv buffers
    std::vector<model_component<vertex_list>> VerticesLists;

    std::vector<model_data> Models;

    // use when NEEDED
    std::unordered_map<string_view, uint32, string_hash<string_view>> IndexMap;

    uint32
    CreateModel(const string_view &Name);

    model_data *
    GetModel(uint32 ID);

    uint32
    GetModelID(const string_view &Name);

    ogl_vbo *
    GetVBOComponent(uint32 ID);

    ogl_ibo *
    GetIBOComponent(uint32 ID);

    ogl_vbo *
    GetNBOComponent(uint32 ID);

    ogl_vbo *
    GetTBOComponent(uint32 ID);

    vertex_list *
    GetVertex3DListComponent(uint32 ID);

    void
    LoadModel(const char* Format
	      , const char *Filename
	      , uint32 ID);
} ModelDataBase;

#endif
