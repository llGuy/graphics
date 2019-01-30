
#include <fstream>
#include "global.h"
#include "scene.h"
#include <iostream>
#include <glm/gtx/transform.hpp>

ogl_program *
main_scene::InitShader(void)
{
    ogl_shader Vsh { 0, GL_VERTEX_SHADER };
    Vsh.Create();
    
    std::fstream VFile("glsl/test_vertex.glsl");
    auto VshSource = std::string(std::istreambuf_iterator<char>(VFile)
				 , std::istreambuf_iterator<char>());
    const char *Sources[1] { VshSource.c_str() };
    ogl_shader_compilation_info VshCompileInfo = Vsh.Compile(Sources
							     , 1);

    if (VshCompileInfo.Status == COMPILATION_ERROR)
    {
	printf("Compile Error %s :\n%s", "glsl/test_vertex.glsl", VshCompileInfo.Message);
    }

    ogl_shader Fsh { 0, GL_FRAGMENT_SHADER };
    Fsh.Create();
    
    std::fstream FFile("glsl/test_fragment.glsl");
    auto FshSource = std::string(std::istreambuf_iterator<char>(FFile)
				 , std::istreambuf_iterator<char>());
    Sources[0] = FshSource.c_str();
    ogl_shader_compilation_info FshCompileInfo = Fsh.Compile(Sources
							     , 1);

    if (FshCompileInfo.Status == COMPILATION_ERROR)
    {
	printf("Compile Error %s :\n%s", "glsl/test_fragment.glsl", FshCompileInfo.Message);
    }

    ogl_program *Program = new ogl_program;
    
    Program->Create();
    Program->AttachShader(Vsh);
    Program->AttachShader(Fsh);
    
    ogl_shader_compilation_info LinkInfo = Program->Link();
    if (LinkInfo.Status == COMPILATION_ERROR)
    {
	printf("Link Error: \n%s", LinkInfo.Message);
    }

    return Program;
}

void
main_scene::InitEntities(void)
{
    {
	Entities.MainPlayerID = EntityDataBase.CreateEntity("Entity.Main"_hash);
	uint32 KBCompIndex = EntityDataBase.KeyboardComponents.Add(Entities.MainPlayerID
								   , keyboard_component());
	uint32 MBCompIndex = EntityDataBase.MouseComponents.Add(Entities.MainPlayerID
								, mouse_component());
	uint32 PHCompIndex = EntityDataBase.PhysicsComponents.Add(Entities.MainPlayerID
								  , physics_component());

	entity_data *MainPlayerData = EntityDataBase.GetEntity(Entities.MainPlayerID);
	MainPlayerData->Components.KeyboardComponentIndex = KBCompIndex;
	MainPlayerData->Components.MouseComponentIndex = MBCompIndex;
	MainPlayerData->Components.PhysicsComponentIndex = PHCompIndex;

	MainPlayerData->Position3D = glm::vec3(0.0f, 0.0f, 0.0f);
	MainPlayerData->Direction3D = glm::vec3(1.0f, 0.0f, 0.0f);
    } 


    {
	Entities.BoxID = EntityDataBase.CreateEntity("Entity.Cube"_hash);
	uint32 ModelMatrixCompIndex = EntityDataBase.ModelMatrixComponents.Add(Entities.BoxID
									       , model_matrix_component());
	render_component BoxRenderComponent;
	uint32 CubeModelID = ModelDataBase.CreateModel("Model.Cube"_hash);
	ModelDataBase.LoadModel("OBJ"
				, "res/cube.obj"
				, CubeModelID);
	BoxRenderComponent.Layer = &MainLayer.Layer;
	BoxRenderComponent.Model = CubeModelID;
	uint32 RenderCompIndex = EntityDataBase.RenderComponents.Add(Entities.BoxID
								     , BoxRenderComponent);

	entity_data *BoxData = EntityDataBase.GetEntity(Entities.BoxID);
	BoxData->Position3D = glm::vec3(5.0f, 0.0f, 0.0f);
	BoxData->Direction3D = glm::vec3(1.0f, 0.0f, 0.0f);
	BoxData->Scale3D = glm::vec3(1.0f, 1.0f, 1.0f);
	BoxData->Components.RenderComponentIndex = RenderCompIndex;
	BoxData->Components.ModelMatrixComponentIndex = ModelMatrixCompIndex;
    }
}

void
main_scene::Init(void)
{
    ogl_program *Program = InitShader();

    Program->Bind();
    MainLayer.ProjectionMatrixLoc = Program->GetUniformLocation("ProjectionMatrix");
    MainLayer.ViewMatrixLoc = Program->GetUniformLocation("ViewMatrix");
    MainLayer.ModelMatrixLoc = Program->GetUniformLocation("ModelMatrix");

    MainLayer.Layer.Program = Program;
    MainLayer.Layer.Texture = nullptr;

    InitEntities();

    Camera.ProjectionMatrix = glm::perspective(glm::radians(50.0f)
					       , (float)WindowData.Width / (float)WindowData.Height
					       , 0.1f
					       , 100000.0f);
}

void
main_scene::Render(camera &Camera)
{
    MainLayer.Layer.Program->Bind();

    MainLayer.Layer.Program->SendUniformMat4(MainLayer.ProjectionMatrixLoc
					     , &Camera.ProjectionMatrix[0][0]
					     , 1);
    MainLayer.Layer.Program->SendUniformMat4(MainLayer.ViewMatrixLoc
					     , &Camera.ViewMatrix[0][0]
					     , 1);
    
    for (uint32 Instance = 0
	     ; Instance < MainLayer.Layer.Instances.size()
	     ; ++Instance)
    {
	model_instance *ModelInstance = &MainLayer.Layer.Instances[Instance];
       
	MainLayer.Layer.Program->SendUniformMat4(MainLayer.ModelMatrixLoc
						 , &ModelInstance->TRS[0][0]
						 , 1);
	
	model_data *ModelData = ModelDataBase.GetModel(ModelInstance->ModelID);

	ModelData->VAO.Bind();

	ogl_ibo *IBO = ModelDataBase.GetIBOComponent(ModelInstance->ModelID);
	IBO->Bind(GL_ELEMENT_ARRAY_BUFFER);

	glDrawElements(ModelData->Primitive
		       , IBO->Count
		       , IBO->Type
		       , IBO->Start);

	UnbindBuffers(GL_ELEMENT_ARRAY_BUFFER);
	UnbindVAOs();
    }
}

void 
main_scene::Update(void)
{
    entity_data *MainPlayer = EntityDataBase.GetEntity(Entities.MainPlayerID);
    Camera.ViewMatrix = Look(MainPlayer->Position3D
			     , MainPlayer->Position3D + MainPlayer->Direction3D
			     , glm::vec3(0.0f, 1.0f, 0.0f));
    Camera.Position3D = MainPlayer->Position3D;
    Camera.Direction3D = MainPlayer->Direction3D;
}
