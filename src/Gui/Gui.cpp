#include "Gui.hpp"

#include <algorithm>

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

}

void Gui::draw(HWND hwnd, Commands* cmdListPtr, DrawData* data) {
 
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Main dockspace
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
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetCursorPosX(data->guiSizes.menuBarStartExtent);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open", "Ctrl + O")) *cmdListPtr++ = Gui::cmd_restoreWindow;
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

    ImGui::ShowDemoWindow();

    ImGui::SetNextWindowSize({ 600, 600 }, ImGuiCond_FirstUseEver);

    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::Begin("App Data")) {
        ImGui::Text("Framerate: %f", io.Framerate);
        float (*func)(void*, int) = [](void* data, int i) { return ((float*)data)[i]; };
        constexpr float scale = 0.020;
        constexpr int sampleCount = sizeof data->stats.frameWaitTimesGraph / sizeof(float);
        ImGui::PlotLines("Frame Time Error", func, data->stats.frameWaitTimesGraph, sampleCount, 0, nullptr, -scale, scale, ImVec2(0, 200));
        ImGui::Text("Scale: %.3fms", scale * 1000);
        ImGui::Text("Viewport resizes: %u", data->stats.resizeCount);
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Viewport")) {

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) data->vpData.orbitActive = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) data->vpData.orbitActive = false;
        if (data->vpData.orbitActive) {
            data->vpData.orbitAngle  += glm::vec2(-0.003 * io.MouseDelta.y, 0.003 * io.MouseDelta.x);
            data->vpData.orbitAngle.x = glm::clamp(data->vpData.orbitAngle.x, glm::radians(-90.0f), glm::radians(90.0f));
        }
        ImVec2 currentVpSize = ImGui::GetContentRegionAvail();
        if (data->vpData.viewportSize != currentVpSize) { 
            *cmdListPtr++ = Gui::cmd_resizeViewport; 
            data->vpData.viewportSize = currentVpSize; 
        }
        ImGui::Image(data->vpData.framebufferTexID, data->vpData.viewportSize);

    }
    ImGui::End();
    ImGui::PopStyleVar();
    
    ImGui::EndFrame();
    *cmdListPtr = Gui::cmd_null;

}

void Gui::destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}