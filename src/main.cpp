#include "App.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	Core::Instance mainInstance;
	App::InstanceInfo initInfo = { hInstance, nCmdShow }; 

	App::init(&mainInstance, &initInfo);

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