// TODO : TEST RERECORDING COMMAND BUFFERS EVERY FRAME !!! AND PUSH CONSTANTS !!!

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
	   , Window_Data *window
	   , Vulkan_API::State *vk)
{
    scene->user_camera.set_default(window->w, window->h, window->m_x, window->m_y);

    // initialize cmdbuf semaphores and fence
    Vulkan_API::allocate_command_pool(vk->gpu.queue_families.graphics_family
				      , &vk->gpu
				      , &scene->cmdpool);

    Vulkan_API::allocate_command_buffers(&scene->cmdpool
					 , VK_COMMAND_BUFFER_LEVEL_PRIMARY
					 , &vk->gpu
					 , Memory_Buffer_View<VkCommandBuffer>{1, &scene->cmdbuf});

    Vulkan_API::init_semaphore(&vk->gpu, &scene->img_ready);
    Vulkan_API::init_semaphore(&vk->gpu, &scene->rndr_finished);
    Vulkan_API::init_fence(&vk->gpu, VK_FENCE_CREATE_SIGNALED_BIT, &scene->cpu_wait);



    Rendering::init_rendering_system();

    Rendering::Renderer_Init_Data rndr_d = {};
    rndr_d.rndr_id = "renderer.test_material_renderer"_hash;
    rndr_d.mtrl_max = 3;
    rndr_d.ppln_id = "pipeline.main_pipeline"_hash;
    rndr_d.mtrl_unique_data_stage_dst = VK_SHADER_STAGE_VERTEX_BIT;

    allocate_memory_buffer(rndr_d.descriptor_sets, 1);
    rndr_d.descriptor_sets[0] = "descriptor_set.test_descriptor_sets"_hash;

    Rendering::add_renderer(&rndr_d);
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
					     , 1000.0f);

    ubo.projection_matrix[1][1] *= -1;

    Vulkan_API::Buffer &current_ubo = uniform_buffers.p[current_image];

    auto map = current_ubo.construct_map();
    map.begin(gpu);
    map.fill(Memory_Byte_Buffer{sizeof(ubo), &ubo});
    map.end(gpu);
}

internal void
record_cmd(Rendering::Rendering_State *rnd_objs
	   , Vulkan_API::State *vk
	   , Scene *scene
	   , u32 image_index, u32 frame_num)
{
    Vulkan_API::begin_command_buffer(&scene->cmdbuf, 0, nullptr);

    Vulkan_API::Registered_Render_Pass render_pass = rnd_objs->test_render_pass;
    Vulkan_API::Registered_Graphics_Pipeline pipeline_ptr = rnd_objs->graphics_pipeline;
    Vulkan_API::Registered_Framebuffer fbo = vk->swapchain.framebuffers.extract(image_index);
    Vulkan_API::Registered_Descriptor_Set descriptor_sets = rnd_objs->descriptor_sets;

    VkClearValue clears[2] {Vulkan_API::init_clear_color_color(0, 0, 0, 0), Vulkan_API::init_clear_color_depth(1.0f, 0)};
		
    Vulkan_API::command_buffer_begin_render_pass(render_pass.p
						 , fbo.p
						 , Vulkan_API::init_render_area({0, 0}, vk->swapchain.extent)
						 , Memory_Buffer_View<VkClearValue>{2, clears}
						 , VK_SUBPASS_CONTENTS_INLINE
						 , &scene->cmdbuf);

    Rendering::update_renderers(&scene->cmdbuf);
    
    /*Vulkan_API::command_buffer_bind_pipeline(pipeline_ptr.p, &scene->cmdbuf);

    VkDeviceSize offset[] = {0};
    Vulkan_API::command_buffer_bind_vbos(rnd_objs->test_model.p->raw_cache_for_rendering
					 , Memory_Buffer_View<VkDeviceSize>{rnd_objs->test_model.p->raw_cache_for_rendering.count, offset}
					 , 0, 1
					 , &scene->cmdbuf);

    Vulkan_API::command_buffer_bind_ibo(rnd_objs->test_model.p->index_data
					, &scene->cmdbuf);

    Vulkan_API::Descriptor_Set *descriptor_set = &descriptor_sets.p[image_index];
    Vulkan_API::command_buffer_bind_descriptor_sets(pipeline_ptr.p
						    , Memory_Buffer_View<VkDescriptorSet>{1, &descriptor_set->set}
						    , &scene->cmdbuf);

    glm::mat4 model = glm::scale(glm::vec3(3.0f));
    Vulkan_API::command_buffer_push_constant(&model
					     , sizeof(model)
					     , 0
					     , VK_SHADER_STAGE_VERTEX_BIT
					     , pipeline_ptr.p
					     , &scene->cmdbuf);

    model = glm::mat4(0.0f);

    Vulkan_API::Draw_Indexed_Data index_data;
    index_data.index_count = rnd_objs->test_model.p->index_data.index_count;
    index_data.instance_count = 1;
    index_data.first_index = 0;
    index_data.vertex_offset = 0;
    index_data.first_instance = 0;
    Vulkan_API::command_buffer_draw_indexed(&scene->cmdbuf
					    , index_data);*/

    Vulkan_API::command_buffer_end_render_pass(&scene->cmdbuf);
    Vulkan_API::end_command_buffer(&scene->cmdbuf);
}

internal void
render_frame(Rendering::Rendering_State *rendering_objects
	     , Vulkan_API::State *vulkan_state
	     , Scene *scene)
{
    persist u32 current_frame = 0;
    persist constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

#if 0
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

#else
    Vulkan_API::wait_fences(&vulkan_state->gpu, Memory_Buffer_View<VkFence>{1, &scene->cpu_wait});

    auto next_image_data = Vulkan_API::acquire_next_image(&vulkan_state->swapchain
							  , &vulkan_state->gpu
							  , &scene->img_ready
							  , &scene->cpu_wait);
    
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
    Vulkan_API::reset_fences(&vulkan_state->gpu, Memory_Buffer_View<VkFence>{1, &scene->cpu_wait});

    record_cmd(rendering_objects, vulkan_state, scene, next_image_data.image_index, current_frame);
    
    VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;;
	    
    Vulkan_API::submit(Memory_Buffer_View<VkCommandBuffer>{1, &scene->cmdbuf}
                               , Memory_Buffer_View<VkSemaphore>{1, &scene->img_ready}
                               , Memory_Buffer_View<VkSemaphore>{1, &scene->rndr_finished}
                               , Memory_Buffer_View<VkPipelineStageFlags>{1, &wait_stages}
                               , &scene->cpu_wait
                               , &vulkan_state->gpu.graphics_queue);
    
    VkSemaphore signal_semaphores[] = {scene->rndr_finished};

    Vulkan_API::present(Memory_Buffer_View<VkSemaphore>{1, &scene->rndr_finished}
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
#endif
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

    glm::vec3 d = glm::normalize(glm::vec3(scene->user_camera.d.x
					   , 0.0f
					   , scene->user_camera.d.z));
    
    glm::vec3 res = {};
	    
    if (window->key_map[GLFW_KEY_W]) acc_v(d, res);
    if (window->key_map[GLFW_KEY_A]) acc_v(-glm::cross(d, scene->user_camera.u), res);
    if (window->key_map[GLFW_KEY_S]) acc_v(-d, res);
    if (window->key_map[GLFW_KEY_D]) acc_v(glm::cross(d, scene->user_camera.u), res);
    if (window->key_map[GLFW_KEY_SPACE]) acc_v(scene->user_camera.u, res);
    if (window->key_map[GLFW_KEY_LEFT_SHIFT]) acc_v(-scene->user_camera.u, res);

    if (movements > 0)
    {
	res = res * 15.0f * dt;

	scene->user_camera.p += res;
    }
}
