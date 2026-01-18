@echo off
REM Simple build script for MinGW (requires Windows SDK headers)
REM This script attempts to build without CMake

echo OpenMeters - Simple Build Script
echo ==================================
echo.

REM Check for g++
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found in PATH
    echo Please ensure MinGW is installed and in PATH
    exit /b 1
)

echo [1/4] Creating build directory...
if not exist build mkdir build
if not exist build\bin mkdir build\bin

echo [2/4] Compiling common library (header-only, no compilation needed)
echo        ✓ common/types.h
echo        ✓ common/audio-format.h
echo        ✓ common/meter-values.h

echo [3/4] Compiling meters library...
g++ -std=c++17 -O2 -Wall -c core\meters\peak-meter.cpp -o build\peak-meter.o
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile peak-meter.cpp
    exit /b 1
)

g++ -std=c++17 -O2 -Wall -c core\meters\rms-meter.cpp -o build\rms-meter.o
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile rms-meter.cpp
    exit /b 1
)

echo [4/4] Compiling audio engine...
REM Note: This requires Windows SDK headers (mmdeviceapi.h, audioclient.h)
REM If compilation fails here, you need Windows SDK installed

g++ -std=c++17 -O2 -Wall -c core\audio\wasapi-capture.cpp -o build\wasapi-capture.o -lole32 -loleaut32 -lavrt
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to compile wasapi-capture.cpp
    echo.
    echo This likely means Windows SDK headers are missing.
    echo Options:
    echo   1. Install Windows SDK (free, ~1GB)
    echo   2. Install Visual Studio Build Tools (includes Windows SDK)
    echo   3. Use Visual Studio Community (free, full IDE)
    echo.
    echo Download links:
    echo   Windows SDK: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
    echo   Visual Studio: https://visualstudio.microsoft.com/downloads/
    exit /b 1
)

g++ -std=c++17 -O2 -Wall -c core\audio\audio-engine.cpp -o build\audio-engine.o
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile audio-engine.cpp
    exit /b 1
)

echo [5/5] Compiling application...
g++ -std=c++17 -O2 -Wall -c app\main.cpp -o build\main.o
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile main.cpp
    exit /b 1
)

echo [6/6] Linking executable...
g++ -o build\bin\openmeters.exe build\*.o -lole32 -loleaut32 -lavrt -static-libgcc -static-libstdc++
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Linking failed
    exit /b 1
)

echo.
echo ==================================
echo Build successful!
echo Executable: build\bin\openmeters.exe
echo ==================================
echo.
echo To run: build\bin\openmeters.exe

