#ifndef _RENDER_H_
#define _RENDER_H_

#include "int.h"
#include "model.h"
#include "opengl_api.h"

struct model_instance
{
    uint32 ModelID;
    glm::mat4 TRS;
};

struct render_layer
{
    ogl_program *Program;
    ogl_texture *Texture;
 
    std::vector<model_instance> Instances;
};

#endif
