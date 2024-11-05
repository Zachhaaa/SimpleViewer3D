#include <cstdio>
#include <cassert>
#include <string>

#include <Config.hpp>
#include <stringLib.h>

#include <Windows.h>
#include <lmcons.h> 

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	
	wchar_t installLoc[1024];
	DWORD size = sizeof installLoc; 
	const wchar_t* regKeyPath = concatW(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\", cfg::appNameNoSpaces);  

	LSTATUS err = RegGetValue(
		HKEY_LOCAL_MACHINE, 
		regKeyPath,
		L"InstallLocation",
		RRF_RT_REG_SZ,
		NULL,
		installLoc, 
		&size
	);
	assert(err == ERROR_SUCCESS && "Failed to get install location!");

	const wchar_t* exePath           = concatW(installLoc, L"\\Simple Viewer 3D.exe");
	const wchar_t* logoPath          = concatW(installLoc, L"\\Logo.svg");
	const wchar_t* mouseControlsPath = concatW(installLoc, L"\\Mouse-Controls.svg"); 
	const wchar_t* imguiIni          = concatW(installLoc, L"\\imgui.ini"); 
	const wchar_t* launcherPath      = concatW(installLoc, L"\\SimpleViewer3Dlauncher.exe");

	DeleteFileW(exePath); 
	DeleteFileW(logoPath);
	DeleteFileW(mouseControlsPath);
	DeleteFileW(imguiIni);
	DeleteFileW(launcherPath); 

	const wchar_t* startMenuDir = concatW(L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\", cfg::appName);
	const wchar_t* startMenuLnkStrs[] = { startMenuDir, L"\\", cfg::appName, L".lnk" }; 
	const wchar_t* startMenuLnk = concatMultipleW(startMenuLnkStrs, arraySize(startMenuLnkStrs));
	BOOL fileErr;
	fileErr = DeleteFileW(startMenuLnk);      assert(fileErr);
	fileErr = RemoveDirectoryW(startMenuDir); assert(fileErr); 
	
	// Delete desktop shortcut 
	{

		wchar_t username[UNLEN + 1];
		DWORD   usernameLen = UNLEN + 1;

		BOOL err = GetUserName(username, &usernameLen);
		assert(err != 0 && "Failed to Get username!");
		const wchar_t* desktopShortcutCloudPathStrs[] = { L"C:\\Users\\", username, L"\\OneDrive\\Desktop\\", cfg::appName, L".lnk" };
		const wchar_t* desktopShortcutCloudPath = concatMultipleW(desktopShortcutCloudPathStrs, arraySize(desktopShortcutCloudPathStrs));

		DeleteFileW(desktopShortcutCloudPath); 

		const wchar_t* desktopShortcutLocalPathStrs[] = { L"C:\\Users\\", username, L"\\Desktop\\", cfg::appName, L".lnk" };
		const wchar_t* desktopShortcutLocalPath = concatMultipleW(desktopShortcutLocalPathStrs, arraySize(desktopShortcutLocalPathStrs));

		DeleteFileW(desktopShortcutLocalPath);

		free((void*)desktopShortcutCloudPath);
		free((void*)desktopShortcutLocalPath);

	}

	LSTATUS keyDelErr = RegDeleteKeyW(HKEY_LOCAL_MACHINE, regKeyPath); assert(keyDelErr == ERROR_SUCCESS); 

	// Delete HKEY_CLASSES_ROOT\Applications\Simple Viewer 3D.exe 
	const wchar_t* appKeyStrs[] = { L"Applications\\", cfg::launcherName, L".exe" };
	const wchar_t* appKey     = concatMultipleW(appKeyStrs, arraySize(appKeyStrs));
	const wchar_t* commandKey = concatW(appKey, L"\\shell\\open\\command");
	const wchar_t* openKey    = concatW(appKey, L"\\shell\\open");
	const wchar_t* shellKey   = concatW(appKey, L"\\shell"); 
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, commandKey); assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, openKey);	  assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, shellKey);	  assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, appKey);	  assert(keyDelErr == ERROR_SUCCESS);

	// Delete HKEY_CLASSES_ROOT\SimpleViewer3d.stl
	const wchar_t* fileClass           = concatW(cfg::appNameNoSpaces, L"_file");
	const wchar_t* fileClassCommandKey = concatW(fileClass, L"\\shell\\open\\command");
	const wchar_t* fileClassOpenKey    = concatW(fileClass, L"\\shell\\open");
	const wchar_t* fileClassShellKey   = concatW(fileClass, L"\\shell");
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, fileClassCommandKey);  assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, fileClassOpenKey);	    assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, fileClassShellKey);    assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyW(HKEY_CLASSES_ROOT, fileClass);	        assert(keyDelErr == ERROR_SUCCESS);

	keyDelErr = RegDeleteKeyValueW(HKEY_CLASSES_ROOT, L".stl\\OpenWithProgids", fileClass); assert(keyDelErr == ERROR_SUCCESS);
	keyDelErr = RegDeleteKeyValueW(HKEY_CLASSES_ROOT, L".obj\\OpenWithProgids", fileClass); assert(keyDelErr == ERROR_SUCCESS);

	free((void*)regKeyPath); 
	free((void*)exePath); 
	free((void*)logoPath); 
	free((void*)mouseControlsPath); 
	free((void*)imguiIni); 
	free((void*)launcherPath); 
	free((void*)startMenuDir);
	free((void*)startMenuLnk);
	free((void*)appKey); 
	free((void*)commandKey); 
	free((void*)openKey); 
	free((void*)shellKey); 
	free((void*)fileClass);
	free((void*)fileClassCommandKey);
	free((void*)fileClassOpenKey);
	free((void*)fileClassShellKey);

	const wchar_t* msgBoxText = concatW(cfg::appName, L" uninstalled successfully."); 

	MessageBoxW(NULL, msgBoxText, cfg::appName, MB_OK | MB_ICONINFORMATION);

	free((void*)msgBoxText); 

	STARTUPINFOW si{};
	si.cb = sizeof si; 
	PROCESS_INFORMATION pi{};
	const wchar_t drive[3] = { installLoc[0], ':', '\0' }; 
	const wchar_t* commandStrs[] = { L"cmd.exe /c ", drive, L" && cd \"", installLoc , L"\" && del Uninstall.exe && cd ../ && rmdir \"", cfg::appName,  L"\"" };
	const wchar_t* command = concatMultipleW(commandStrs, arraySize(commandStrs)); 

	CreateProcessW(
		NULL, 
		(LPWSTR)command,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi
	);

	free((void*)command); 

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}