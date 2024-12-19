#include "App.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	Core::Instance mainInstance;
	App::InstanceInfo initInfo = { hInstance, nCmdShow, pCmdLine }; 

	App::init(&mainInstance, initInfo);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);

		App::runCycle(&mainInstance);
	}

	App::close(&mainInstance);

	return 0;
}