#pragma once

#include <imgui.h>


/// width / height
constexpr float c_WindowAspectRatio = 1.5;
/// MonitorWidth(px) * percentSize or MonitorWidth(px) * percentSize. Smaller value is used and the other width is calculated based on the aspect ratio. 1.0 maximizes the window and aspect ratio is ignored.
constexpr float c_WindowPercentSize = 0.85;
/// Min window width and height
constexpr int c_minWidth = 300, c_minHeight = 300; 

namespace c_vlkn {

constexpr VkColorSpaceKHR  colorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
constexpr VkFormat         format      = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR; 
constexpr uint32_t         imageCount  = 2; 

constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

}

namespace c_style {

/*
* Color are listed from the most used color to be color 1, and the
* highest number color to be the least use color of the theme. 
* For example:
* col1 = background colors
* col2 = border colors
* col3 = hover colors
*/
constexpr ImVec4 col1      = ImGui::rgba32toVec4(15, 15, 15, 255);
constexpr ImVec4 col2      = ImGui::rgba32toVec4(20, 20, 20, 255);
constexpr ImVec4 col3      = ImGui::rgba32toVec4(40, 40, 40, 255);
constexpr ImVec4 activeCol = ImGui::rgba32toVec4(69, 61, 40, 255);
constexpr ImVec4 hoverCol  = ImGui::rgba32toVec4(113, 100, 65, 255);

}