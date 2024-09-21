#include "Gui.hpp"

#include <imgui_internal.h>

#include <algorithm>

namespace c_style {

    /*
    * Color are listed from the most used color to be color 1, and the
    * highest number color to be the least use color of the theme.
    * For example:
    * col1 = background colors
    * col2 = border colors
    * col3 = hover colors
    */
    constexpr ImVec4 col1 = ImGui::rgba32toVec4(15, 15, 15, 255);
    constexpr ImVec4 col2 = ImGui::rgba32toVec4(20, 20, 20, 255);
    constexpr ImVec4 col3 = ImGui::rgba32toVec4(40, 40, 40, 255);
    constexpr ImVec4 activeCol = ImGui::rgba32toVec4(69, 61, 40, 255);
    constexpr ImVec4 hoverCol = ImGui::rgba32toVec4(113, 100, 65, 255);

}

void Gui::init(InitInfo* initInfo, GuiSizes* guiSizes, float guiDpi) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(initInfo->hwnd);

    ImGui_ImplVulkan_InitInfo imguiInitInfo{};
    imguiInitInfo.Instance        = initInfo->instance;
    imguiInitInfo.PhysicalDevice  = initInfo->physicalDevice;
    imguiInitInfo.Device          = initInfo->device;
    imguiInitInfo.QueueFamily     = initInfo->graphicsQueueIndex;
    imguiInitInfo.Queue           = initInfo->graphicsQueue;
    imguiInitInfo.PipelineCache   = VK_NULL_HANDLE;
    imguiInitInfo.DescriptorPool  = initInfo->descriptorPool;
    imguiInitInfo.RenderPass      = initInfo->renderPass;
    imguiInitInfo.Subpass         = 0;
    imguiInitInfo.MinImageCount   = initInfo->imageCount;
    imguiInitInfo.ImageCount      = initInfo->imageCount;
    imguiInitInfo.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    imguiInitInfo.Allocator       = nullptr;
    imguiInitInfo.CheckVkResultFn = [](VkResult err) { assert(err == VK_SUCCESS && "check_vk_result failed"); };
    
    ImGui_ImplVulkan_Init(&imguiInitInfo);

    for (int i = 0; i < (sizeof(Gui::GuiSizes) / sizeof(float)); ++i) {
        ((float*)guiSizes)[i] *= guiDpi;
    }

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", guiSizes->fontSize);
    io.Fonts->Build();

    // Gui Style
    {

        // sizes
        ImGuiStyle& style = ImGui::GetStyle();

        style.FramePadding = ImVec2(20, 4);
        style.ItemInnerSpacing = ImVec2(6, 6);
        style.TabBarBorderSize = 2;
        style.TabBarOverlineSize = 0;
        style.TabRounding = 12;
        style.WindowTitleAlign = ImVec2(0.5, 0.5);
        style.DockingSeparatorSize = 2;
        style.TabBorderSize = 0;

        style.Colors[ImGuiCol_MenuBarBg]     = c_style::col1;
        style.Colors[ImGuiCol_TitleBg]       = c_style::col1;
        style.Colors[ImGuiCol_TitleBgActive] = c_style::col1;

        style.Colors[ImGuiCol_Header]        = c_style::col3;
        style.Colors[ImGuiCol_HeaderHovered] = ImGui::rgba32toVec4(60, 60, 60, 255);

        style.Colors[ImGuiCol_Tab]               = c_style::col3;
        style.Colors[ImGuiCol_TabHovered]        = c_style::hoverCol;
        style.Colors[ImGuiCol_TabSelected]       = c_style::activeCol;
        style.Colors[ImGuiCol_TabDimmed]         = c_style::col3;
        style.Colors[ImGuiCol_TabDimmedSelected] = c_style::col3;

        style.Colors[ImGuiCol_ButtonHovered] = ImGui::rgba32toVec4(255, 255, 255, 75);

        style.Colors[ImGuiCol_DockingPreview] = c_style::hoverCol;

    }

}

void Gui::draw(HWND hwnd, Commands* cmdListPtr, DrawData* data) {
 
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Main dockspace
    ImGuiID dockspaceID;
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        float titleBarPadding = 0.5f * (data->guiSizes.titleBarHeight - data->guiSizes.fontSize);
        ImVec2 prevFramePadding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));

        ImGui::Begin("Simple Viewer 3D", nullptr, windowFlags);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, prevFramePadding);
        dockspaceID = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspaceID);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetCursorPosX(data->guiSizes.menuBarStartExtent);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open", "Ctrl + O")) *cmdListPtr++ = Gui::cmd_openDialog;
                ImGui::EndMenu();
            }
            ImGui::PopStyleVar();
            ImGui::SetCursorPosX(0.465f * viewport->Size.x);
            ImGui::Text("Simple Viewer 3D");


            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0, 1.0, 1.0, 0.15));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0, 1.0, 1.0, 0.4));

            float btnWidth     = data->guiSizes.wndBtnWidth;
            float btnHeight    = data->guiSizes.titleBarHeight;
            float iconSize     = 0.25f * btnWidth;
            float iconStartPos = 0.5f * (btnWidth - iconSize);
            constexpr float iconLineWidth = 2.0;

            // Minimize Button
            ImGui::SetCursorPosX(viewport->Size.x - 3 * btnWidth);
            ImVec2 minimizeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * btnHeight);
            if (ImGui::Button("##Min", ImVec2(btnWidth, btnHeight))) *cmdListPtr++ = Gui::cmd_minimizeWindow;

            // Maximize/Restore Button
            ImGui::SetCursorPosX(viewport->Size.x - 2 * btnWidth);
            ImVec2 windowIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
            if (ImGui::Button("##Max", ImVec2(btnWidth, btnHeight))) {
                if (IsZoomed(hwnd)) *cmdListPtr++ = Gui::cmd_restoreWindow;
                else                *cmdListPtr++ = Gui::cmd_maximizeWindow;
            }

            // Close App Button 
            ImGui::SetCursorPosX(viewport->Size.x - btnWidth);
            ImVec2 closeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
            if (ImGui::Button("##Close", ImVec2(btnWidth, btnHeight))) *cmdListPtr++ = Gui::cmd_closeWindow;

            ImGui::PopStyleColor(3);

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text);
            drawList->AddLine(minimizeIconDrawPos, minimizeIconDrawPos + ImVec2(iconSize, 0.0), iconColor, iconLineWidth);
            if (IsZoomed(hwnd)) {
                constexpr float inset = 0.07;
                ImVec2 recCorner1 = windowIconDrawPos + ImVec2(inset * btnHeight, 0.0);
                ImVec2 recCorner2 = windowIconDrawPos + ImVec2(iconSize, iconSize - inset * btnHeight);
                drawList->AddRect(recCorner1, recCorner2, iconColor, 2.0, 0, iconLineWidth);
                recCorner1 += ImVec2(-iconLineWidth, iconLineWidth);
                recCorner2 += ImVec2(-iconLineWidth, iconLineWidth);
                drawList->AddRectFilled(recCorner1, recCorner2, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
                recCorner1 = windowIconDrawPos + ImVec2(0.0, inset * btnHeight);
                recCorner2 = windowIconDrawPos + ImVec2(iconSize - inset * btnHeight, iconSize);
                drawList->AddRect(recCorner1, recCorner2, iconColor, 2.0, 0, iconLineWidth);
            }
            else {
                drawList->AddRect(windowIconDrawPos, windowIconDrawPos + ImVec2(iconSize, iconSize), iconColor, 2.0, 0, iconLineWidth);
            }


            drawList->AddLine(closeIconDrawPos, closeIconDrawPos + ImVec2(iconSize, iconSize), iconColor, iconLineWidth);
            drawList->AddLine(closeIconDrawPos + ImVec2(0.0, iconSize), closeIconDrawPos + ImVec2(iconSize, 0.0), iconColor, iconLineWidth);


            ImGui::EndMenuBar();
        }
        ImGui::PopStyleVar(2);
        ImGui::End();
        ImGui::PopStyleVar(2);

    }

    ImGuiIO& io = ImGui::GetIO();
#ifdef DEBUG
    ImGui::ShowDemoWindow();
#endif
#ifdef DEVINFO

    if (ImGui::Begin("App Data")) {
        ImGui::Text("Framerate: %f", io.Framerate);
        float (*func)(void*, int) = [](void* data, int i) { return ((float*)data)[i]; };
        constexpr float scale = 0.020;
        constexpr int sampleCount = sizeof data->stats.frameWaitTimesGraph / sizeof(float);
        ImGui::PlotLines("Frame Time Error", func, data->stats.frameWaitTimesGraph, sampleCount, 0, nullptr, -scale, scale, ImVec2(0, 200));
        ImGui::Text("Scale: %.3fms", scale * 1000);
        ImGui::Text("Viewport resizes: %u", data->stats.resizeCount);
        ImGui::Text("Performance Times:");
        ImGui::Text("openFile: %.2fms", 1000 * data->stats.perfTimes.openFile);
        ImGui::Text("fileClose: %.2fms", 1000 * data->stats.perfTimes.fileClose);
        ImGui::Text("viewportResize: %.2fms", 1000 * data->stats.perfTimes.viewportResize);
        ImGui::Text("appLauch: %.2fms", 1000 * data->stats.perfTimes.appLaunch);
        ImGui::Text("renderingCommands: %.2fms", 1000 * data->stats.perfTimes.renderingCommands);

    }
    ImGui::End();
#endif
 
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    for (ViewportGuiData& vpData : data->vpDatas) {

        vpData.visible = false;
        ImGuiWindowFlags windFlags = ImGuiWindowFlags_NoScrollbar;
        windFlags |= ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_Once);
        if (ImGui::Begin(vpData.objectName.get(), &vpData.open, windFlags)) {
            ImVec2 currentVpSize = ImGui::GetContentRegionAvail();

            if (currentVpSize.x > 0.0 && currentVpSize.y > 0.0) vpData.visible = true;
            else goto ContentNotVisible;

            if (vpData.m_size != currentVpSize) { 
                vpData.resize = true; 
                vpData.m_size   = currentVpSize; 
            }
            ImGui::Image(vpData.framebufferTexID, vpData.m_size);

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) vpData.orbitActive = true;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) vpData.orbitActive = false;
            if (vpData.orbitActive) {
                vpData.orbitAngle  += glm::vec2(-0.003 * io.MouseDelta.y, 0.003 * io.MouseDelta.x);
                vpData.orbitAngle.x = glm::clamp(vpData.orbitAngle.x, glm::radians(-90.0f), glm::radians(90.0f));
            }
            ContentNotVisible:;

        }
        ImGui::End();
    

    }

    ImGui::PopStyleVar();
    ImGui::EndFrame();
    *cmdListPtr = Gui::cmd_null;
}

void Gui::destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}