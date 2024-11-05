#pragma once 

// includes
#include "Gui/Gui.hpp"

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <cassert>
#include <string>

// macros
#define arraySize(array) (sizeof(array) / sizeof(array[0]))
#define CORE_ASSERT(exp) assert(exp);


namespace Core {

// structs
struct ivec2 {
    int x, y; 
};

struct WindowInstance {

    HWND  hwnd;
    ivec2 size; // units: pixels
    float dpi;  // set by display scaling
    float refreshInterval; 

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
    VkSampler                logoSampler; 
    VkDescriptorSetLayout    descriptorSetLayout;
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;
    VkImage                  logoImg;
    VkDeviceMemory           logoImgMem;
    VkImageView              logoImgView;
    float                    frameWaitTime;

};

struct Instance {

    WindowInstance          wind{};
    VlknRenderInstance      rend{};
    Gui::DrawData           gui;
    std::wstring            installDir = L"C:\\Program Files";

};

// functions
class RegKey {
public: 

    RegKey(HKEY rootKey, const wchar_t* subkey) {
        LONG result = RegCreateKeyEx(
            rootKey,
            subkey,
            0,
            NULL,
            0,
            KEY_WRITE,
            NULL,
            &hkey,
            NULL
        );
        assert(result == ERROR_SUCCESS && "Failed to open registry!");
    }
    void set(const wchar_t* value, const wchar_t* data) {
        LONG result = RegSetValueEx(hkey, value, 0, REG_SZ, (const BYTE*)data, DWORD((wcslen(data) + 1) * sizeof(wchar_t)));
        assert(result == ERROR_SUCCESS && "Failed to write registry!");
    }
    void set(const wchar_t* value, std::wstring& data) {
        LONG result = RegSetValueEx(hkey, value, 0, REG_SZ, (const BYTE*)data.c_str(), DWORD((data.size() + 1) * sizeof(wchar_t)));
        assert(result == ERROR_SUCCESS && "Failed to write registry!");
    }
    void set(const wchar_t* value, DWORD data) {
        LONG result = RegSetValueEx(hkey, value, 0, REG_DWORD, (const BYTE*)&data, sizeof(data));
        assert(result == ERROR_SUCCESS && "Failed to write registry!");
    }
    ~RegKey() { RegCloseKey(hkey); }

public: 

    HKEY hkey;

};



VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void     DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

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