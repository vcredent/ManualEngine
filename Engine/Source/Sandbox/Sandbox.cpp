/* ------------------------------------------------------------------------ */
/* Sandbox.h                                                                */
/* ------------------------------------------------------------------------ */
/*                        This file is part of:                             */
/*                            BRIGHT ENGINE                                 */
/* ------------------------------------------------------------------------ */
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
/* ------------------------------------------------------------------------ */
#include <Runtime/Win32/RenderDeviceCotnextWin32.h>
#include <Runtime/Camera/Camera.h>
#include <ImGuiNav/ImGuiNav.h>

int main()
{
    Window *window = new Window("TurbineEngine", 1080, 1060);
    RenderDeviceContextWin32 *rdc = new RenderDeviceContextWin32(window);
    RenderDevice *rd = rdc->CreateRenderDevice();
    Displayer *displayer = new Displayer(rd, window);
    ImGuiNav::Initialize(displayer);

    Camera camera;

    while (!window->IsClose()) {
        window->PollEvents();

        VkCommandBuffer cmdBuffer;
        displayer->CmdBeginDisplayRendering(&cmdBuffer);
        {
            ImGuiNav::BeginNewFrame(cmdBuffer);
            static bool showDemoWindow = true;
            ImGui::ShowDemoWindow(&showDemoWindow);
            ImGuiNav::EndNewFrame(cmdBuffer);
        }
        displayer->CmdEndDisplayRendering(cmdBuffer);
    }

    ImGuiNav::Terminate();
    delete displayer;
    rdc->DestroyRenderDevice(rd);
    delete rdc;
    delete window;

    return 0;
}