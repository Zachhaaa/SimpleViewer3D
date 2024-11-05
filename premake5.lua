workspace "Simple Viewer 3D" 
    configurations { "Debug", "OptDebug", "DevRelease", "Dist" }
    platforms      { "x64" }

startproject "SimpleViewer3D"

include "SimpleViewer3D"
include "SimpleViewer3Dlauncher"
include "SimpleViewer3Duninstaller"
include "SimpleViewer3Dinstaller"