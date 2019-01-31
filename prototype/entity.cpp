#include "entity.h"
#include "render.h"
#include "global.h"
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

entity_data_base EntityDataBase = {};

internal void
UpdateModelMatrixComponents(std::vector<model_matrix_component> &Components
			    , std::vector<entity_data> &Entities)
{
    model_matrix_component *End = &( Components.back() ) + 1;
    for (model_matrix_component *Component = &( Components[0] )
	     ; Component != End
	     ; ++Component)
    {
	entity_data *Entity = &( Entities[Component->EntityID] );
	
	glm::mat4 TranslateMatrix = glm::translate(Entity->Position3D);
	glm::mat4 ScaleMatrix = glm::scale(Entity->Scale3D);

	Component->TRS = TranslateMatrix * ScaleMatrix;
    }
}

internal void
UpdateRenderComponents(std::vector<render_component> &Components
		      , std::vector<model_matrix_component> &ModelMatrices
		      , std::vector<entity_data> &Entities)
{
    render_component *End = &( Components.back() ) + 1;
    for (render_component *Component = &( Components[0] )
	     ; Component != End
	     ; ++Component)
    {
	entity_data *Entity = &Entities[Component->EntityID];
	int32 ModelMatrixIndex = Entity->Components.ModelMatrixComponentIndex;

	if (ModelMatrixIndex != MISSING_COMPONENT)
	{
	    model_instance Instance{ Component->Model, ModelMatrices[ModelMatrixIndex].TRS };
	    Component->Layer->Instances.push_back(Instance);
	}
    }
}

internal void
UpdateKeyboardComponents(std::vector<keyboard_component> &Components
			 , std::vector<entity_data> &Entities)
{
    local_persist auto GetLateral = [](const glm::vec3 &Vector) -> glm::vec3
	{ return glm::normalize(glm::vec3(Vector.x, 0.0f, Vector.z)); };

    keyboard_component *End = &( Components.back() ) + 1;
    for (keyboard_component *Component = &( Components[0] )
	     ; Component != End
	     ; ++Component)
    {
	entity_data *Entity = &Entities[Component->EntityID];
	
	if (Entity->Components.KeyboardComponentIndex != MISSING_COMPONENT)
	{
	    glm::vec3 LateralDirection3D = GetLateral(Entity->Direction3D);

	    if (glm::all(glm::lessThan(Entity->Velocity3D
				       , glm::vec3(100000000.0f))))
	    {
		if (WindowData.KeyMap[GLFW_KEY_W])
		    Entity->Velocity3D += LateralDirection3D;
		if (WindowData.KeyMap[GLFW_KEY_A]) Entity->Velocity3D -= glm::cross(LateralDirection3D, glm::vec3(0.0f, 1.0f, 0.0f));
		if (WindowData.KeyMap[GLFW_KEY_S]) Entity->Velocity3D -= LateralDirection3D;
		if (WindowData.KeyMap[GLFW_KEY_D]) Entity->Velocity3D += glm::cross(LateralDirection3D, glm::vec3(0.0f, 1.0f, 0.0f));
		if (WindowData.KeyMap[GLFW_KEY_SPACE]) Entity->Velocity3D += glm::vec3(0.0f, 1.0f, 0.0f);
		if (WindowData.KeyMap[GLFW_KEY_LEFT_SHIFT]) Entity->Velocity3D += glm::vec3(0.0f, -1.0f, 0.0f);
	    }
	}
    }
}

// TODO Parameterize mouse sensitivity
#define MOUSE_SENSITIVITY 0.02f

internal void
UpdateMouseComponents(std::vector<mouse_component> &Components
		      , std::vector<entity_data> &Entities)
{
    local_persist auto RotateDirection = [](const glm::vec2 &CursorDifference
					    , const glm::vec3 &Direction
					    , window_data &WindowData) -> glm::vec3
    {
	glm::vec3 NewDirection = Direction;
	
	float XAngle = glm::radians(-CursorDifference.x) * MOUSE_SENSITIVITY;
	float YAngle = glm::radians(-CursorDifference.y) * MOUSE_SENSITIVITY;

	NewDirection = glm::mat3(glm::rotate(XAngle, glm::vec3(0.0f, 1.0f, 0.0f))) * NewDirection;

	glm::vec3 YRotateAxis = glm::cross(NewDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	NewDirection = glm::mat3(glm::rotate(YAngle, YRotateAxis)) * NewDirection;

	return NewDirection;
    };

    mouse_component *End = &( Components.back() ) + 1;
    for (mouse_component *Component = &( Components[0] )
	     ; Component != End
	     ; ++Component)
    {
	entity_data *Entity = &Entities[Component->EntityID];
	
	if (WindowData.MouseMoved)
	{
	    glm::vec2 Difference = WindowData.CurrentMousePosition - Component->PreviousMousePosition;
	    Component->PreviousMousePosition = WindowData.CurrentMousePosition;

	    Entity->Direction3D = normalize(RotateDirection(Difference
							    , Entity->Direction3D
							    , WindowData));
	}
    }
}

#define SPEED 5.0f

internal void
UpdatePhysicsComponents(std::vector<physics_component> &Components
			, std::vector<entity_data> &Entities)
{
    physics_component *End = &( Components.back() ) + 1;
    for (physics_component *Component = &( Components[0] )
	     ; Component != End
	     ; ++Component)
    {
	entity_data *Entity = &Entities[Component->EntityID];

	Entity->Position3D += Entity->Velocity3D * TimeData.Elapsed() * SPEED;

	Entity->Velocity3D = glm::vec3(0.0f);
    }
}

uint32
entity_data_base::CreateEntity(const string_view &Name)
{
    uint32 ID = Entities.size();
    IndexMap[Name] = ID;
    Entities.push_back(entity_data {});
    return ID;
}

entity_data *
entity_data_base::GetEntity(uint32 ID)
{
    return &Entities[ID];
}

void
entity_data_base::Update(void)
{
    UpdateModelMatrixComponents(ModelMatrixComponents.List
				, Entities);

    UpdateRenderComponents(RenderComponents.List
			  , ModelMatrixComponents.List
			  , Entities);

    UpdateKeyboardComponents(KeyboardComponents.List
			     , Entities);

    UpdateMouseComponents(MouseComponents.List,
			   Entities);

    UpdatePhysicsComponents(PhysicsComponents.List
			    , Entities);
}
