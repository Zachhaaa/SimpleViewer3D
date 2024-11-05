#include <Windows.h>
#include <stringLib.h>
#include <Config.hpp>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	
	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo); 
	PROCESS_INFORMATION processInfo{};
	
	// Extract the working dir the command line args.
	if (pCmdLine[0] == L'\0' || pCmdLine[0] != L'\"') return -1;
	int workingDirEndIndex = 1;
	for (; pCmdLine[workingDirEndIndex] != L'\"'; workingDirEndIndex++) {
		if (pCmdLine[workingDirEndIndex] == L'\0') return -2; 
	}
	int& workingDirSize = workingDirEndIndex; 
	wchar_t* workingDir = (wchar_t*)malloc(workingDirSize * sizeof(wchar_t)); if (workingDir == nullptr) return -3; 
	memcpy((void*)workingDir, &pCmdLine[1], (workingDirSize - 1) * sizeof(wchar_t)); 
	workingDir[workingDirSize - 1] = L'\0'; 

	const wchar_t* exePathStrs[] = { L"\"", workingDir, L"\\", cfg::appName, L".exe\"" };
	const wchar_t* exePath = concatMultipleW(exePathStrs, arraySize(exePathStrs));

	if (pCmdLine[workingDirEndIndex + 1] != L' ' && pCmdLine[workingDirEndIndex + 2] != L'\"') return -4;
	wchar_t* commandLine = concatW(exePath, &pCmdLine[workingDirEndIndex + 1]);

	CreateProcessW(
		NULL,
		commandLine,
		NULL, 
		NULL, 
		FALSE,
		0, 
		NULL, 
		workingDir,
		&startupInfo,
		&processInfo
	);

	CloseHandle(processInfo.hProcess); 
	CloseHandle(processInfo.hThread); 

	free((void*)workingDir);
	free((void*)exePath);
	free((void*)commandLine); 

	return 0; 

}