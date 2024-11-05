#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>

#include <string>

namespace Gui {

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

	float  fontSize;
	float  largeFontSize; 
	float  childBorderSize;
	float  childRounding;
	float  frameRounding;
	ImVec2 framePadding;
	ImVec2 windowPadding;
	ImVec2 childPadding; 
	ImVec2 itemSpacing; 

};

enum GuiPage : int {
	
	guiPage_NotEnoughSpace,
	guiPage_license,
	guiPage_installDir,
	guiPage_installing,
	guiPage_finish, 

	guiPageCount // Use this for the number of values in this 

};
inline GuiPage incr(GuiPage& v) { return v = (GuiPage)((int)v + 1); }
inline GuiPage decr(GuiPage& v) { return v = (GuiPage)((int)v - 1); }

struct DrawData {

	GuiSizes guiSizes;
	
	bool            termsAccepted    = false; 
	bool            userAcceptText   = false; 
	bool            desktopShortcut  = true;
	bool            launchOnFinish   = true;
	bool            openFolderDialog = false;
	bool            install          = false;
	GuiPage         currentPage      = guiPage_license;
	ImTextureID     logoTexId; 
	ImVec2          logoSize; 
	std::string     guiInstallDir = "C:\\Program Files\\Simple Viewer 3D";

};

void init(InitInfo* initInfo, GuiSizes* guiSizes, float guiDpi);
void draw(DrawData* data);
void destroy();

}