#include "App.hpp"
#include "CoreConstants.hpp"

#include <VulkanHelpers.hpp>

#include <algorithm>

void Core::createSwapchain(Instance* inst, VkSwapchainKHR oldSwapchain) {

    // Swapchain creation 
    {

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.flags = 0;
        createInfo.surface = inst->rend.surface;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(inst->rend.physicalDevice, inst->rend.surface, &capabilities);

        CORE_ASSERT(c_vlkn::imageCount >= capabilities.minImageCount);

        createInfo.minImageCount   = c_vlkn::imageCount;
        createInfo.imageFormat     = c_vlkn::format;
        createInfo.imageColorSpace = c_vlkn::colorSpace;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            createInfo.imageExtent = capabilities.currentExtent;
        }
        else {
            createInfo.imageExtent = {
                std::clamp((unsigned)inst->wind.m_size.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp((unsigned)inst->wind.m_size.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };

        }

        inst->rend.windowImageExtent = createInfo.imageExtent;

        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { inst->rend.graphicsQueueIndex, inst->rend.presentQueueIndex };
        if (inst->rend.graphicsQueueIndex == inst->rend.presentQueueIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = c_vlkn::presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult err = vkCreateSwapchainKHR(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchain);
        CORE_ASSERT(err == VK_SUCCESS && "Swapchain creation failed");

    }

    // Swap chain images
    {

        vkGetSwapchainImagesKHR(inst->rend.device, inst->rend.swapchain, &inst->rend.imageCount, nullptr);
        inst->rend.swapchainImages = new VkImage[inst->rend.imageCount];
        vkGetSwapchainImagesKHR(inst->rend.device, inst->rend.swapchain, &inst->rend.imageCount, inst->rend.swapchainImages);

    }

    // Swap chain image views
    {

        inst->rend.swapchainImageViews = new VkImageView[inst->rend.imageCount];
        for (size_t i = 0; i < inst->rend.imageCount; ++i) {

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.flags = 0;
            createInfo.image = inst->rend.swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = c_vlkn::format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchainImageViews[i]);
            CORE_ASSERT(err == VK_SUCCESS && "Image view creation failed");

        }

    }

}
void Core::createFramebuffers(VlknRenderInstance* rend) {

    rend->framebuffers = new VkFramebuffer[rend->imageCount];

    for (uint32_t i = 0; i < rend->imageCount; ++i) {

        VkImageView attachments[] = { rend->swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = rend->renderPass;
        framebufferInfo.attachmentCount = arraySize(attachments);
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = rend->windowImageExtent.width;
        framebufferInfo.height          = rend->windowImageExtent.height;
        framebufferInfo.layers          = 1;

        VkResult err = vkCreateFramebuffer(rend->device, &framebufferInfo, nullptr, &rend->framebuffers[i]);
        CORE_ASSERT(err == VK_SUCCESS && "Frambuffer creation failed");

    }

}
void Core::cleanupSwapchainResources(VlknRenderInstance* rend) {

    for (unsigned i = 0; i < rend->imageCount; ++i) {
        vkDestroyFramebuffer(rend->device, rend->framebuffers[i], nullptr);
    }
    delete[] rend->framebuffers;
    for (unsigned i = 0; i < rend->imageCount; ++i) {
        vkDestroyImageView(rend->device, rend->swapchainImageViews[i], nullptr);
    }
    delete[] rend->swapchainImageViews;
    delete[] rend->swapchainImages;

}
void Core::recreateSwapchain(Instance* inst) {

    vkDeviceWaitIdle(inst->rend.device);

    cleanupSwapchainResources(&inst->rend);
    vkDestroySwapchainKHR(inst->rend.device, inst->rend.swapchain, nullptr);

    createSwapchain(inst);
    createFramebuffers(&inst->rend);

}

void Core::createVertexIndexBuffers(Instance* inst, ViewportInstance* vpInst, VertexIndexBuffersInfo* buffsInfo) {

    VkBuffer       vertStageBuff, indexStageBuff;
    VkDeviceMemory vertStageMem,  indexStageMem;

    // Vertex buffer
    {

        vlknh::BufferCreateInfo buffInfo{};
        buffInfo.physicalDevice = inst->rend.physicalDevice;
        buffInfo.size           = buffsInfo->vertexDataSize;
        buffInfo.usage          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffInfo.properties     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vlknh::createBuffer(inst->rend.device, buffInfo, &vertStageBuff, &vertStageMem);
        vlknh::loadBuffer(inst->rend.device, vertStageMem, buffsInfo->vertexData, buffsInfo->vertexDataSize);

        buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vlknh::createBuffer(inst->rend.device, buffInfo, &vpInst->vertBuff, &vpInst->vertBuffMem);

    }

    // Indexbuffer
    {

        vlknh::BufferCreateInfo buffInfo{};
        buffInfo.physicalDevice = inst->rend.physicalDevice;
        buffInfo.size           = buffsInfo->indexDataSize;
        buffInfo.usage          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffInfo.properties     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vlknh::createBuffer(inst->rend.device, buffInfo, &indexStageBuff, &indexStageMem);
        vlknh::loadBuffer(inst->rend.device, indexStageMem, buffsInfo->indexData, buffsInfo->indexDataSize);

        buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vlknh::createBuffer(inst->rend.device, buffInfo, &vpInst->indexBuff, &vpInst->indexBuffMem);

    }

    VkCommandBuffer singleTimeBuff; 
    vlknh::SingleTimeCommandBuffer::begin  (inst->rend.device, inst->rend.commandPool, &singleTimeBuff);
    vlknh::SingleTimeCommandBuffer::copy   (singleTimeBuff, buffsInfo->vertexDataSize, vertStageBuff, vpInst->vertBuff);
    vlknh::SingleTimeCommandBuffer::copy   (singleTimeBuff, buffsInfo->indexDataSize, indexStageBuff, vpInst->indexBuff);
    vlknh::SingleTimeCommandBuffer::submit (inst->rend.device, singleTimeBuff, inst->rend.commandPool, inst->rend.graphicsQueue);

    vkQueueWaitIdle(inst->rend.graphicsQueue);

    vkFreeCommandBuffers(inst->rend.device, inst->rend.commandPool, 1, &singleTimeBuff);
    vkFreeMemory        (inst->rend.device, vertStageMem,   nullptr);
    vkDestroyBuffer     (inst->rend.device, vertStageBuff,  nullptr);
    vkFreeMemory        (inst->rend.device, indexStageMem,  nullptr);
    vkDestroyBuffer     (inst->rend.device, indexStageBuff, nullptr);

}

VkResult Core::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void Core::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Core::Callback::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    Instance* inst = (Instance*)GetWindowLongPtr(hwnd, GWLP_USERDATA); 

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) return true;

    switch (uMsg)
    {
    case WM_NCHITTEST: {
        POINT mpos = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &mpos);

        constexpr int borderWidth = 8;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        // Check if the cursor is in a corner or edge for resizing
        if (mpos.y < inst->gui.guiSizes.titleBarHeight && mpos.y >= borderWidth && mpos.x > inst->gui.guiSizes.menuBarEndExtent && mpos.x < windowRect.right - 3 * inst->gui.guiSizes.wndBtnWidth ) {
            return HTCAPTION;
        }
        else if (mpos.y < borderWidth && mpos.x < borderWidth) {
            return HTTOPLEFT;
        }
        else if (mpos.y < borderWidth && mpos.x > windowRect.right - borderWidth) {
            return HTTOPRIGHT;
        }
        else if (mpos.y > windowRect.bottom - borderWidth && mpos.x < borderWidth) {
            return HTBOTTOMLEFT;
        }
        else if (mpos.y > windowRect.bottom - borderWidth && mpos.x > windowRect.right - borderWidth) {
            return HTBOTTOMRIGHT;
        }
        else if (mpos.y < borderWidth) {
            return HTTOP;
        }
        else if (mpos.y > windowRect.bottom - borderWidth) {
            return HTBOTTOM;
        }
        else if (mpos.x < borderWidth) {
            return HTLEFT;
        }
        else if (mpos.x > windowRect.right - borderWidth) {
            return HTRIGHT;
        }
        return HTCLIENT; 
    }
    case WM_SIZE: {

        inst->wind.m_size.x = LOWORD(lParam);
        inst->wind.m_size.y = HIWORD(lParam);

        recreateSwapchain(inst);
        App::render(inst);

        return 0; 
    }
    case WM_ACTIVATE: {
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);

        SetWindowPos(hwnd,
            NULL,
            rcClient.left, rcClient.top,
            rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
            SWP_FRAMECHANGED
        );

        return 0; 

    }
    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = c_minWidth;
        lpMMI->ptMinTrackSize.y = c_minHeight;
        return 0;
    }
    case WM_NCCALCSIZE: {
        if (wParam == TRUE)
        {
            NCCALCSIZE_PARAMS* pncsp = (NCCALCSIZE_PARAMS*)lParam;

            // Offset the 12 pixel extension when maximized
            if (IsZoomed(hwnd)) {
                pncsp->rgrc[0].left += 12;  
                pncsp->rgrc[0].top += 12;   
                pncsp->rgrc[0].right -= 12; 
                pncsp->rgrc[0].bottom -= 12;
            }
            // Extend the border 1 pixel in for the standard border
            else {
                pncsp->rgrc[0].left += 1;
                pncsp->rgrc[0].top += 1;
                pncsp->rgrc[0].right -= 1;
                pncsp->rgrc[0].bottom -= 1;
            }

            return 0;
        }
        break;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

}
VKAPI_ATTR VkBool32 VKAPI_CALL Core::Callback::validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {

    printf("%s\n\n", pCallbackData->pMessage);
    assert(!"Validation layer callback called! Check the console for the error.");

    return VK_FALSE;

}