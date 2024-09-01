#pragma once

#include <Windows.h>

#define arraySize(array) (sizeof(array) / sizeof(array[0]))

namespace App {

	void init(HINSTANCE hInstance, int nCmdShow);
	void render();
	void close(); 

}