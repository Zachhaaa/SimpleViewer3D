project "Simple Viewer 3D"
    kind       "WindowedApp"
    language   "C++"
    cppdialect "C++17"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"

    if not os.isdir "bin/shaderObjs" then
        os.mkdir "bin/shaderObjs"
    end

    includedirs {
        "%{os.getenv('VULKAN_SDK')}/include",
        "%{wks.location}/Dependencies/*",
        "%{wks.location}/Dependencies/imgui/backends",
    }

    libdirs     { "%{os.getenv('VULKAN_SDK')}/lib" }
    links       { "vulkan-1", "Shcore"}

    -- So these files are actually included in the compile list
    -- The actual code for these files is generated when compiling the shaders
    os.execute "python TouchShaderArrays.py"

    files {
        "src/**.cpp", 
        "src/**.hpp", 
        "src/**.h", 
        "src/shaders/**.vert", 
        "src/shaders/**.frag",
        "%{wks.location}/Dependencies/*/*.cpp",
        "%{wks.location}/Dependencies/*/*.hpp",
        "%{wks.location}/Dependencies/*/*.c",
        "%{wks.location}/Dependencies/*/*.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_win32.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_vulkan.cpp",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_vulkan.h",
        "%{wks.location}/Dependencies/imgui/backends/imgui_impl_win32.cpp",
        "%{wks.location}/Dependencies/glm/glm/**.hpp",
        "resources.rc"
    }

    defines     { "_CRT_SECURE_NO_WARNINGS" }

    floatingpoint    "Fast"
    vectorextensions "AVX2"

    filter "files:**.vert or files:**.frag"
        buildmessage "Compiling shader: %{file.relpath}"
        buildcommands { 
            "glslc %{file.relpath} -o bin/shaderObjs/%{file.name}.spv -O", 
            "python ../FileToArray.py bin/shaderObjs/%{file.name}.spv src/FileArrays/" 
        } 
        buildoutputs {
            "bin/shaderObjs/%{file.name}.spv",
            "src/FileArrays/*.cpp",
            "src/FileArrays/*.h",
        }
    
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