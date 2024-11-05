#pragma once
namespace cfg {

const wchar_t* appName         = L"Simple Viewer 3D"; 
const wchar_t* appNameNoSpaces = L"SimpleViewer3D";
const wchar_t* version         = L"1.0"; 
const wchar_t* launcherName    = L"SimpleViewer3Dlauncher";

/// Used to test if the drive has enough space for the app.
/// Size is in bytes.
constexpr size_t approxAppSize = (size_t)1e6; 

}