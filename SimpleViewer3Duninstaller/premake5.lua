project "Uninstall"
    kind       "WindowedApp"
    language   "C++"
    cppdialect "C++17"
    targetdir  "bin/%{cfg.buildcfg}" 
    objdir     "bin/obj"

    includedirs "%{wks.location}/Dependencies/*"

    files {
        "*.cpp",
        "*.hpp",
        "*.h",
		"resources.rc"
    }
	
	flags { "NoManifest", "MultiProcessorCompile" }
	
    defines     { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug or configurations:OptDebug"
        defines { "DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:DevRelease or configurations:Dist"
        defines  { "NDEBUG" }
        optimize "On"
        symbols  "Off"
        inlining "Auto"
        runtime "Release"
        flags { "LinkTimeOptimization" }

    filter "platforms:x64"
        architecture "x86_64"