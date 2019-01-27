#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "int.h"
#include <glm/glm.hpp>

#define MAX_KEYS 360
#define MAX_MB   5

extern struct window_data
{
    class GLFWwindow *Window;

    bool KeyMap[MAX_KEYS];
    
    bool MouseButtonMap[MAX_MB];
    glm::vec2 CurrentMousePosition;
    bool MouseMoved;
} WindowData;

#endif
