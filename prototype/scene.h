#ifndef _SCENE_H_
#define _SCENE_H_

#include "model.h"
#include "render.h"
#include "camera.h"
#include "entity.h"

struct main_scene
{
    struct main_layer
    {
	uint32 ProjectionMatrixLoc;
	uint32 ViewMatrixLoc;
	uint32 ModelMatrixLoc;

	render_layer3D Layer;
    } MainLayer;

    camera Camera;

    struct scene_entities
    {
	uint32 MainPlayerID;

	uint32 BoxID;
    } Entities;

    void
    Init(void);

    void
    InitEntities(void);

    ogl_program *
    InitShader(void);

    void
    Render(camera &Camera);

    void 
    Update(void);
};

#endif
