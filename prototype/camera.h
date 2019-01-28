#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <glm/glm.hpp>

struct camera
{
    glm::vec3 Position3D;
    glm::vec3 Direction3D;
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
};

extern glm::mat4
Look(const glm::vec3 &Position3D
     , const glm::vec3 &Direction3D
     , const glm::vec3 &Up);

#endif
