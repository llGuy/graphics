#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "int.h"
#include "model.h"
#include "scene.h"
#include "global.h"

internal void
HandleKey(GLFWwindow *Window
	  , int32 Key
	  , int32 Scancode
	  , int32 Action
	  , int32 Mods)
{
    if (Key < MAX_KEYS)
    {
	if (Action == GLFW_PRESS)
	{
	    WindowData.KeyMap[Key] = true;
	}
	else if (Action == GLFW_RELEASE)
	{
	    WindowData.KeyMap[Key] = false;
	}
    }
}

internal void
HandleMouseMove(GLFWwindow *Window
		, double X
		, double Y)
{
    WindowData.CurrentMousePosition = glm::vec2((float)X, (float)Y);
    WindowData.MouseMoved = true;
}

internal void
HandleMB(GLFWwindow *Window
	 , int32 Button
	 , int32 Action
	 , int32 Mods)
{
    if (Action == GLFW_PRESS)
    {
	WindowData.MouseButtonMap[Button] = true;
    }
    else if (Action == GLFW_RELEASE)
    {
	WindowData.MouseButtonMap[Button] = false;
    }
};

int32 
main(int32 Argc
     , char *Argv[])
{
    if (!glfwInit())
    {
	printf("error initializing GLFW\n");
	return(0);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    WindowData.Width = 900;
    WindowData.Height = 600;
    WindowData.Window = glfwCreateWindow(WindowData.Width
					 , WindowData.Height
					 , "Prototype"
					 , nullptr
					 , nullptr);

    glfwSetInputMode(WindowData.Window
		     , GLFW_CURSOR
		     , GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(WindowData.Window
		       , HandleKey);
    glfwSetCursorPosCallback(WindowData.Window
			     , HandleMouseMove);
    glfwSetMouseButtonCallback(WindowData.Window
			       , HandleMB);
    
    glfwMakeContextCurrent(WindowData.Window);

    if (glewInit() != GLEW_OK)
    {
	printf("error inittializing GLEW\n");
    }

    
    main_scene MainScene;
    MainScene.Init();


    glEnable(GL_DEPTH_TEST);


    while(!glfwWindowShouldClose(WindowData.Window))
    {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.2, 0.2, 0.2, 1.0);


	/* Rendering */
	MainScene.Render(MainScene.Camera);


	glfwSwapBuffers(WindowData.Window);
	glfwPollEvents();

	/* Update Entities Before the scene updates */
	EntityDataBase.Update();
	
	TimeData.Reset();

	MainScene.Update();

	WindowData.MouseMoved = false;
    }

    glfwDestroyWindow(WindowData.Window);
    glfwTerminate();

    return(0);
}
