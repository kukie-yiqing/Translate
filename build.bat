@echo off
"D:\Program Files\Windhawk\Compiler\bin\clang++.exe" -O2 -std=c++17 -mwindows -municode -static -lwininet -lwinmm -luser32 -lgdi32 -lshell32 -lole32 main.cpp resource.res -o Translate.exe
if %ERRORLEVEL% equ 0 (
    echo Build Successful!
) else (
    echo Build Failed with error code %ERRORLEVEL%
)
