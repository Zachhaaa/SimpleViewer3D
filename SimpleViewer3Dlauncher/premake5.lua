project "SimpleViewer3Dlauncher"
    kind       "WindowedApp"
    language   "C++"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"

    includedirs "%{wks.location}/Dependencies/*"

    files { "*.cpp", "*.hpp", "*.h", "resources.rc" }

	flags { "MultiProcessorCompile" }

    defines     { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug or configurations:OptDebug"
        defines { "DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:DevRelease or configurations:Dist"
        defines { "NDEBUG" } 
        optimize "Speed"
        inlining "Auto"
        runtime  "Release"
        symbols  "Off"
        flags   { "LinkTimeOptimization" }

    filter "platforms:x64"
        architecture "x86_64"