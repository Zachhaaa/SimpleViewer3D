#include "CoreCallbacks.hpp"
#include "CoreConstants.hpp"
#include "Core.hpp"

#include <cstdio>
#include <cassert> 
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <dwmapi.h>
#include <mutex> 
#include <Timer.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace win {

    extern int windowWidth;
    extern int windowHeight;
    extern bool windowMinimized;

}
void recreateSwapchain();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    if (uMsg == WM_ACTIVATE)
    {
        // Extend the frame into the client area.
        MARGINS margins;

        margins.cxLeftWidth    = 1;     
        margins.cxRightWidth   = 1;    
        margins.cyBottomHeight = 1; 
        margins.cyTopHeight    = 1;    

        DwmExtendFrameIntoClientArea(hwnd, &margins);

        return 0; 
    }

    switch (uMsg)
    {
    case WM_NCHITTEST: {
        POINT mpos = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &mpos);

        constexpr int borderWidth = 8;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        // Check if the cursor is in a corner or edge for resizing
        if (mpos.y < 35 && mpos.y >= borderWidth && mpos.x >= borderWidth && mpos.x <= windowRect.right - borderWidth) {
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

        break;
    }
    case WM_SIZE: {
        // If minimized
        if (LOWORD(lParam) == 0)
        { win::windowMinimized = true; return 0; }
        else 
        { win::windowMinimized = false; }

        win::windowWidth  = LOWORD(lParam);
        win::windowHeight = HIWORD(lParam);
        recreateSwapchain();
        App::render();

        return 0; 
    }
    case WM_SHOWWINDOW: {
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);

        // Inform the application of the frame change.
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
    case WM_NCCALCSIZE: 
        if (wParam == TRUE) return 0; 
        break;
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

    assert(!"Validation layer called! check Error-Log.txt");

    return VK_FALSE;

}