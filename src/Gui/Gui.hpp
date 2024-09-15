#pragma once

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <imgui_operators.h>


namespace Gui {

enum Commands {

	cmd_null,
	cmd_minimizeWindow,
	cmd_maximizeWindow,
	cmd_restoreWindow,
	cmd_closeWindow,
	cmd_openDialog,
	cmd_resizeViewport,

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

// Move the following constants elsewhere
struct GuiSizes {

	float fontSize = 16.0;
	float titleBarHeight = 28.0;
	float wndBtnWidth = 40.0f;
	float menuBarStartExtent = 12.0f;
	float menuBarEndExtent = 44.0f;
		
};

struct DisplayData {

	float   frameWaitTimesGraph[200];
	uint32_t resizeCount;

};

struct ViewportRenderData {

	bool        orbitActive;
	glm::vec2   orbitAngle;
	ImVec2      viewportSize;
	ImTextureID framebufferTexID; 

};

struct DrawData {

	GuiSizes            guiSizes; 
	DisplayData         stats{};
	ViewportRenderData  vpData{};

};

void init    (InitInfo* initInfo, GuiSizes* guiSizes, float guiDpi);
void draw    (HWND hwnd, Commands* cmdListPtr, DrawData* data);
void destroy (); 
	
}