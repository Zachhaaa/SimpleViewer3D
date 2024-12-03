project "SimpleViewer3Dinstaller"
    kind       "WindowedApp"
    language   "C++"
    cppdialect "C++17"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"
        
    includedirs {
        "%{os.getenv('VULKAN_SDK')}/include",
        "%{wks.location}/Dependencies/*",
        "%{wks.location}/Dependencies/imgui/backends",
    }
    
    libdirs     { "%{os.getenv('VULKAN_SDK')}/lib" }
    links       { "vulkan-1", "Shcore"}
	
	os.execute "python AddFileArrays.py"
    
    files {
        "src/**.cpp", 
        "src/**.hpp", 
        "src/**.h", 
        "%{wks.location}/Dependencies/*/*.cpp",
        "%{wks.location}/Dependencies/*/*.hpp",
        "%{wks.location}/Dependencies/*/*.c",
        "%{wks.location}/Dependencies/*/*.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_win32.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_vulkan.cpp",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_vulkan.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_win32.cpp",
        "resources.rc",

    }

    flags { "NoManifest", "MultiProcessorCompile" }

    defines     { "_CRT_SECURE_NO_WARNINGS" }

    floatingpoint    "Fast"
    vectorextensions "AVX2"

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