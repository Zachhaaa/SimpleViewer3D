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

#include <vector>

// macros
#define arraySize(array) (sizeof(array) / sizeof(array[0]))
#define CORE_ASSERT(exp) assert(exp);

namespace Core {

// structs
struct WindowInstance {

    HWND                hwnd;
    glm::ivec2          m_size; // units: pixels
    float               dpi;  // set by display scaling
    float              refreshInterval;

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
    VkSwapchainKHR           swapchain;
    uint32_t                 imageCount;
    VkImage*                 swapchainImages;
    VkImageView*             swapchainImageViews;
    VkRenderPass             renderPass;
    VkFramebuffer*           framebuffers;
    VkCommandPool            commandPool;
    VkCommandBuffer          commandBuff;
    VkDescriptorPool         descriptorPool;
    //uint32_t                 maxDescriptorSets;
    //uint32_t                 setCount;
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;
    float                    frameWaitTime;

};

// NOT imgui viewport as in a separate window.This is where the 3D model is drawn.
struct ViewportInstance {

    VkImage                 image;
    VkDeviceMemory          imageMem;
    VkImageView             imageView;
    VkFramebuffer           framebuffer;
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
    VkSampler             frameSampler;
    VkDescriptorSetLayout descriptorSetLayout;
    VkImage               logoImg;
    VkDeviceMemory        logoImgMem;
    VkImageView           logoImgView;
    VkDescriptorSet       logoDescriptor;

    std::vector<ViewportInstance> vpInstances;


};

struct Instance {

    WindowInstance          wind{};
    VlknRenderInstance      rend{};
    ViewportsRenderInstance vpRend{};
    // DO NOT ZERO the following struct. gui.guiSizes must not be set to zero and must use init values.
    Gui::DrawData           gui; 

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

void     createVertexIndexBuffers  (Instance* inst, ViewportInstance* vpInst, VertexIndexBuffersInfo* buffsInfo);

VkResult CreateDebugUtilsMessengerEXT  (VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void     DestroyDebugUtilsMessengerEXT (VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator);


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
