#pragma once

#include <Windows.h>

#ifdef DEBUG
#define ENABLE_VK_VALIDATION_LAYERS
#endif

#define arraySize(array) (sizeof(array) / sizeof(array[0]))

namespace App {

	void init(HINSTANCE hInstance, int nCmdShow);
	void render();
	void close(); 

}