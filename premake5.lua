workspace "SimpleViewer3D" 
    configurations { "Debug", "OptDebug", "DevRelease", "Dist" }
    platforms      { "x64" }

project "SimpleViewer3Dapp"
    kind       "WindowedApp"
    language   "C++"
    cppdialect "C++17"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"

    includedirs {
        "%{os.getenv('VULKAN_SDK')}/include",
        "Dependencies/*",
        "Dependencies/imgui/backends",
    }

    libdirs     { "%{os.getenv('VULKAN_SDK')}/lib" }
    links       { "vulkan-1", "Shcore"}

    os.execute "touch src/FileArrays/shader_frag_spv.cpp"
    os.execute "touch src/FileArrays/shader_frag_spv.h"
    os.execute "touch src/FileArrays/shader_vert_spv.cpp"
    os.execute "touch src/FileArrays/shader_vert_spv.h"

    files {
        "src/**.cpp", 
        "src/**.hpp", 
        "src/**.h", 
        "src/shaders/**.vert", 
        "src/shaders/**.frag",
        "Dependencies/**.cpp",
        "Dependencies/**.hpp",
    }
    removefiles {
        "Dependencies/imgui/**.cpp",
        "Dependencies/imgui/**.h",
        "Dependencies/glm/**.cpp",
        "Dependencies/glm/**.hpp",
    }
    files {
        "Dependencies/imgui/*.cpp",
        "Dependencies/imgui/*.h",
        "Dependencies/imgui/backends/imgui_impl_vulkan.cpp",
        "Dependencies/imgui/backends/imgui_impl_vulkan.h",
        "Dependencies/imgui/backends/imgui_impl_win32.cpp",
        "Dependencies/imgui/backends/imgui_impl_win32.h",
        "Dependencies/glm/glm/**.hpp",

    }

    defines     { "_CRT_SECURE_NO_WARNINGS" }

    floatingpoint    "Fast"
    vectorextensions "AVX2"

    staticruntime "On"

    filter "files:**.vert or files:**.frag"
        buildmessage "Compiling shader: %{file.relpath}"
        buildcommands { "glslc %{file.relpath} -o res/shaders/%{file.name}.spv -O", "python FileToArray.py res/shaders/%{file.name}.spv src/FileArrays/" } 
        buildoutputs  "res/shaders/%{file.name}.spv"
    
    -- Standard Debug mode
    filter "configurations:Debug"
        defines { "DEBUG", "ENABLE_VK_VALIDATION_LAYERS", "DEVINFO" }
        symbols "On"
        runtime "Debug"

    -- Debug mode with opimizations if Debug is unussably slow.
    filter "configurations:OptDebug"
        defines  { "DEBUG", "ENABLE_VK_VALIDATION_LAYERS", "DEVINFO" }
        symbols "On"
        optimize "Speed"
        inlining "Auto"
        runtime "Release"

    -- Fully optimized build with all of the developer info (Frame times, performance metrics, etc). 
    filter "configurations:DevRelease"
        defines  { "NDEBUG", "DEVINFO" }
        optimize "Speed"
        symbols  "Off"
        inlining "Auto"
        runtime "Release"
        flags { "LinkTimeOptimization" }

    --  Fully optimized build that will be the distributed copy. 
    filter "configurations:Dist"
        defines  { "NDEBUG" }
        optimize "Speed"
        symbols  "Off"
        inlining "Auto"
        runtime "Release"
        flags { "LinkTimeOptimization" }

    filter "platforms:x64"
        architecture "x86_64"