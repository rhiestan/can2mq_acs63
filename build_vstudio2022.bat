@ECHO OFF

CD /d "%~dp0"

ECHO Set Visual Studio environment
CALL  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64

MKDIR build_vs
cd build_vs
cmake -Wno-dev -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release;RelWithDebInfo;Debug ../src

cd ..
pause
