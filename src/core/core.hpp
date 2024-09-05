#pragma once

#include <Windows.h>

#define arraySize(array) (sizeof(array) / sizeof(array[0]))
 
struct GuiSizes {

	float fontSize           = 16.0;
	float titleBarHeight     = 28.0;
	float wndBtnWidth        = 40.0f;
	float menuBarStartExtent = 20.0f;

};
extern GuiSizes s_gui;

namespace App {

	void init(HINSTANCE hInstance, int nCmdShow);
	void render();
	void close(); 

}