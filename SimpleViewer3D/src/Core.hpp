#pragma once

// includes
#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Gui/Gui.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>

#ifndef DEVINFO
#define DISABLE_SCOPEDTIMER
#endif

#include <Timer.hpp>

#include <vector>

// macros
#define arraySize(array) (sizeof(array) / sizeof(array[0]))
#define CORE_ASSERT(exp) assert(exp); // to quickly change the assert function if anyone wishes todo so.

namespace Core {

// structs
struct WindowInstance {

    HWND       hwnd;
    glm::ivec2 m_size; // units: pixels
    float      dpi;  // set by display scaling
    float      refreshInterval;

};

struct VlknRenderInstance {

    VkInstance               instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR             surface;
    uint32_t                 graphicsQueueIndex;
    uint32_t                 presentQueueIndex;
    VkPhysicalDevice         physicalDevice;
    VkDevice                 device;
    VkQueue                  graphicsQueue;
    VkQueue                  presentQueue;
    VkExtent2D               windowImageExtent;
    VkPresentModeKHR         presentMode;
    VkSwapchainKHR           swapchain;
    uint32_t                 imageCount;
    VkImage*                 swapchainImages;
    VkImageView*             swapchainImageViews;
    VkRenderPass             renderPass;
    VkFramebuffer*           framebuffers;
    VkCommandPool            commandPool;
    VkCommandBuffer          commandBuff;
    VkDescriptorPool         descriptorPool;
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;
    float                    frameWaitTime;

};

// NOT imgui viewport as in a separate window. This is where the mesh is drawn.
struct ViewportInstance {

    VkImage                 image;
    VkDeviceMemory          imageMem;
    VkImageView             imageView;
    VkFramebuffer           framebuffer;
    VkImage                 colorImage;
    VkDeviceMemory          colorImageMem;
    VkImageView             colorImageView;
    VkImage                 depthImage;
    VkDeviceMemory          depthImageMem;
    VkImageView             depthImageView;
    VkBuffer                vertBuff;
    VkDeviceMemory          vertBuffMem;
    VkBuffer                indexBuff;
    VkDeviceMemory          indexBuffMem;
    VkDescriptorSet         descriptorSet;

};

struct ViewportsRenderInstance {

    VkRenderPass          renderPass;
    VkPipelineLayout      pipelineLayout;
    VkPipeline            graphicsPipeline;
    VkPipeline            meshOutlinePipeline; 
    VkSampler             frameSampler;
    VkDescriptorSetLayout descriptorSetLayout;
    VkImage               logoImg;
    VkDeviceMemory        logoImgMem;
    VkImageView           logoImgView;
    VkImage               icoImg;
    VkDeviceMemory        icoImgMem;
    VkImageView           icoImgView;
    VkImage               mouseControlsImg;
    VkDeviceMemory        mouseControlsImgMem;
    VkImageView           mouseControlsImgView;
    VkDescriptorSet       logoDescriptor;

    std::vector<ViewportInstance> vpInstances;


};

struct Instance {

    WindowInstance          wind{};
    VlknRenderInstance      rend{};
    ViewportsRenderInstance vpRend{};
    Gui::DrawData           gui{};

};

struct  VertexIndexBuffersInfo {

    void*  vertexData;
    size_t vertexDataSize;

    void*  indexData; 
    size_t indexDataSize;

};

// functions
void     createSwapchain           (Instance* inst, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
void     createFramebuffers        (VlknRenderInstance* rend); 
void     cleanupSwapchainResources (VlknRenderInstance* rend);
void     recreateSwapchain         (Instance* inst);

void     createGeometryData        (Instance* inst, ViewportInstance* vpInst, VertexIndexBuffersInfo* buffsInfo);
void     createVpImageResources    (Instance* inst, ViewportInstance* vpInst, const VkExtent2D size);
bool     openMeshFile              (Instance* inst, const char* file);
void     destroyGeometryData       (VkDevice device, ViewportInstance* vpInst);
void     destroyVpImageResources   (VkDevice device, ViewportInstance* vpInst);   

    namespace Callback {

    LRESULT CALLBACK               WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); 
    VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    }

}
