#pragma once

#include "core.hpp"
#include <glm/glm.hpp>

struct Camera
{
    glm::vec2 mp;
    glm::vec3 p; // position
    glm::vec3 d; // direction
    glm::vec3 u; // up

    f32 fov;
    f32 asp; // aspect ratio
    f32 n, f; // near and far planes

    glm::mat4 p_m;
    glm::mat4 v_m;

    void
    set_default(f32 w, f32 h);
    
    void
    compute_projection(void);

    void
    compute_view(void);
};

struct Scene
{
    Camera user_camera;
};

void
init_scene(Scene *scene
	   , Window *window);

void
update_scene(Scene *scene);

// for the moment : only handles GLFW key codes
enum Input_Type {MOUSE_BUTTON, MOUSE_MOVEMENT, WINDOW_RESIZE, KEYBOARD /* ... */};

struct Input_Data
{
    union
    {
	struct
	{
	    s32 k_value;
	    s32 k_scancode;
	    s32 k_action;
	    s32 k_mods;
	};
	struct
	{
	    s32 mb_button;
	    s32 mb_action;
	    s32 mb_mods;
	};
	struct
	{
	    f32 mm_x;
	    f32 mm_y;
	};
	struct
	{
	    s32 r_w;
	    s32 r_h;
	};
    };
};

void
handle_input(Input_Type type
	     , class GLFWwindow *window
	     , const Input_Data &input_data);
