#include "Core/Core.hpp"
#include <Timer.hpp>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	App::init(hInstance, nCmdShow);


	double startTime = getTime();
	while (true) {
		
		MSG msg = {};
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		App::render();

		
	}

	App::close();

	return 0;
}