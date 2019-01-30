#include <array>
#include <sstream>
#include <fstream>
#include "model.h"

model_data_base ModelDataBase = {};

uint32
model_data_base::CreateModel(const string_view &Name)
{
    IndexMap[Name] = Models.size();
    Models.push_back(model_data{ model_components{}, Models.size(), GL_TRIANGLES });
    return Models.back().ID;
}

model_data *
model_data_base::GetModel(uint32 ID)
{
    return &Models[ID];
}

uint32
model_data_base::GetModelID(const string_view &Name)
{
    return IndexMap[Name];
}

ogl_vbo *
model_data_base::GetVBOComponent(uint32 ID)
{
    return &VBOs[Models[ID].Components.VBOIndex].Component;
}

ogl_ibo *
model_data_base::GetIBOComponent(uint32 ID)
{
    return &IBOs[Models[ID].Components.IBOIndex].Component;
}

ogl_vbo *
model_data_base::GetNBOComponent(uint32 ID)
{
    return &NBOs[Models[ID].Components.NBOIndex].Component;
}

ogl_vbo *
model_data_base::GetTBOComponent(uint32 ID)
{
    return &TBOs[Models[ID].Components.TBOIndex].Component;
}

vertex_list *
model_data_base::GetVertex3DListComponent(uint32 ID)
{
    return &VerticesLists[Models[ID].Components.VerticesListIndex].Component;
}

internal std::vector<std::string>
Split(const std::string &String
      , const char Splitter)
{
	std::vector<std::string> Words;
	std::string Current;
	std::istringstream iss(String);
	while (std::getline(iss, Current, Splitter)) Words.push_back(Current);

	return Words;
}

internal void
ProcessVertex(const std::vector<std::string> &VertexData
	      , std::vector<uint32> &Indices
	      , const std::vector<glm::vec2> &RawUVs
	      , const std::vector<glm::vec3> &RawNormals
	      , std::vector<glm::vec2> &UVs
	      ,  std::vector<glm::vec3> &Normals
	      , uint32 & TriangleVertex)
{
	int32 CurrentVertex = std::stoi(VertexData[0]) - 1;
	Indices.push_back(CurrentVertex);

	if (RawUVs.size() > 0)
	{
		glm::vec2 CurrentUV = RawUVs[std::stoi(VertexData[1]) - 1];
		UVs[CurrentVertex] = CurrentUV;
	}

	if (RawNormals.size() > 0)
	{
		glm::vec3 CurrentNormal = RawNormals[std::stoi(VertexData[2]) - 1];
		float NormalY = CurrentNormal.y;
		float NormalZ = CurrentNormal.z;

		Normals[CurrentVertex] = CurrentNormal;
	}

	TriangleVertex = CurrentVertex;
}

internal void
BreakFaceLine(const std::vector<std::string> &FaceLineWords
	      , std::vector<uint32> &Indices
	      , const std::vector<glm::vec2> &RawUVs
	      , const std::vector<glm::vec3> &RawNormals
	      , std::vector<glm::vec2> &UVs
	      , std::vector<glm::vec3> &Normals
	      , std::vector<glm::vec3> &Tangents
	      , std::vector<float> &TangentAmounts
	      , std::vector<glm::vec3> &Vertices)
{
	std::array<std::vector<std::string>, 3> FaceIndices;

	std::array<uint32, 3> TriangleVertices;

	for (uint32 i = 0; i < 3; ++i)
	{
		FaceIndices[i] = Split(FaceLineWords[i + 1], '/');
		ProcessVertex(FaceIndices[i], Indices, RawUVs, RawNormals, UVs, Normals, TriangleVertices[i]);
	}

	glm::vec3 DeltaPos1 = Vertices[TriangleVertices[1]] - Vertices[TriangleVertices[0]];
	glm::vec3 DeltaPos2 = Vertices[TriangleVertices[2]] - Vertices[TriangleVertices[0]];

	glm::vec2 DeltaUV1 = UVs[TriangleVertices[1]] - UVs[TriangleVertices[0]];
	glm::vec2 DeltaUV2 = UVs[TriangleVertices[2]] - UVs[TriangleVertices[0]];

	float r = 1.0f / (DeltaUV1.x * DeltaUV2.y - DeltaUV1.y * DeltaUV2.x);
	DeltaPos1 *= DeltaUV2.y;
	DeltaPos2 *= DeltaUV1.y;
	glm::vec3 Tangent = DeltaPos1 - DeltaPos2;
	Tangent *= r;

	Tangents[TriangleVertices[0]] = Tangent;
	Tangents[TriangleVertices[1]] = Tangent;
	Tangents[TriangleVertices[2]] = Tangent;

	++TangentAmounts[TriangleVertices[0]];
	++TangentAmounts[TriangleVertices[1]];
	++TangentAmounts[TriangleVertices[2]];
}

internal void
CreateModel(std::vector<glm::vec3> &Vertices
	    , std::vector<glm::vec3> &Normals
	    , std::vector<glm::vec2> &UVs
	    , std::vector<uint32> &Indices
	    , std::vector<glm::vec3> &Tangents
	    , uint32 ID)
{
    model_data *Model = ModelDataBase.GetModel(ID);

    Model->VAO.Create();
    Model->VAO.Bind();

    ogl_vbo VBO;
    /*    uint32 BufferTest;
    glGenBuffers(1, &BufferTest);
    glBindBuffer(GL_ARRAY_BUFFER, BufferTest);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * Vertices.size(), Vertices.data(), GL_STATIC_DRAW);
    VBO.ID = BufferTest;*/
    VBO.Create();
    VBO.Bind(GL_ARRAY_BUFFER);
    VBO.Fill(sizeof(glm::vec3) * Vertices.size()
	     , &Vertices[0]
	     , GL_STATIC_DRAW
	     , GL_ARRAY_BUFFER);
    ogl_attribute_create_info VertexAttribute{ 0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr };
    Model->VAO.Attach(VBO, VertexAttribute);

    ogl_vbo NBO;
    NBO.Create();
    NBO.Bind(GL_ARRAY_BUFFER);
    NBO.Fill(sizeof(glm::vec3) * Normals.size()
	     , &Normals[0]
	     , GL_STATIC_DRAW
	     , GL_ARRAY_BUFFER);
    ogl_attribute_create_info NormalAttribute{ 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr };
    Model->VAO.Attach(NBO, NormalAttribute);

    ogl_vbo UVBO;
    UVBO.Create();
    UVBO.Bind(GL_ARRAY_BUFFER);
    UVBO.Fill(sizeof(glm::vec2) * UVs.size()
	     , &UVs[0]
	     , GL_STATIC_DRAW
	     , GL_ARRAY_BUFFER);
    ogl_attribute_create_info UVAttribute{ 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr };
    Model->VAO.Attach(UVBO, UVAttribute);

    ogl_vbo TBO;
    TBO.Create();
    TBO.Bind(GL_ARRAY_BUFFER);
    TBO.Fill(sizeof(glm::vec3) * Tangents.size()
	     , &Tangents[0]
	     , GL_STATIC_DRAW
	     , GL_ARRAY_BUFFER);
    ogl_attribute_create_info TangentAttribute{ 3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr };
    Model->VAO.Attach(TBO, TangentAttribute);

    ogl_ibo IBO;
    IBO.Create();
    IBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
    IBO.Fill(sizeof(uint32) * Indices.size()
	     , &Indices[0]
	     , GL_STATIC_DRAW
	     , GL_ELEMENT_ARRAY_BUFFER);
    IBO.Count = Indices.size();
    IBO.Type = GL_UNSIGNED_INT;
    IBO.Start = nullptr;

    ModelDataBase.VBOs.push_back(model_component<ogl_vbo>{ ID, VBO });
    ModelDataBase.NBOs.push_back(model_component<ogl_vbo>{ ID, NBO });
    ModelDataBase.UVBOs.push_back(model_component<ogl_vbo>{ ID, UVBO });
    ModelDataBase.TBOs.push_back(model_component<ogl_vbo>{ ID, TBO });
    ModelDataBase.IBOs.push_back(model_component<ogl_ibo>{ ID, IBO });
}

internal void
LoadOBJ(const char *Filename
	, uint32 ID)
{
    std::ifstream File(Filename);

    if (!File.good())
    {
	printf("Error loading file %s", Filename);
    }

    std::vector<glm::vec3> Normals;
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec2> UVs;
    std::vector<glm::vec3> Tangents;
    std::vector<float> TangentAmounts;

    /* won't be in the  correct order */
    std::vector<glm::vec2> RawUVs;
    std::vector<glm::vec3> RawNormals;
    std::vector<uint32> Indices;

    std::string Line;
    while (std::getline(File, Line))
    {
	std::vector<std::string> Words = Split(Line, ' ');

	if (Words[0] == "v")
	{
	    glm::vec3 Vertex;
	    for (uint32 i = 0; i < 3; ++i) Vertex[i] = std::stof(Words[i + 1]);
	    Vertices.push_back(Vertex);
	}
	else if (Words[0] == "vt")
	{
	    glm::vec2 UV;
	    for (uint32 i = 0; i < 2; ++i) UV[i] = std::stof(Words[i + 1]);
	    RawUVs.push_back(glm::vec2(UV.x, 1.0f - UV.y));
	}
	else if (Words[0] == "vn")
	{
	    glm::vec3 Normal;
	    for (uint32 i = 0; i < 3; ++i) Normal[i] = std::stof(Words[i + 1]);
	    RawNormals.push_back(Normal);
	}
	else if (Words[0] == "f")
	{
	    Tangents.resize(Vertices.size());
	    TangentAmounts.resize(Vertices.size());
	    Normals.resize(Vertices.size());
	    UVs.resize(Vertices.size());

	    BreakFaceLine(Words, Indices, RawUVs, RawNormals, UVs, Normals, Tangents, TangentAmounts, Vertices);

	    break;
	}
    }

    while (std::getline(File, Line))
    {
	std::vector<std::string> Words = Split(Line, ' ');
	if (Words[0] == "f")
	{
	    BreakFaceLine(Words, Indices, RawUVs, RawNormals, UVs, Normals, Tangents, TangentAmounts, Vertices);
	}
    }

    for (uint32 i = 0; i < Tangents.size(); ++i)
    {
	Tangents[i] /= TangentAmounts[i];
    }

    CreateModel(Vertices, Normals, UVs, Indices, Tangents, ID);
}

void
model_data_base::LoadModel(const char* Format
	  , const char *Filename
	  , uint32 ID)
{
    if (strcmp(Format, "OBJ") == 0)
    {
	LoadOBJ(Filename
		, ID);
    }
}
