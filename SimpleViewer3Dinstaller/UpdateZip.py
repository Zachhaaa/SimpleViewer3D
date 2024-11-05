import zipfile

zipf = zipfile.ZipFile("ZipToInstall/Simple Viewer 3D.zip", "w")
zipf.write("../SimpleViewer3D/Logo.svg",                                    "Simple Viewer 3D/Logo.svg")
zipf.write("../SimpleViewer3D/Mouse-Controls.svg",                          "Simple Viewer 3D/Mouse-Controls.svg")
zipf.write("../SimpleViewer3D/bin/Dist/Simple Viewer 3D.exe",               "Simple Viewer 3D/Simple Viewer 3D.exe")
zipf.write("../SimpleViewer3Dlauncher/bin/Dist/SimpleViewer3Dlauncher.exe", "Simple Viewer 3D/SimpleViewer3Dlauncher.exe")
zipf.write("../SimpleViewer3Duninstaller/bin/Dist/Uninstall.exe",           "Simple Viewer 3D/Uninstall.exe")

print("Created Simple Viewer 3D.zip")