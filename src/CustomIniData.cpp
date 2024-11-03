#include "CustomIniData.hpp"

#include <cstdio> 
#include <string.h>
#include <cassert>

bool getCustomIniData(CustomIniData* dataIn) {
	FILE* ini = fopen("imgui.ini", "r"); 
	if (ini == nullptr)  return false;

	char line[100]; 
	bool OsWindowFound = false, PreferencesFound = false; 
	while (fgets(line, sizeof line, ini)) {
		if (memcmp("[OsWindow]", line, 10) == 0) {
			fgets(line, sizeof line, ini);
			int count = sscanf(line, "Pos=%i,%i", &dataIn->windowPosX, &dataIn->windowPosY);
			assert(count == 2); 
			fgets(line, sizeof line, ini);
			count = sscanf(line, "Size=%i,%i", &dataIn->windowWidth, &dataIn->windowHeight);
			assert(count == 2);
			fgets(line, sizeof line, ini);
			count = sscanf(line, "Maximized=%i", &dataIn->windowMaximized);
			assert(count == 1);
			OsWindowFound = true; 
		}
		else if (memcmp("[Preferences]", line, 13) == 0) {
			fgets(line, sizeof line, ini);
			int count = sscanf(line, "Sensitivity=%i", &dataIn->sensitivity);
			assert(count == 1);
			PreferencesFound = true; 
		}
	}

	fclose(ini); 

	return OsWindowFound && PreferencesFound; 

}

void setCustomIniData(const CustomIniData& dataOut) {
	FILE* ini = fopen("imgui.ini", "a");
	if (ini == nullptr) return; 

	fprintf(
		ini, 
		"[OsWindow]\nPos=%i,%i\nSize=%i,%i\nMaximized=%i\n\n", 
		dataOut.windowPosX, 
		dataOut.windowPosY,
		dataOut.windowWidth,
		dataOut.windowHeight,
		dataOut.windowMaximized
	);
	fprintf(ini, "[Preferences]\nSensitivity=%i\n\n", dataOut.sensitivity);

	fclose(ini);
}