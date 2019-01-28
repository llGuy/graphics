#include "camera.h"
#include <glm/gtx/transform.hpp>

glm::mat4
Look(const glm::vec3 &Position3D
     , const glm::vec3 &Direction3D
     , const glm::vec3 &Up)
{
    return glm::lookAt(Position3D
		       , Position3D + Direction3D
		       , Up);
}
