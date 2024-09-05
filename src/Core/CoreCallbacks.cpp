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

    extern int   windowWidth;
    extern int   windowHeight;
    extern bool  windowMinimized;
    extern float windowDpi;

}


void recreateSwapchain();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
    case WM_NCHITTEST: {
        POINT mpos = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &mpos);

        constexpr int borderWidth = 8;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        // Check if the cursor is in a corner or edge for resizing
        if (mpos.y < s_gui.titleBarHeight && mpos.y >= borderWidth && mpos.x > 50 && mpos.x < windowRect.right - 3 * s_gui.wndBtnWidth ) {
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

const char* fileName = "Error-Log.txt";
#ifdef ENABLE_VK_VALIDATION_LAYERS

int openConsole() {

    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    return 0;
}
int a = openConsole(); // just to run resetFile when app boots

VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {

    printf("%s\n\n", pCallbackData->pMessage);
    assert(!"Validation layer callback called! Check the console for the error.");

    return VK_FALSE;

}

#endif 