import os
import FileToArrayLib
shaderDir    = "src/Shaders/"
fileArrDir   = "src/FileArrays/" 
shaderBinDir = "bin/shaderObjs/"
shaderFiles = os.listdir(shaderDir)

for shaderFile in shaderFiles: 
    arrName = shaderFile.replace(".", "_") 
    cppFile = fileArrDir + arrName + "_spv.cpp"
    hFile   = fileArrDir + arrName + "_spv.h"
    open(cppFile, 'a')
    open(hFile, "a")
    spvFilePath = shaderBinDir + shaderFile + ".spv"; 
    if os.path.exists(spvFilePath): 
        FileToArrayLib.ConvertFileToArray(spvFilePath, "src/FileArrays/")
