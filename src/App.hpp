 #pragma once

#include "Core.hpp"

#include <Windows.h>

namespace App {

struct InstanceInfo {

    HINSTANCE hInstance;
    int       nCmdShow; 

};

void init   (Core::Instance* inst, InstanceInfo* initInfo);
void render (Core::Instance* inst);
void close  (Core::Instance* inst);
                
}