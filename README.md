# Simple Viewer 3D

![Logo](Logo.svg)

## Install Prebuilt .exe

You can download an installer from [here](https://simpleviewer3d.netlify.app/). 

## What is it?

Simple Viewer 3D is a Windows only app to view 3D files using the Vulkan API. It's designed to be simple and fast. 
It launches fast, is responsive, and loads files quickly. 

![App Gui](https://i.imgur.com/tSg15Rs.png)

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

3. Compile with your build system using one of the following configurations.

| Build | Description |
|:------|:--------------------------------------------|
| Dist | Fully optimized build for distribution. You get this build when using the [installer](https://github.com/Zachhaaa/SimpleViewer3Dinstaller). |
| DevRelease |Same as Dist build, but there is a window that contains performance timing and information. |
| Debug | Standard debug build, more windows with app information, and Vulkan validation layers are enabled. |
| OptDebug | Same as Debug build, but optimization flags are on.  |

## Related Repositories 

One of my goals for this project was to make it so anyone could [install](https://simpleviewer3d.netlify.app/) and use it.
Because of that, there are 4 other repos associated with this project that make it possible.

| Repo | Description |
|:------|:--------------------------------------------|
| [SimpleViewer3Dinstaller](https://github.com/Zachhaaa/SimpleViewer3Dinstaller) | Portable .exe for installing SimpleViewer3D |
| [SimpleViewer3Dlauncher](https://github.com/Zachhaaa/SimpleViewer3Dlauncher) | Installed with installer and used when you use "open with" to open a file in an app on windows |
| [SimpleViewer3Dsite](https://github.com/Zachhaaa/SimpleViewer3Dsite) | Repo for the [SimpleViewer3d website](https://simpleviewer3d.netlify.app/) |
| [SimpleViewer3Duninstaller](https://github.com/Zachhaaa/SimpleViewer3Duninstaller) | Installed with installer, so the user can uninstall the app.   |