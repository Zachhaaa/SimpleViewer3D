# Simple Viewer 3D

![Logo](SimpleViewer3D/Logo.svg)

## Install Prebuilt .exe

You can download an installer from [here](https://simpleviewer3d.netlify.app/). 

## What is it?

Simple Viewer 3D is a Windows only app to view 3D files using the Vulkan API. It's designed to be simple and fast. 
It launches fast, is responsive, and loads files quickly. 

![App Gui](https://simpleviewer3d.netlify.app/SV3Dexample.png)

### Supported file types
 - .stl
 - .obj

I know that is not much, but it uses my optimized file parser (I actually haven't measured how it compares to other libraries, it might be slower for all I know lol). 

## Building
### Prerequisites
You will need the following preinstalled in order to build the project.

 - [Premake 5](https://premake.github.io/)
 - [Vulkan SDK](https://vulkan.lunarg.com/)
 - [Python](https://www.python.org/) (For custom build scripts)
 - [Premake 5 compatible compiler](https://premake.github.io/docs/Using-Premake)
 - Windows (app is not cross platform)

### Build Process

1. Run a recursive clone. 
```bash
git clone https://github.com/Zachhaaa/SimpleViewer3D.git --recurse-submodules
```
2. Run Premake while in the cloned repo's directory.
```bash
# For Visual Studio 2022
premake5 vs2022
```
Premake commands for different compilers can be found [here](https://premake.github.io/docs/Using-Premake).

3. There are 4 projects in this repo all are described below. Choose the one you want to build with your build system. 

__IMPORTANT__: Make sure you don't build the installer until you build the first 3 Dist builds below. Make sure you follow the build [instructions](#Building-the-Installer) for the installer.

 | Project | Description |
|:------|:--------------------------------------------|
| __SimpleViewer3D__ | The main app pictured above. |
| __SimpleViewer3Dlauncher__ | Installed with installer, and used when you use "open with" to open a file in an app on windows. |
| __SimpleViewer3Duninstaller__ | Installed with installer, so the user can uninstall the app.   |
| __SimpleViewer3Dinstaller__  | Portable, standalone .exe for installing SimpleViewer3D. |

4. Choose a build type as descibed below

| Build | Description |
|:------|:--------------------------------------------|
| Dist | Fully optimized build for distribution. You get this build when using the [installer](https://simpleviewer3d.netlify.app/). |
| DevRelease | Same as Dist build, but there is a window that contains performance timing and information. |
| Debug | Standard debug build, more windows with app information, and Vulkan validation layers are enabled. |
| OptDebug | Same as Debug build, but optimization flags are on.  |

For the SimpleViewer3Dlauncher and SimpleViewer3Duninstaller projects.

 - DevRelease is identical to Dist build
 - OptDebug is identical to Debug build (They are so small that I didn't think that adding an optimized debug build was needed). 

## Building the Installer

1. Build Dist builds for

 - __SimpleViewer3D__
 - __SimpleViewer3Dlauncher__
 - __SimpleViewer3Duninstaller__

2. Run the following command, this must be done anytime you make changes to any of the 3 projects above because the python script puts those in a .zip to be integrated into the installer .exe.
```bash
python SimpleViewer3Dinstaller/UpdateZip.py
```
3. Choose Build type, and Build the project.
