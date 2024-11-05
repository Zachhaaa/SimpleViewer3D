#pragma once

struct CustomIniData {

    int sensitivity; 

    int windowWidth, windowHeight;
    int windowPosX, windowPosY;
    int windowMaximized; // 1 = maxmized. int for file read reasons

};
/// @return true = success, false = failure/ini doesn't exist. 
bool getCustomIniData(CustomIniData* dataIn, const char* iniPath);
void setCustomIniData(const CustomIniData& dataOut, const char* iniPath);