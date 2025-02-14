#pragma once

/// width / height
constexpr float c_WindowAspectRatio = 1.5;
/// MonitorWidth(px) * percentSize or MonitorWidth(px) * percentSize. Smaller value is used and the other width is calculated based on the aspect ratio. 1.0 maximizes the window and aspect ratio is ignored.
constexpr float c_WindowPercentSize = 0.85;
/// Min window width and height
constexpr int c_minWidth = 300, c_minHeight = 300; 

namespace c_vlkn {

constexpr VkColorSpaceKHR       colorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
constexpr VkFormat              format            = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkSampleCountFlagBits sampleCount       = VK_SAMPLE_COUNT_8_BIT;
constexpr VkPresentModeKHR      presentMode       = VK_PRESENT_MODE_MAILBOX_KHR; // NOTE: The framerate is limited by a sleep (check bottom of App::render()) to reduce input lag but not render unnecessary frames.
constexpr VkPresentModeKHR      backupPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
constexpr uint32_t              imageCount        = 2; 
constexpr uint32_t              maxSets           = 101;

constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

}