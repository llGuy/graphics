#include "entity.h"
#include "render.h"
#include <glm/gtx/transform.hpp>

entity_data_base EntityDataBase = {};

internal void
UpdateModelMatrixComponents(std::vector<model_matrix_component> &Components
			    , std::vector<entity_data> &Entities)
{
    model_matrix_component *End = &( Components.back() );
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
    render_component *End = &( Components.back() );
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
    UpdateModelMatrixComponents(ModelMatrixComponents
				, Entities);

    UpdateRenderComponent(RenderComponents
			  , ModelMatrixComponents
			  , Entities);
}
