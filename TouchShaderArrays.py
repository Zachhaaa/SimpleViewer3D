import os

shaderDir  = "src/Shaders/"
fileArrDir = "src/FileArrays/" 
shaderFiles = os.listdir(shaderDir)

for shaderFile in shaderFiles: 
    arrName = shaderFile.replace(".", "_") 
    cppFile = fileArrDir + arrName + "_spv.cpp"
    hFile   = fileArrDir + arrName + "_spv.h"
    open(cppFile, 'a')
    open(hFile, "a")