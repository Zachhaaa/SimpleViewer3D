#pragma once

#include <Windows.h>

#include <vulkan/vulkan.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); 
VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);