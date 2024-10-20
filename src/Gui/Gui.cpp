#include "Gui.hpp"

#include <imgui_internal.h>

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace c_style {

    /*
    * Color are listed from the most used color to be color 1, and the
    * highest number color to be the least use color of the theme.
    * For example:
    * col1 = background colors
    * col2 = border colors
    * col3 = hover colors
    */
    
    constexpr ImVec4 col1      (0.0588, 0.0588, 0.0588, 1.0); // rgba(15, 15, 15, 255)
    constexpr ImVec4 col2      (0.0784, 0.0784, 0.0784, 1.0); // rbga(20, 20, 20, 255)
    constexpr ImVec4 col3      (0.157, 0.157, 0.157, 1.0);    // rgba(40, 40, 40, 255)
    constexpr ImVec4 activeCol (0.271, 0.239, 0.157, 1.0);    // rgba(69, 61, 40, 255)
    constexpr ImVec4 hoverCol  (0.443, 0.392, 0.255, 1.0);    // rgba(113, 100, 65, 255)

}

void Gui::init(InitInfo* initInfo, GuiStyleEx* styleEx, float guiDpi) {

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

    // Style
    {

        // Sizes
        styleEx->sizes.fontSize           = 16.0f; 
        styleEx->sizes.largeFontSize      = 28.0f;
        styleEx->sizes.titleBarHeight     = 28.0f;
        styleEx->sizes.wndBtnWidth        = 40.0f;
        styleEx->sizes.menuBarStartExtent = 12.0f;
        styleEx->sizes.menuBarEndExtent   = 240.0f;

        ImGuiStyle& imStyle = ImGui::GetStyle(); 

        imStyle.FramePadding         = ImVec2(12, 3); 
        imStyle.ItemInnerSpacing     = ImVec2(4, 4); 
        imStyle.TabBarBorderSize     = 2; 
        imStyle.TabBarOverlineSize   = 0; 
        imStyle.TabRounding          = 12;
        imStyle.WindowTitleAlign     = ImVec2(0.5, 0.5); 
        imStyle.DockingSeparatorSize = 2; 
        imStyle.TabBorderSize        = 0; 
        imStyle.PopupRounding        = 6; 

        for (int i = 0; i < (sizeof(styleEx->sizes) / sizeof(float)); ++i) {
            ((float*)&styleEx->sizes)[i] *= guiDpi;
        }

        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf",   styleEx->sizes.fontSize);
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arialbd.ttf", styleEx->sizes.largeFontSize);
        io.Fonts->Build();

        imStyle.WindowPadding    *= guiDpi; 
        imStyle.WindowRounding   *= guiDpi; 
        imStyle.WindowMinSize    *= guiDpi; 
        imStyle.ChildRounding    *= guiDpi; 
        imStyle.PopupRounding    *= guiDpi; 
        imStyle.FramePadding     *= guiDpi; 
        imStyle.FrameRounding    *= guiDpi; 
        imStyle.ItemSpacing      *= guiDpi; 
        imStyle.ItemInnerSpacing *= guiDpi; 
        imStyle.CellPadding      *= guiDpi; 
        imStyle.GrabRounding     *= guiDpi; 
        imStyle.TabRounding      *= guiDpi; 

        // Colors
        styleEx->colors.shortcutText      = ImVec4(0.784, 0.784, 0.784, 1.0);
        styleEx->colors.mainWndBackground = ImVec4(0.0784, 0.0784, 0.0784, 1.0);

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.39f, 0.25f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.44f, 0.26f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.29f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.44f, 0.39f, 0.25f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.27f, 0.24f, 0.16f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.44f, 0.39f, 0.25f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    }

}

void Gui::draw(HWND hwnd, Commands* commands, DrawData* data) {
 
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    // Still need to fix this
    bool openShortcut = io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false) || ImGui::IsKeyPressed(ImGuiKey_Space, false);
    if (openShortcut) {
        *commands |= Gui::cmd_openDialogBit;
        // Because the window focus transfers to the open menu. The app never recieves an event for key release.
        // This means that when you press it again IsKeyPressed() will not be flagged because down is still flagged from the last press.
        // This effectively manually triggers a release event. 
        io.AddKeyEvent(ImGuiKey_O, false);
        io.AddKeyEvent(ImGuiKey_Space, false);
    }
    bool quitShortcut = io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Q);
    if (quitShortcut) *commands |= Gui::cmd_closeWindowBit;

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
        float titleBarPadding = 0.5f * (data->styleEx.sizes.titleBarHeight - data->styleEx.sizes.fontSize);
        ImVec2 prevFramePadding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, data->styleEx.colors.mainWndBackground);

        ImGui::Begin("Simple Viewer 3D", nullptr, windowFlags);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, prevFramePadding);
        dockspaceID = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspaceID);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetCursorPosX(data->styleEx.sizes.menuBarStartExtent);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
            float width = data->styleEx.sizes.titleBarHeight;
            float height = width;
            ImGui::Image(data->icoTexID, ImVec2(width, height));

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open", "Ctrl + O"))                                 *commands |= Gui::cmd_openDialogBit;
                if (ImGui::MenuItem("Close", "Ctrl + W", false, !!data->vpDatas.size())) data->lastFocusedVp->open = false;
                if (ImGui::MenuItem("Quit", "Ctrl + Q"))                                 *commands |= Gui::cmd_closeWindowBit;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Model")) {
                if (ImGui::MenuItem("Center", "Shift + C", false, !!data->vpDatas.size())) data->lastFocusedVp->model[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Preferences")) { 
                ImGui::Text("Sensitivity"); ImGui::SameLine();
                ImGui::SliderFloat("##Sense", &data->sensitivity, 1.0f, 100.0f, " % .0f", ImGuiSliderFlags_ClampOnInput);
                ImGui::EndMenu(); 
            }
            ImGui::PopStyleVar(); 
            ImGui::SetCursorPosX(0.465f * viewport->Size.x);
            ImGui::Text("Simple Viewer 3D");


            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0, 1.0, 1.0, 0.15));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0, 1.0, 1.0, 0.4));

            float btnWidth     = data->styleEx.sizes.wndBtnWidth;
            float btnHeight    = data->styleEx.sizes.titleBarHeight;
            float iconSize     = 0.25f * btnWidth;
            float iconStartPos = 0.5f * (btnWidth - iconSize);
            constexpr float iconLineWidth = 2.0;

            // Minimize Button
            ImGui::SetCursorPosX(viewport->Size.x - 3 * btnWidth);
            ImVec2 minimizeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * btnHeight);
            if (ImGui::Button("##Min", ImVec2(btnWidth, btnHeight))) *commands |= Gui::cmd_minimizeWindowBit;

            // Maximize/Restore Button
            ImGui::SetCursorPosX(viewport->Size.x - 2 * btnWidth);
            ImVec2 windowIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
            if (ImGui::Button("##Max", ImVec2(btnWidth, btnHeight))) {
                if (IsZoomed(hwnd)) *commands |= Gui::cmd_restoreWindowBit;
                else                *commands |= Gui::cmd_maximizeWindowBit;
            }

            // Close App Button 
            ImGui::SetCursorPosX(viewport->Size.x - btnWidth);
            ImVec2 closeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
            if (ImGui::Button("##Close", ImVec2(btnWidth, btnHeight))) *commands |= Gui::cmd_closeWindowBit;

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
        ImVec2 windSize = ImGui::GetWindowSize(); 
        ImGui::SetCursorPos((windSize - data->logoSize) * 0.5f - ImVec2(0, 2 * data->styleEx.sizes.largeFontSize));
        ImGui::Image(data->logoTexID, data->logoSize); 

        ImGui::PushFont(io.Fonts->Fonts[1]); 
        const char* tipLine1 = "Open File: Ctrl + O";
        const char* tipLine2 = "or";
        const char* tipLine3 = "Ctrl + Space";
        float line1Width = ImGui::CalcTextSize(tipLine1).x;
        float line2Width = ImGui::CalcTextSize(tipLine2).x;
        float line3Width = ImGui::CalcTextSize(tipLine3).x;
        ImGui::PushStyleColor(ImGuiCol_Text, data->styleEx.colors.shortcutText);
        ImGui::SetCursorPosX((windSize.x - line1Width) / 2);
        ImGui::Text(tipLine1);
        ImGui::SetCursorPosX((windSize.x - line2Width) / 2);
        ImGui::Text(tipLine2);
        ImGui::SetCursorPosX((windSize.x - line3Width) / 2);
        ImGui::Text(tipLine3);
        ImGui::PopStyleColor(); 
        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleColor(); 
        ImGui::PopStyleVar(2);

    }

#ifdef DEBUG
    ImGui::ShowDemoWindow();
    if (ImGui::Begin("GuiStyleEx")) {

        ImGui::ColorEdit4("shortcutText", (float*)&data->styleEx.colors.shortcutText, ImGuiColorEditFlags_AlphaBar); 
        ImGui::ColorEdit4("mainWndBackground", (float*)&data->styleEx.colors.mainWndBackground, ImGuiColorEditFlags_AlphaBar);

        ImGui::SliderFloat("titleBarHeight",     (float*)&data->styleEx.sizes.titleBarHeight,     1.0f, 50.0f);
        ImGui::SliderFloat("wndBtnWidth",        (float*)&data->styleEx.sizes.wndBtnWidth,        1.0f, 50.0f);
        ImGui::SliderFloat("menuBarStartExtent", (float*)&data->styleEx.sizes.menuBarStartExtent, 1.0f, 50.0f);
        ImGui::SliderFloat("menuBarEndExtent",   (float*)&data->styleEx.sizes.menuBarEndExtent,   1.0f, 50.0f);

    }
    ImGui::End(); 

    if (ImGui::Begin("Debug info")) {
        ImGui::SeparatorText("ViewportGuiData");
        if (data->vpDatas.size() > 0) {

            const char* openFiles[16];
            for(int i = 0; i < 16 && i < data->vpDatas.size(); i++) 
                openFiles[i] = data->vpDatas[i].objectName.get(); 
            static int currentFile = 0; 
            if (currentFile >= data->vpDatas.size()) currentFile = 0; 
            ImGui::Combo("File", &currentFile, openFiles,(int)data->vpDatas.size());
            ImGui::SeparatorText("model matrix");
            if (ImGui::BeginTable("model matrix", 4, ImGuiTableFlags_SizingFixedSame)) {
            
                glm::mat4& model = data->vpDatas[currentFile].model; 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.3f", model[0][0]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", model[1][0]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", model[2][0]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f", model[3][0]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.3f", model[0][1]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", model[1][1]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", model[2][1]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f", model[3][1]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.3f", model[0][2]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", model[1][2]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", model[2][2]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f", model[3][2]);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.3f", model[0][3]);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", model[1][3]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", model[2][3]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f", model[3][3]);

                ImGui::EndTable();
            }
            ImGui::SeparatorText("other");
            if (ImGui::BeginTable("ViewportGuiData", 2, ImGuiTableFlags_SizingFixedSame)) {

                ViewportGuiData& vpData = data->vpDatas[currentFile];

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("zoomDistance");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", vpData.zoomDistance);
                // BOOKMARK: label shortcut for centering the model. 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("farPlaneClip");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", vpData.farPlaneClip);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("modelCenter");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("(%.3f, %.3f, %.3f)", vpData.modelCenter.x, vpData.modelCenter.y, vpData.modelCenter.z);


                ImGui::EndTable();
            }
        }
    }
    ImGui::End(); 

#endif
#ifdef DEVINFO

    if (ImGui::Begin("App Data")) {
        ImGui::Text("Framerate: %f", io.Framerate);
        float (*func)(void*, int) = [](void* data, int i) { return ((float*)data)[i]; };
        constexpr float scale = 0.020;
        constexpr int sampleCount = sizeof data->stats.frameWaitTimesGraph / sizeof(float);
        ImGui::PlotLines("Frame Time Error", func, data->stats.frameWaitTimesGraph, sampleCount, 0, nullptr, -scale, scale, ImVec2(0, 200));
        ImGui::Text("Scale: %.3fms", scale * 1000);
        ImGui::SeparatorText("Viewports Data");
        ImGui::Text("Viewport resizes: %u", data->stats.resizeCount);
        ImGui::SeparatorText("Performance Times");
        ImGui::Text("openFile: %.2fms", 1000 * data->stats.perfTimes.openFile);
        ImGui::Text("fileClose: %.2fms", 1000 * data->stats.perfTimes.fileClose);
        ImGui::Text("viewportResize: %.2fms", 1000 * data->stats.perfTimes.viewportResize);
        ImGui::Text("appLauch: %.2fms", 1000 * data->stats.perfTimes.appLaunch);
        ImGui::Text("logoRasterize: %.2fms", 1000 * data->stats.perfTimes.logoRasterize);
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
            bool focus = ImGui::IsWindowFocused(); 
            bool wKey = ImGui::IsKeyPressed(ImGuiKey_W, false); 
            if (focus && io.KeyCtrl && wKey) vpData.open = false;
            data->lastFocusedVp = &vpData;

            ImVec2 currentVpSize = ImGui::GetContentRegionAvail();

            if (currentVpSize.x > 0.0 && currentVpSize.y > 0.0) {

                vpData.visible = true;

                if (vpData.size != currentVpSize) { 
                    vpData.resize = true; 
                    vpData.size = currentVpSize; 
                }
                ImGui::Image(vpData.framebufferTexID, vpData.size);

                ImGui::SetCursorPos(ImGui::GetCursorStartPos() + ImVec2(0, 15));
                
                //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20)); 
                //ImGuiChildFlags childFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
                //ImGui::BeginChild("ModelInfo", ImVec2(0.0, 0.0), childFlags;
                if (ImGui::TreeNodeEx("File Info", ImGuiTreeNodeFlags_SpanTextWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTable("File Info Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Triangles");
                        ImGui::TableSetColumnIndex(1); 
                        ImGui::Text("%u", vpData.triangleCount);
                        ImGui::TableNextRow(); 
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Unique Triangles");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u", vpData.uniqueTriangleCount);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Text Format?");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", vpData.isTextFormat ? "Yes" : "No");
                        ImGui::EndTable(); 
                    }
                    ImGui::TreePop(); 
                }
                //ImGui::EndChild();
                //ImGui::PopStyleVar(); 

                constexpr float c_defPanSense   = 0.0009f;
                constexpr float c_defOrbitSense = 0.01f;
                constexpr float c_defZoomSense  = 0.1f; 
                float sensePreferenceMultiplier = data->sensitivity / 100; 
                if (ImGui::IsWindowHovered()) {

                    float zoomSensitivity = c_defZoomSense * sensePreferenceMultiplier;
                    vpData.zoomDistance += zoomSensitivity * -vpData.zoomDistance * io.MouseWheel;
                    vpData.zoomDistance = std::clamp(vpData.zoomDistance, vpData.zoomMin, -1.0f);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))  vpData.orbitActive = true;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) vpData.panActive   = true;

                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))  vpData.orbitActive = false;
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) vpData.panActive   = false;

                if (vpData.orbitActive && io.MouseDelta != ImVec2(0, 0)) {

                    float& x = io.MouseDelta.x;
                    float& y = io.MouseDelta.y; 
                    float orbitSensitivity = c_defOrbitSense * sensePreferenceMultiplier; 
                    float angle = orbitSensitivity * sqrtf(x * x + y * y);
                    vpData.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-y, x, 0.0f)) * vpData.model;

                }
                if (vpData.panActive) {

                    float panSensitivity = c_defPanSense * sensePreferenceMultiplier; 
                    glm::vec2 panDelta = panSensitivity * -vpData.zoomDistance * glm::vec2(io.MouseDelta.x, io.MouseDelta.y);
                    vpData.panPos() += panDelta;

                }
            }
        }
        // Down here to hit cache line from vpData.panPos()
        if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_C, false)) vpData.model[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
      
        ImGui::End();

    }

    //BOOKMARK: fix bug where the window won't close when clicked from menu because the window loses focus when the menu is clicked. 

    ImGui::PopStyleVar();
    ImGui::EndFrame();
}

void Gui::destroy() {

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

}