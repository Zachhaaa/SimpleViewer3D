#include "Core.hpp"

#include <cstdio>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Core::Callback::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) return true;

    switch (uMsg)
    {

    case WM_KEYDOWN: {

        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

    }
    case WM_DESTROY: case WM_CLOSE: {

        PostQuitMessage(0);
        return 0;

    }

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