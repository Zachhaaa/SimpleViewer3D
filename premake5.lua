workspace "SimpleViewer3D" 
    configurations { "Debug", "OptDebug", "Release" }
    platforms      { "x64" }

project "SimpleViewer3Dapp"
    kind       "WindowedApp"
    language   "C++"
    cppdialect "C++17"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"

    includedirs {
        "%{os.getenv('VULKAN_SDK')}/include",
        "Dependencies/glm",
        "Dependencies/imgui/backends",
        "Dependencies/imgui",
        "Dependencies/TimerApi"
    }

    libdirs     { "%{os.getenv('VULKAN_SDK')}/lib" }
    links       { "vulkan-1" }

    files {
        "src/**.cpp", 
        "src/**.hpp", 
        "src/**.h", 
        "src/shaders/**.vert", 
        "src/shaders/**.frag",
        "Dependencies/imgui/*.cpp",
        "Dependencies/imgui/*.h",
        "Dependencies/imgui/backends/imgui_impl_vulkan.cpp",
        "Dependencies/imgui/backends/imgui_impl_vulkan.h",
        "Dependencies/imgui/backends/imgui_impl_win32.cpp",
        "Dependencies/imgui/backends/imgui_impl_win32.h",
        "Dependencies/TimerApi/*.cpp",
        "Dependencies/TimerApi/*.hpp"
    }

    defines     { "_CRT_SECURE_NO_WARNINGS" }

    floatingpoint    "Fast"
    vectorextensions "AVX2"

    staticruntime "On"

    filter "files:**.vert or files:**.frag"
        buildmessage "Compiling shader: %{file.relpath}"
        buildcommands "glslc %{file.relpath} -o res/shaders/%{file.name}.spv -O"
        buildoutputs  "res/shaders/%{file.name}.spv"
    
    -- Standard Debug mode.
    filter "configurations:Debug"
        defines { "DEBUG", "ENABLE_VK_VALIDATION_LAYERS" }
        symbols "On"
        runtime "Debug"

    -- Debug mode with opimizations in Debug is unussably slow.
    filter "configurations:OptDebug"
        defines  { "DEBUG" }
        symbols "On"
        optimize "Speed"
        inlining "Auto"
        runtime "Release"

    -- Standard, fully optmized release build. 
    filter "configurations:Release"
        defines  { "NDEBUG" }
        optimize "Speed"
        symbols  "Off"
        inlining "Auto"
        runtime "Release"
        flags { "LinkTimeOptimization" }

    filter "platforms:x64"
        architecture "x86_64"