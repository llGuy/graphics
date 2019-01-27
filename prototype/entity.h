#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "int.h"
#include <vector>
#include "model.h"
#include <glm/glm.hpp>

#define MISSING_COMPONENT -1

struct model_matrix_component
{
    uint32 EntityID;

    glm::mat4 TRS;
};

struct render_component
{
    uint32 EntityID;
    
    uint32 Model;

    struct render_layer *Layer;
};

struct keyboard_component
{
    uint32 EntityID;
};

struct mouse_component
{
    uint32 EntityID;
};

struct entity_components
{
    int32 ModelMatrixComponentIndex;
    int32 RenderComponentIndex;
    int32 KeyboardComponentIndex;
    int32 MouseComponentIndex;
};

struct entity_data
{
    glm::vec3 Position3D;
    glm::vec3 Direction3D;
    glm::vec3 Scale3D;

    entity_components Components;
};

extern struct entity_data_base
{
    std::vector<model_matrix_component> ModelMatrixComponents;
    std::vector<render_component> RenderComponents;
    std::vector<keyboard_component> KeyboardComponents;
    std::vector<mouse_component> MoueComponents;

    std::vector<entity_data> Entities;
    
    std::unordered_map<string_view, uint32, string_hash<string_view>> IndexMap;

    uint32
    CreateEntity(const string_view &Name);
    
    entity_data *
    GetEntity(uint32 ID);

    uint32
    GetEntityID(const string_view &Name);

    void
    Update(void);
} EntityDataBase;

#endif
