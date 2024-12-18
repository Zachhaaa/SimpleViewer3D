import zipfile
import os
import FileToArrayLib

zipf = zipfile.ZipFile("SimpleViewer3Dinstaller/ZipToInstall/Simple Viewer 3D.zip", "w", compression=zipfile.ZIP_DEFLATED)
zipf.write("SimpleViewer3D/Logo.svg",                                    "Simple Viewer 3D/Logo.svg")
zipf.write("SimpleViewer3D/Mouse-Controls.svg",                          "Simple Viewer 3D/Mouse-Controls.svg")
zipf.write("SimpleViewer3D/bin/Dist/Simple Viewer 3D.exe",               "Simple Viewer 3D/Simple Viewer 3D.exe")
zipf.write("SimpleViewer3Dlauncher/bin/Dist/SimpleViewer3Dlauncher.exe", "Simple Viewer 3D/SimpleViewer3Dlauncher.exe")
zipf.write("SimpleViewer3Duninstaller/bin/Dist/Uninstall.exe",           "Simple Viewer 3D/Uninstall.exe")

zipf.close()

print("Created Simple Viewer 3D.zip")

FileToArrayLib.ConvertFileToArray("SimpleViewer3D/Logo.svg", "SimpleViewer3Dinstaller/src/FileArrays/")
FileToArrayLib.ConvertFileToArray("SimpleViewer3Dinstaller/ZipToInstall/Simple Viewer 3D.zip", "SimpleViewer3Dinstaller/src/FileArrays/")