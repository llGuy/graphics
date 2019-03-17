#include <chrono>
#include "scene.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

constexpr f32 PI = 3.14159265359f;

void
Camera::set_default(f32 w, f32 h, f32 m_x, f32 m_y)
{
    mp = glm::vec2(m_x, m_y);
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
	   , Window_Data *window)
{
    scene->user_camera.set_default(window->w, window->h, window->m_x, window->m_y);
}

internal void
update_ubo(u32 current_image
	   , Vulkan_API::GPU *gpu
	   , Vulkan_API::Swapchain *swapchain
	   , Vulkan_API::Registered_Buffer &uniform_buffers
	   , Scene *scene)
{
    struct Uniform_Buffer_Object
    {
	alignas(16) glm::mat4 model_matrix;
	alignas(16) glm::mat4 view_matrix;
	alignas(16) glm::mat4 projection_matrix;
    };
	
    persist auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    Uniform_Buffer_Object ubo = {};

    ubo.model_matrix = glm::rotate(time * glm::radians(90.0f)
				   , glm::vec3(0.0f, 0.0f, 1.0f));
    /*    ubo.view_matrix = glm::lookAt(glm::vec3(2.0f)
				  , glm::vec3(0.0f)
				  , glm::vec3(0.0f, 0.0f, 1.0f));*/
    ubo.view_matrix = scene->user_camera.v_m;
    ubo.projection_matrix = glm::perspective(glm::radians(60.0f)
					     , (float)swapchain->extent.width / (float)swapchain->extent.height
					     , 0.1f
					     , 10.0f);

    ubo.projection_matrix[1][1] *= -1;

    Vulkan_API::Buffer &current_ubo = uniform_buffers.p[current_image];

    auto map = current_ubo.construct_map();
    map.begin(gpu);
    map.fill(Memory_Byte_Buffer{sizeof(ubo), &ubo});
    map.end(gpu);
}

internal void
render_frame(Rendering::Rendering_State *rendering_objects
	     , Vulkan_API::State *vulkan_state
	     , Scene *scene)
{
    persist u32 current_frame = 0;
    persist constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    
    Vulkan_API::Registered_Command_Buffer &command_buffers = rendering_objects->command_buffers;
    Vulkan_API::Registered_Fence &fences = rendering_objects->fences;
    Vulkan_API::Registered_Semaphore &image_ready_semaphores = rendering_objects->image_ready_semaphores;
    Vulkan_API::Registered_Semaphore &render_finished_semaphores = rendering_objects->render_finished_semaphores;

    Vulkan_API::wait_fences(&vulkan_state->gpu, Memory_Buffer_View<VkFence>{1, &fences.p[current_frame]});

    auto next_image_data = Vulkan_API::acquire_next_image(&vulkan_state->swapchain
							  , &vulkan_state->gpu
							  , &image_ready_semaphores.p[current_frame]
							  , &fences.p[current_frame]);
    
    if (next_image_data.result == VK_ERROR_OUT_OF_DATE_KHR)
    {
	// recreate swapchain
	return;
    }
    else if (next_image_data.result != VK_SUCCESS && next_image_data.result != VK_SUBOPTIMAL_KHR)
    {
	OUTPUT_DEBUG_LOG("%s\n", "failed to acquire swapchain image");
    }

    update_ubo(next_image_data.image_index
	       , &vulkan_state->gpu
	       , &vulkan_state->swapchain
	       , rendering_objects->uniform_buffers
	       , scene);

    // KEEP EYE ON PERFORMANCE HERE!!
    Vulkan_API::reset_fences(&vulkan_state->gpu, Memory_Buffer_View<VkFence>{1, &fences.p[current_frame]});

    VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;;
	    
    Vulkan_API::submit(Memory_Buffer_View<VkCommandBuffer>{1, &command_buffers.p[next_image_data.image_index]}
                               , Memory_Buffer_View<VkSemaphore>{1, &image_ready_semaphores.p[current_frame]}
                               , Memory_Buffer_View<VkSemaphore>{1, &render_finished_semaphores.p[current_frame]}
                               , Memory_Buffer_View<VkPipelineStageFlags>{1, &wait_stages}
                               , &fences.p[current_frame]
                               , &vulkan_state->gpu.graphics_queue);
    
    VkSemaphore signal_semaphores[] = {render_finished_semaphores.p[current_frame]};

    Vulkan_API::present(Memory_Buffer_View<VkSemaphore>{1, &render_finished_semaphores.p[current_frame]}
                                , Memory_Buffer_View<VkSwapchainKHR>{1, &vulkan_state->swapchain.swapchain}
                                , &next_image_data.image_index
                                , &vulkan_state->gpu.present_queue);
    
    if (next_image_data.result == VK_ERROR_OUT_OF_DATE_KHR || next_image_data.result == VK_SUBOPTIMAL_KHR)
    {
	// recreate swapchain
    }
    else if (next_image_data.result != VK_SUCCESS)
    {
	OUTPUT_DEBUG_LOG("%s\n", "failed to present swapchain image");
    }
    
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void
update_scene(Scene *scene
	     , Window_Data *window
	     , Rendering::Rendering_State *rnd
	     , Vulkan_API::State *vk
	     , f32 dt)
{
    handle_input(scene, window, dt);
    scene->user_camera.compute_view();
    
    render_frame(rnd, vk, scene);
}

void
handle_input(Scene *scene
	     ,Window_Data *window
	     , f32 dt)
{
    if (window->m_moved)
    {
#define SENSITIVITY 15.0f
    
	glm::vec2 prev_mp = scene->user_camera.mp;
	glm::vec2 curr_mp = glm::vec2(window->m_x, window->m_y);

	glm::vec3 res = scene->user_camera.d;
	    
	glm::vec2 d = (curr_mp - prev_mp);

	f32 x_angle = glm::radians(-d.x) * SENSITIVITY * dt;// *elapsed;
	f32 y_angle = glm::radians(-d.y) * SENSITIVITY * dt;// *elapsed;
	res = glm::mat3(glm::rotate(x_angle, scene->user_camera.u)) * res;
	glm::vec3 rotate_y = glm::cross(res, scene->user_camera.u);
	res = glm::mat3(glm::rotate(y_angle, rotate_y)) * res;

	scene->user_camera.d = res;
	    
	scene->user_camera.mp = curr_mp;
    }

    u32 movements = 0;
    auto acc_v = [&movements](const glm::vec3 &d, glm::vec3 &dst){ ++movements; dst += d; };

    glm::vec3 res = {};
	    
    if (window->key_map[GLFW_KEY_W]) acc_v(scene->user_camera.d, res);
    if (window->key_map[GLFW_KEY_A]) acc_v(-glm::cross(scene->user_camera.d, scene->user_camera.u), res);
    if (window->key_map[GLFW_KEY_S]) acc_v(-scene->user_camera.d, res);
    if (window->key_map[GLFW_KEY_D]) acc_v(glm::cross(scene->user_camera.d, scene->user_camera.u), res);
    if (window->key_map[GLFW_KEY_SPACE]) acc_v(scene->user_camera.u, res);
    if (window->key_map[GLFW_KEY_LEFT_SHIFT]) acc_v(-scene->user_camera.u, res);

    if (movements > 0)
    {
	res = res * 15.0f * dt;

	scene->user_camera.p += res;
    }
}
