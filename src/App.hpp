 #pragma once

#include "Core.hpp"

#include <Windows.h>

namespace App {

struct InstanceInfo {

    HINSTANCE       hInstance;
    int             nCmdShow; 
    const char*     fileToOpen; 

};

void init   (Core::Instance* inst, const InstanceInfo& initInfo);
void render (Core::Instance* inst);
void close  (Core::Instance* inst);
                
}