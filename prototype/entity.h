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

struct physics_component
{
    uint32 EntityID;

    float MaxSpeed;
    float Speed;
};

struct render_component
{
    uint32 EntityID;
    
    uint32 Model;

    struct render_layer3D *Layer;
};

struct keyboard_component
{
    uint32 EntityID;
};

struct mouse_component
{
    uint32 EntityID;

    glm::vec2 PreviousMousePosition{ 0.0f };
};

struct entity_components
{
    int32 ModelMatrixComponentIndex{ MISSING_COMPONENT };
    int32 RenderComponentIndex { MISSING_COMPONENT };
    int32 KeyboardComponentIndex { MISSING_COMPONENT };
    int32 MouseComponentIndex { MISSING_COMPONENT };
    int32 PhysicsComponentIndex { MISSING_COMPONENT };
};

struct entity_data
{
    glm::vec3 Position3D;
    glm::vec3 Direction3D;
    glm::vec3 Velocity3D;
    glm::vec3 Scale3D;

    entity_components Components;
};

template <typename T> struct component_list
{
    std::vector<T> List;

    uint32
    Add(uint32 EntityID
	, const T &CompIn)
    {
	T Component = CompIn;
	Component.EntityID = EntityID;
	List.push_back(Component);

	return List.size() - 1;
    }
};

extern struct entity_data_base
{
    component_list<model_matrix_component> ModelMatrixComponents;
    component_list<render_component> RenderComponents;
    component_list<keyboard_component> KeyboardComponents;
    component_list<mouse_component> MouseComponents;
    component_list<physics_component> PhysicsComponents;

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
