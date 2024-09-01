#include "CoreCallbacks.hpp"



#include <cstdio>
#include <cassert> 
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <dwmapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

const char* fileName = "Error-Log.txt";
#ifdef ENABLE_VK_VALIDATION_LAYERS

int resetFile() {
    FILE* file = fopen(fileName, "w");
    assert(file != NULL);
    fclose(file);
    return 0;
}
int a = resetFile(); // just to run resetFile when app boots

#endif 

VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    FILE* logfile = fopen(fileName, "a");
    assert(logfile != NULL);

    fprintf(logfile, "%s\n", pCallbackData->pMessage);

    fclose(logfile);

    return VK_FALSE;

}