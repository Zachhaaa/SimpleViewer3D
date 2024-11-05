import sys

readFile = open(sys.argv[1], 'rb')

last_slash = sys.argv[1].rfind('/')
if last_slash == -1: 
    last_slash = 0
else:
    last_slash += 1

arrayName = sys.argv[1][last_slash:].replace(".", "_")
arrayName = arrayName.replace(" ", "_")
readFile.seek(0, 2); 
outSourceFileStr = "unsigned char " + arrayName + "[" + str(readFile.tell()) + "] = {\n"
readFile.seek(0, 0); 

while True: 
    outSourceFileStr += '    '
    bytes_chunk = readFile.read(20)
    if not bytes_chunk: 
        break
    for byte in bytes_chunk: 
        outSourceFileStr += f"0x{byte:02X},"
    outSourceFileStr += '\n'

outSourceFileStr += "};"

outSourceFile = open(sys.argv[2] + arrayName + ".cpp", "w")
outSourceFile.write(outSourceFileStr)

print(f"Created {arrayName}"); 

outHeaderFile = open(sys.argv[2] + arrayName + ".h", "w")
outHeaderFile.write(f"#pragma once\nextern unsigned char {arrayName}[{readFile.tell()}];")

outHeaderFile.close()
outSourceFile.close()
readFile.close()