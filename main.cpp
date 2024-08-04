/* ======================================================================== */
/* main.cpp                                                                 */
/* ======================================================================== */
/*                        This file is part of:                             */
/*                           COPILOT ENGINE                                 */
/* ======================================================================== */
/*                                                                          */
/* Copyright (C) 2022 Vcredent All rights reserved.                         */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License");          */
/* you may not use this file except in compliance with the License.         */
/*                                                                          */
/* You may obtain a copy of the License at                                  */
/*     http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied  */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/*                                                                          */
/* ======================================================================== */
#include "platform/win32/render_device_context_win32.h"
#include <vector>
#include "rendering/camera/projection_camera.h"
#include "rendering/camera/game_player_camera_controller.h"
#include "rendering/renderer_canvas.h"
#include "rendering/renderer_screen.h"
#include "rendering/renderer_graphics.h"
#include "rendering/renderer_axis_line.h"
#include "utils/fps_counter.h"
#include <navui.h>

Window *window;
RenderDeviceContext *rdc;
RenderDevice *rd;
RendererScreen *screen;
RendererCanvas *canvas;
RendererGraphics *graphics;
RendererAxisLine *axis_line;
RenderObject *object;
ProjectionCamera *camera;
CameraController *game_player_controller;
RenderDevice::Texture2D *canvas_preview_texture;
RenderDevice::Texture2D *canvas_depth_texture;
ImVec2 viewport_window_region = ImVec2(32.0f, 32.0f);
FPSCounter fps_counter;

static float mouse_scroll_xoffset = 0.0f;
static float mouse_scroll_yoffset = 0.0f;

struct DebugData {
    std::vector<float> fps_data;
    float canvas_render_time = 0.0f;
    float screen_render_time = 0.0f;
};

static DebugData debug;

#include <navui.h>

void _update_camera()
{
    // update camera
    static bool is_dragging = false;
    if (window->getkey(GLFW_KEY_F)) {
        is_dragging = true;
        window->hide_cursor();
    }

    if (window->getkey(GLFW_KEY_ESCAPE) && is_dragging) {
        window->show_cursor();
        is_dragging = false;
        game_player_controller->uncontinual();
    }

    if (is_dragging) {
        // key button
        game_player_controller->on_event_key(GLFW_KEY_W, window->getkey(GLFW_KEY_W));
        game_player_controller->on_event_key(GLFW_KEY_S, window->getkey(GLFW_KEY_S));
        game_player_controller->on_event_key(GLFW_KEY_A, window->getkey(GLFW_KEY_A));
        game_player_controller->on_event_key(GLFW_KEY_D, window->getkey(GLFW_KEY_D));

        // mouse
        float xpos = 0.0f;
        float ypos = 0.0f;
        window->get_cursor_position(&xpos, &ypos);
        game_player_controller->on_event_cursor(xpos, ypos);

        // scroll
        game_player_controller->on_event_scroll(mouse_scroll_xoffset, mouse_scroll_yoffset);
    }

    game_player_controller->on_update_camera();
}

void update()
{
    canvas->set_extent(viewport_window_region.x, viewport_window_region.y);
    camera->set_aspect_ratio(viewport_window_region.x / viewport_window_region.y);
    _update_camera();
}

void rendering()
{
    /* render to canvas */
    double canvas_render_start_time = glfwGetTime();
    VkCommandBuffer canvas_cmd_buffer;
    canvas->cmd_begin_canvas_render(&canvas_cmd_buffer);
    {
        Mat4 projection, view;

        projection = camera->get_projection_matrix();
        view = camera->get_view_matrix();

        axis_line->cmd_setval_viewport(canvas_cmd_buffer, canvas->get_width(), canvas->get_height());
        axis_line->cmd_draw_line(canvas_cmd_buffer, projection, view);
        graphics->cmd_begin_graphics_render(canvas_cmd_buffer);
        {
            graphics->cmd_setval_viewport(canvas_cmd_buffer, canvas->get_width(), canvas->get_height());
            graphics->cmd_setval_view_matrix(canvas_cmd_buffer, view);
            graphics->cmd_setval_projection_matrix(canvas_cmd_buffer, projection);
            graphics->cmd_draw_list(canvas_cmd_buffer);
        }
        graphics->cmd_end_graphics_render(canvas_cmd_buffer);
    }
    canvas->cmd_end_canvas_render();
    canvas_preview_texture = canvas->get_canvas_texture();
    canvas_depth_texture = canvas->get_canvas_depth();
    double canvas_render_end_time = glfwGetTime();

    double screen_render_start_time = glfwGetTime();
    VkCommandBuffer screen_cmd_buffer;
    screen->cmd_begin_screen_render(&screen_cmd_buffer);
    {
        /* ImGui */
        NavUI::BeginNewFrame(screen_cmd_buffer);
        {
            static bool show_demo_flag = true;
            ImGui::ShowDemoWindow(&show_demo_flag);

            NavUI::BeginViewport("视口");
            {
                static ImTextureID preview = NULL;
                if (preview != NULL)
                    NavUI::RemoveTexture(preview);

                static ImTextureID depth = NULL;
                if (depth != NULL)
                    NavUI::RemoveTexture(depth);

                preview = NavUI::AddTexture(canvas_preview_texture->sampler, canvas_preview_texture->image_view, canvas_preview_texture->image_layout);
                depth = NavUI::AddTexture(canvas_depth_texture->sampler, canvas_depth_texture->image_view, canvas_depth_texture->image_layout);

                // Main image
                {
                    viewport_window_region = ImGui::GetContentRegionAvail();
                    ImGui::Image(preview, ImVec2(canvas_preview_texture->width, canvas_preview_texture->height));
                }

                // Depth image
                {
                    ImVec2 position = ImGui::GetWindowPos();
                    ImVec2 size = ImGui::GetWindowSize();

                    ImVec2 offset = ImVec2(30.0f, 70.0f);
                    ImVec2 tex_size = ImVec2((size.x * 0.1f) * 1.3f, (size.y * 0.1f) * 1.3f);

                    ImVec2 depth_tex_pos = ImVec2(position.x + offset.x, position.y + size.y - tex_size.y - offset.y);

                    ImDrawList *draw = ImGui::GetWindowDrawList();
                    draw->AddImage(depth, depth_tex_pos, ImVec2(depth_tex_pos.x + tex_size.x, depth_tex_pos.y + tex_size.y));
                }
            }
            NavUI::EndViewport();

            ImGui::Begin("object");
            {
                ImGui::SeparatorText("变换");
                Vec3 position = object->get_object_position();
                NavUI::DragFloat3("平移: ", glm::value_ptr(position), 0.01f);
                object->set_object_position(position);

                Vec3 rotation = object->get_object_rotation();
                NavUI::DragFloat3("旋转: ", glm::value_ptr(rotation), 0.01f);
                object->set_object_rotation(rotation);

                Vec3 scaling = object->get_object_scaling();
                NavUI::DragFloat3("缩放: ", glm::value_ptr(scaling), 0.01f);
                object->set_object_scaling(scaling);
            }
            ImGui::End();

            ImGui::Begin("调试");
            {
                ImGui::SeparatorText("高级信息");
                ImGui::Indent(32.0f);
                ImGui::Text("canvas render time: %.2fms", debug.canvas_render_time);
                ImGui::Text("screen render time: %.2fms", debug.screen_render_time);
                ImGui::Text("total render time: %.2fms", debug.canvas_render_time + debug.screen_render_time);
                ImGui::Unindent(32.0f);

                ImGui::SeparatorText("基础信息");
                ImGui::Indent(32.0f);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "fps: %d", fps_counter.fps());
                ImGui::Indent(12.0f);
                ImGui::PlotLines("##", std::data(debug.fps_data), std::size(debug.fps_data), 0, NULL, 0.0f, 144.0f, ImVec2(0.0f, 32.0f));
                ImGui::Unindent(12.0f);
                ImGui::Unindent(32.0f);
            }
            ImGui::End();

            ImGui::Begin("摄像机");
            {
                ImGui::SeparatorText("变换");
                Vec3 position = camera->get_position();
                NavUI::DragFloat3("位置: ", glm::value_ptr(position), 0.01f);
                camera->set_position(position);

                Vec3 target = camera->get_target();
                NavUI::DragFloat3("目标: ", glm::value_ptr(target), 0.01f);
                camera->set_target(target);

                float fov = camera->get_fov();
                NavUI::DragFloat("景深: ", &fov, 0.01f);
                camera->set_fov(fov);

                float near = camera->get_near();
                NavUI::DragFloat("近点: ", &near, 0.01f);
                camera->set_near(near);

                float far = camera->get_far();
                NavUI::DragFloat("远点: ", &far, 0.01f);
                camera->set_far(far);

                float speed = camera->get_speed();
                NavUI::DragFloat("速度: ", &speed, 0.01f);
                camera->set_speed(speed);
            }
            ImGui::End();
        }
        NavUI::EndNewFrame(screen_cmd_buffer);
    }
    screen->cmd_end_screen_render(screen_cmd_buffer);
    double screen_render_end_time = glfwGetTime();

    // debug
    debug.screen_render_time = (screen_render_end_time - screen_render_start_time) * 1000.0f;
    debug.canvas_render_time = (canvas_render_end_time - canvas_render_start_time) * 1000.0f;
}

void initialize()
{
    window = memnew(Window, "CopilotEngine", 1980, 1080);

    window->set_window_scroll_callbacks([](Window *window, float x, float y) {
        mouse_scroll_xoffset = x;
        mouse_scroll_yoffset = y;
    });

    rdc = memnew(RenderDeviceContextWin32, window);
    // initialize
    rdc->initialize();
    rd = ((RenderDeviceContextWin32 *) rdc)->load_render_device();

    screen = memnew(RendererScreen, rd);
    screen->initialize(window);

    canvas = memnew(RendererCanvas, rd);
    canvas->initialize();

    graphics = memnew(RendererGraphics, rd);
    graphics->initialize(canvas->get_render_pass());

    axis_line = memnew(RendererAxisLine, rd);
    axis_line->initialize(canvas->get_render_pass());

    object = RenderObject::load_assets_obj("../assets/cube.obj");
    graphics->push_render_object(object);

    NavUI::InitializeInfo initialize_info = {};
    initialize_info.window = (GLFWwindow *) screen->get_focused_window()->get_native_window();
    initialize_info.Instance = rdc->get_instance();
    initialize_info.PhysicalDevice = rdc->get_physical_device();
    initialize_info.Device = rdc->get_device();
    initialize_info.QueueFamily = rdc->get_graph_queue_family();
    initialize_info.Queue = rdc->get_graph_queue();
    initialize_info.DescriptorPool = rd->get_descriptor_pool();
    initialize_info.RenderPass = screen->get_render_pass();
    initialize_info.MinImageCount = screen->get_image_buffer_count();
    initialize_info.ImageCount = screen->get_image_buffer_count();
    initialize_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    NavUI::Initialize(&initialize_info);

    camera = memnew(ProjectionCamera);
    game_player_controller = memnew(GamePlayerCameraController);
    game_player_controller->make_current_camera(camera);
    camera->set_position(Vec3(0.0f, 0.0f, 6.0f));
}

int main(int argc, char **argv)
{
    initialize();

    while (window->is_close()) {
        fps_counter.update();
        /* poll events */
        window->poll_events();
        update();
        rendering();
        mouse_scroll_xoffset = 0.0f;
        mouse_scroll_yoffset = 0.0f;

        // render data
        double end_time = glfwGetTime();

        debug.fps_data.push_back(fps_counter.fps());
        if (std::size(debug.fps_data) > 255) {
            debug.fps_data.erase(debug.fps_data.begin());
        }

    }

    memdel(object);
    memdel(axis_line);
    memdel(graphics);
    memdel(camera);
    memdel(screen);
    memdel(canvas);
    NavUI::Destroy();
    ((RenderDeviceContextWin32 *) rdc)->destroy_render_device(rd);
    memdel(rdc);
    memdel(window);

    return 0;
}