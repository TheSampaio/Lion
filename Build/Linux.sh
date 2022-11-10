
# Premake5
echo .
echo "==== Building Premake5 ===="
cd -
chmod +x ./premake5.elf
./premake5.elf gmake2

# MakeFile
echo .
make

# Run Program
echo .
./_Bin/linux-x86_64/Debug/GLF3D/GLF3D
echo .
