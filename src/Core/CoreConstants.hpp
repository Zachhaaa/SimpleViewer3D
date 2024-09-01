#pragma once

/// width / height
constexpr float c_WindowAspectRatio = 1.5;
/// MonitorWidth(px) * percentSize or MonitorWidth(px) * percentSize. Smaller value is used and the other width is calculated based on the aspect ratio. 1.0 maximizes the window and aspect ratio is ignored.
constexpr float c_WindowPercentSize = 0.85;

namespace c_vlkn {

	constexpr VkColorSpaceKHR  colorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	constexpr VkFormat         format      = VK_FORMAT_R8G8B8A8_UNORM;
	constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR; 
	constexpr uint32_t         imageCount  = 2; 

}
