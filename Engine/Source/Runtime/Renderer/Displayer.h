/* ------------------------------------------------------------------------ */
/* Displayer.h                                                              */
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
#ifndef _RENDERING_SCREEN_H_
#define _RENDERING_SCREEN_H_

#include "Drivers/RenderDevice.h"
#include "Window/Window.h"

class Displayer {
public:
    Displayer(RenderDevice *vRenderDevice, Window *vWindow);
   ~Displayer();

    RenderDevice *GetRenderDevice() { return rd; }
    VkRenderPass GetRenderPass() { return displayWindow->renderPass; }
    uint32_t GetImageBufferCount() { return displayWindow->imageBufferCount; }
    Window *GetFocusedWindow() { return currentFocusedWindow; }
    void *GetNativeWindow() { return currentFocusedWindow->GetNativeWindow(); }

    void BeginDisplayRendering(VkCommandBuffer *pCmdBuffer);
    void EndDisplayRendering();

private:
    void _Initialize();

    struct SwapchainResource {
        VkCommandBuffer cmdBuffer;
        VkImage image;
        VkImageView image_view;
        VkFramebuffer framebuffer;
    };

    struct DisplayWindow {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        uint32_t imageBufferCount;
        VkCompositeAlphaFlagBitsKHR compositeAlpha;
        VkPresentModeKHR presentMode;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        SwapchainResource *swapchainResources;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        uint32_t width;
        uint32_t height;
    };

    void _CreateSwapChain();
    void _CleanUpSwapChain();
    void _UpdateSwapChain();

    RenderDevice *rd = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    DisplayWindow *displayWindow = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    Window *currentFocusedWindow = VK_NULL_HANDLE;
    VkCommandBuffer currentCmdBuffer;

    uint32_t acquireNextIndex;
};

#endif /* _RENDERING_SCREEN_H_ */
