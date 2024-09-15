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

// macros
#define arraySize(array) (sizeof(array) / sizeof(array[0]))
#define CORE_ASSERT(exp) assert(exp);

namespace Core {

// structs
struct WindowInstance {

    HWND                hwnd;
    glm::ivec2          size; // units: pixels
    float               dpi;  // set by display scaling
    double              refreshInterval;

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
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;

};

struct ViewportRenderInstance {

    VkImage               image;
    VkDeviceMemory        imageMem;
    VkImageView           imageView;
    VkRenderPass          renderPass;
    VkFramebuffer         framebuffer;
    VkImage               depthImage;
    VkDeviceMemory        depthImageMem;
    VkImageView           depthImageView;
    VkPipelineLayout      pipelineLayout;
    VkPipeline            graphicsPipeline;
    VkBuffer              vertBuff;
    VkDeviceMemory        vertBuffMem;
    VkBuffer              indexBuff;
    VkDeviceMemory        indexBuffMem;
    uint32_t              triangleCount;
    VkSampler             frameSampler;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       descriptorSet;
    double                frameWaitTime; 

};

struct Instance {

    WindowInstance          wind{};
    VlknRenderInstance      rend{};
    ViewportRenderInstance  vpRend{};
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

void     createVertexIndexBuffers  (Instance* inst, VertexIndexBuffersInfo* buffsInfo);

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
