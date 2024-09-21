#pragma once

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <imgui_operators.h>

#include <memory>
#include <vector>

namespace Gui {

enum Commands {

	cmd_null,
	cmd_minimizeWindow,
	cmd_maximizeWindow,
	cmd_restoreWindow,
	cmd_closeWindow,
	cmd_openDialog,

	cmdCount // Use this for the number of enum values in this enum

};

struct InitInfo {

	HWND             hwnd;
	VkInstance       instance;
	VkPhysicalDevice physicalDevice;
	VkDevice         device;
	uint32_t         graphicsQueueIndex;
	VkQueue          graphicsQueue; 
	VkDescriptorPool descriptorPool;
	VkRenderPass     renderPass; 
	uint32_t         subpass;
	uint32_t         imageCount;

};

struct GuiSizes {

	float fontSize = 16.0;
	float titleBarHeight = 28.0;
	float wndBtnWidth = 40.0f;
	float menuBarStartExtent = 12.0f;
	float menuBarEndExtent = 44.0f;
		
};

struct PerformanceTimes {

	float openFile;
	float fileClose;
	float viewportResize;
	float appLaunch;
	float renderingCommands;

};

struct DisplayData {

	float            frameWaitTimesGraph[200];
	uint32_t         resizeCount;
	PerformanceTimes perfTimes;

};

struct ViewportGuiData {

	bool        open;
	bool        visible; 
	bool        resize;
	bool        orbitActive;
	std::unique_ptr<char[]> objectName;
	glm::vec2   orbitAngle;
	ImVec2      m_size;
	ImTextureID framebufferTexID; 
	uint32_t    triangleCount;

};

struct DrawData {

	GuiSizes            guiSizes;
#ifdef DEVINFO
	DisplayData         stats{};
#endif
	std::vector<ViewportGuiData> vpDatas;

};

void init    (InitInfo* initInfo, GuiSizes* guiSizes, float guiDpi);
void draw    (HWND hwnd, Commands* cmdListPtr, DrawData* data);
void destroy (); 
	
}