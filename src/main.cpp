#include "App.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	Core::Instance mainInstance;
	App::InstanceInfo initInfo = { hInstance, nCmdShow, pCmdLine }; 

	App::init(&mainInstance, initInfo);

	while (true) {
		
		MSG msg = {};
		if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		App::render(&mainInstance);
	}

	App::close(&mainInstance);

	return 0;
}