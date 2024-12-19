#pragma once

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>

#include <memory>
#include <vector>

namespace Gui {

typedef uint32_t Commands;
enum _Commands : uint32_t {

	cmd_minimizeWindowBit = 1 << 0,
	cmd_maximizeWindowBit = 1 << 2,
	cmd_restoreWindowBit  = 1 << 3,
	cmd_closeWindowBit    = 1 << 4,
	cmd_openDialogBit     = 1 << 5,
	// When adding new commands, make sure you adjust the c_cmdCount below. 

};
constexpr size_t c_cmdCount = 5; 
static_assert(c_cmdCount <= sizeof(Commands) * 8);

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
	const char*      iniPath;

};

struct GuiSizes {

	float fontSize;
	float largeFontSize;
	float titleBarHeight;
	float wndBtnWidth;
	float menuBarStartExtent;
	float menuBarEndExtent;
	float mouseControlsTextVerticalOffset;

};
struct GuiColors {

	ImVec4 shortcutText;
	ImVec4 mainWndBackground; 

};

struct GuiStyleEx {

	GuiSizes  sizes;
	GuiColors colors;

};

struct PerformanceTimer {

	float time; // seconds
	const char* label; 

};

struct PerformanceTimes {

	PerformanceTimer timers[50]; 
	int timerCount = 0; 

	float* getTimer(const char* label) { 
		for (int i = 0; i < timerCount; i++) {
			for (int j = 0; timers[i].label[j] == label[j]; j++) { if (label[j] == '\0') return &timers[i].time; }
		}
		PerformanceTimer* pTimer = &timers[timerCount];
		timerCount++; 
		pTimer->label = label; 
		return &pTimer->time;  
	}

}; 

struct AppStats {

	uint32_t         resizeCount;
	PerformanceTimes perfTimes; 

};

struct ViewportGuiData {

	bool                    open;
	bool                    visible; 
	bool                    resize;
	bool                    orbitActive;
	bool                    panActive;
	bool                    showEdges;
	std::unique_ptr<char[]> objectName;
	glm::mat4               model;
	glm::vec3               modelCenter;
	float                   zoomDistance;
	float                   zoomMin;
	float                   farPlaneClip;
	ImVec2                  size;
	ImTextureID             framebufferTexID; 
	uint32_t                indexCount;
	uint32_t                uniqueVertexCount;
	bool                    isTextFormat; 

	glm::vec2& panPos() { return *(glm::vec2*)&model[3]; }

};

struct DrawData {

	GuiStyleEx       styleEx;
	ImTextureID      logoTexID; 
	ImTextureID      icoTexID;
	ImTextureID      mouseControlsTexID;
	ImVec2           logoSize;
	ImVec2           mouseControlsSize; 
	ViewportGuiData* lastFocusedVp;
	float            sensitivity; 
#ifdef DEVINFO
	AppStats stats{};
#endif
	std::vector<ViewportGuiData> vpDatas;

};

void init    (InitInfo* initInfo, GuiStyleEx* styleEX, float guiDpi);
void draw    (HWND hwnd, Commands* commands, DrawData* data);
void destroy (); 
	
}