#include "Gui.hpp"

const char* license = 
"The MIT License (MIT)\n"
"\n"
"Copyright(c) 2024 Zach Puglia\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files(the \"Software\"), to deal\n"
"in the Software without restriction, including without limitation the rights\n"
"to use, copy, modify, merge, publish, distribute, sublicense, and /or sell\n"
"copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions :\n"
"\n"
"The above copyright notice and this permission notice shall be included in all\n"
"copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"SOFTWARE.\n";

void Gui::init(InitInfo* initInfo, GuiSizes* guiSizes, float guiDpi) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = nullptr; 
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

    guiSizes->fontSize        = 18.0f; 
    guiSizes->largeFontSize   = 28.0f; 
    guiSizes->childBorderSize = 1.0f;  
    guiSizes->childRounding   = 4.0f;
    guiSizes->frameRounding   = 4.0f;
    guiSizes->framePadding    = ImVec2(8, 4); 
    guiSizes->windowPadding   = ImVec2(16, 8); 
    guiSizes->childPadding    = ImVec2(12, 12); 
    guiSizes->itemSpacing     = ImVec2(7, 7); 

    for (int i = 0; i < (sizeof(Gui::GuiSizes) / sizeof(float)); ++i) {
        ((float*)guiSizes)[i] *= guiDpi;
    }

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf",   guiSizes->fontSize);
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arialbd.ttf", guiSizes->largeFontSize);
    io.Fonts->Build();

    // Gui Style
    {

        // sizes
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowBorderSize = 0; 
        style.ChildBorderSize  = guiSizes->childBorderSize;
        style.ChildRounding    = guiSizes->childRounding; 
        style.FrameRounding    = guiSizes->frameRounding;
        style.FramePadding     = guiSizes->framePadding; 
        style.WindowPadding    = guiSizes->windowPadding; 
        style.ItemSpacing      = guiSizes->itemSpacing;
        style.Colors[ImGuiCol_WindowBg]       = ImGui::ColorConvertU32ToFloat4(IM_COL32(15, 15, 15, 255));
        style.Colors[ImGuiCol_ChildBg]        = ImGui::ColorConvertU32ToFloat4(IM_COL32(25, 25, 25, 255));
        style.Colors[ImGuiCol_Border]         = ImGui::ColorConvertU32ToFloat4(IM_COL32(113, 100, 65, 255));
        style.Colors[ImGuiCol_FrameBg]        = ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 40, 40, 255));
        style.Colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(50, 50, 50, 255));
        style.Colors[ImGuiCol_FrameBgActive]  = ImGui::ColorConvertU32ToFloat4(IM_COL32(60, 60, 60, 255));
        style.Colors[ImGuiCol_Button]         = style.Colors[ImGuiCol_FrameBg];
        style.Colors[ImGuiCol_ButtonHovered]  = style.Colors[ImGuiCol_FrameBgHovered];
        style.Colors[ImGuiCol_ButtonActive]   = style.Colors[ImGuiCol_FrameBgActive];
        style.Colors[ImGuiCol_CheckMark]      = ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 177, 115, 255));

    }

}

void Gui::draw(DrawData* data) {

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
    windowFlags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("Simple Viewer 3D Installer", nullptr, windowFlags);

    ImGui::Image(data->logoTexId, data->logoSize); 
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (data->logoSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2);

    ImGuiIO& io = ImGui::GetIO(); 
    switch (data->currentPage) {

    case guiPage_NotEnoughSpace: {

        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::Text("Not enough space on drive %c:\\", data->guiInstallDir[0]);
        ImGui::PopFont();

        ImGui::Text("Pick a different drive");
        if (ImGui::Button("Next")) data->currentPage = guiPage_installDir;

        break;

    }
    case guiPage_license: {

        ImGui::PushFont(io.Fonts->Fonts[1]); 
        ImGui::Text("License Agreement");
        ImGui::PopFont();

        ImVec2 childSize;
        childSize.x = 0.0f; 
        childSize.y = ImGui::GetContentRegionAvail().y * 0.8f;
        ImGui::BeginChild("Lisence Description", childSize, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::TextWrapped(license);
        ImGui::EndChild(); 
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
        if (data->userAcceptText) 
            ImGui::TextWrapped("Click on the check box below to accept the license. App cannot be installed without accepting the license.");
        ImGui::PopStyleColor(); 
        
        ImGui::Checkbox("I accept the terms in the license agreement", &data->termsAccepted);
        bool btn = ImGui::Button("Next");
        if (btn && data->termsAccepted)  incr(data->currentPage);
        if (btn && !data->termsAccepted) data->userAcceptText = true;
        if (data->termsAccepted)         data->userAcceptText = false;

        break;
    }
    case guiPage_installDir: {

        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::Text("Install Location");
        ImGui::PopFont(); 

        float childHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing() + 2 * data->guiSizes.windowPadding.y;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse; 
        ImGui::BeginChild("Install Location", ImVec2(0, childHeight), ImGuiChildFlags_Borders, windowFlags);
        ImGui::Text(data->guiInstallDir.c_str()); 
        data->openFolderDialog = ImGui::Button("Change Install Location");
        ImGui::EndChild();

        ImGui::Checkbox("Create Desktop Shortcut", &data->desktopShortcut);
        if (ImGui::Button("Back"))    { decr(data->currentPage); } ImGui::SameLine();
        if (ImGui::Button("Install")) { incr(data->currentPage); data->install = true; } 
        break;

    }
    case guiPage_installing: {

        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::Text("Installing...");
        ImGui::PopFont();
        break;

    }
    case guiPage_finish: {

        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::Text("Simple Viewer 3D installed successfully!");
        ImGui::PopFont();
        ImGui::Checkbox("Launch once finished", &data->launchOnFinish);
        if (ImGui::Button("Finish")) PostQuitMessage(0);

        break;

    }

    }

    ImGui::End(); 

#ifdef DEBUG
    ImGui::ShowDemoWindow();
#endif 

    ImGui::EndFrame();
}

void Gui::destroy() {

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

}