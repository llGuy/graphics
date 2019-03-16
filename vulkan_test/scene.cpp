#include "scene.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

constexpr f32 PI = 3.14159265359f;

void
Camera::set_default(f32 w, f32 h)
{
    mp = glm::vec2(0);
    p = glm::vec3(0);
    d = glm::vec3(1, 0, 0);
    u = glm::vec3(0, 1, 0);

    fov = 60.0f;
    asp = w / h;
    n = 0.1f;
    f = 10000.0f;
}

void
Camera::compute_projection(void)
{
    p_m = glm::perspective(fov, asp, n, f);
}

void
Camera::compute_view(void)
{
    v_m = glm::lookAt(p, p + d, u);
}

void
init_scene(Scene *scene
	   , Window *window)
{
    scene->user_camera.set_default(window->w, window->h);
}

void
update_scene(Scene *scene)
{
    
}

void
handle_input(Input_Type type
	     , class GLFWwindow *window
	     , const Input_Data &input_data
	     , Scene *scene)
{
    switch(type)
    {
    case Input_Type::MOUSE_BUTTON:
	{
	    
	} break;
	
    case Input_Type::MOUSE_MOVEMENT:
	{
	    
	} break;
	
    case Input_Type::WINDOW_RESIZE:
	{
	    
	} break;
	
    case Input_Type::KEYBOARD:
	{
	    if (input_data.k_value == GLFW_KEY_W);
	    if (input_data.k_value == GLFW_KEY_A);
	    if (input_data.k_value == GLFW_KEY_S);
	    if (input_data.k_value == GLFW_KEY_D);
	    if (input_data.k_value == GLFW_KEY_SPACE);
	    if (input_data.k_value == GLFW_KEY_LEFT_SHIFT);
	} break;
    }	
}
