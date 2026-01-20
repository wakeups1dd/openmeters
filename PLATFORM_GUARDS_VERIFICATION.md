# Platform Guards Verification

## Overview

All WASAPI-related files and logic are properly guarded with platform checks in both CMake and C++ to ensure Windows-specific features only build on Windows.

## CMake Guards ✅

### 1. Early Platform Check
**Location**: `CMakeLists.txt` (line ~20)
```cmake
if(NOT WIN32)
    message(FATAL_ERROR "OpenMeters is Windows-only. This project requires Windows 10+ and WASAPI.")
endif()
```
**Purpose**: Fails fast if not on Windows, before any configuration

### 2. Audio Engine Library
**Location**: `CMakeLists.txt` (line ~55)
```cmake
if(WIN32)
    add_library(audio_engine STATIC
        core/audio/wasapi-capture.cpp
        core/audio/audio-engine.cpp
    )
    # ... Windows-specific linking and definitions
else()
    message(FATAL_ERROR "OpenMeters is Windows-only. This project requires Windows and WASAPI.")
endif()
```
**Purpose**: Only creates audio_engine library on Windows

### 3. Executable Target
**Location**: `CMakeLists.txt` (line ~79)
```cmake
if(WIN32)
    add_executable(openmeters app/main.cpp)
    target_link_libraries(openmeters PRIVATE audio_engine ...)
else()
    message(FATAL_ERROR "OpenMeters executable is Windows-only.")
endif()
```
**Purpose**: Only creates executable on Windows

### 4. Install Rules
**Location**: `CMakeLists.txt` (line ~90)
```cmake
if(WIN32)
    install(TARGETS openmeters RUNTIME DESTINATION bin)
endif()
```
**Purpose**: Only installs on Windows

## C++ Preprocessor Guards ✅

### 1. wasapi-capture.h
**Location**: `core/audio/wasapi-capture.h`
```cpp
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
// ... class definition
#else
#error "WASAPI capture is Windows-only. This file should not be compiled on non-Windows systems."
#endif
```
**Status**: ✅ Entire header wrapped

### 2. wasapi-capture.cpp
**Location**: `core/audio/wasapi-capture.cpp`
```cpp
#include "wasapi-capture.h"

#ifdef _WIN32
// ... entire implementation
#endif // _WIN32
```
**Status**: ✅ Entire implementation wrapped

### 3. audio-engine.h
**Location**: `core/audio/audio-engine.h`
```cpp
#ifdef _WIN32
#include "wasapi-capture.h"
#else
#error "Audio engine is Windows-only. This file should not be compiled on non-Windows systems."
#endif
```
**Status**: ✅ Conditional include of WASAPI header

### 4. audio-engine.cpp
**Location**: `core/audio/audio-engine.cpp`
```cpp
#include "audio-engine.h"

#ifdef _WIN32
// ... entire implementation
#else
#error "Audio engine implementation is Windows-only. This file should not be compiled on non-Windows systems."
#endif
```
**Status**: ✅ Entire implementation wrapped

### 5. main.cpp
**Location**: `app/main.cpp`
```cpp
#ifdef _WIN32
// ... entire application code
#else
#error "OpenMeters is Windows-only. This application requires Windows and WASAPI."
#endif
```
**Status**: ✅ Entire application wrapped

## Guard Strategy

### Defense in Depth
1. **CMake Level**: Prevents configuration on non-Windows
2. **Preprocessor Level**: Prevents compilation if CMake check is bypassed
3. **Error Messages**: Clear, helpful messages at each level

### Benefits
- ✅ **Early Failure**: CMake fails before any compilation
- ✅ **Compile-Time Safety**: `#error` directives prevent accidental compilation
- ✅ **Clear Messages**: Users know exactly why build fails
- ✅ **No Silent Failures**: Multiple layers ensure detection

## Verification Checklist

- [x] CMake checks `WIN32` before creating targets
- [x] CMake fails with clear error on non-Windows
- [x] All WASAPI headers wrapped in `#ifdef _WIN32`
- [x] All WASAPI implementations wrapped in `#ifdef _WIN32`
- [x] All includes of WASAPI headers are conditional
- [x] Application code wrapped in platform guards
- [x] Error messages are clear and helpful

## Testing

### On Windows:
- ✅ CMake configures successfully
- ✅ All targets created
- ✅ Compilation succeeds
- ✅ Application runs

### On Linux/macOS:
- ✅ CMake fails immediately with: `"OpenMeters is Windows-only..."`
- ✅ If CMake somehow proceeds, `#error` directives prevent compilation
- ✅ Clear error messages guide users

## Summary

All WASAPI-related code is properly guarded at multiple levels:
1. **CMake**: Platform check before any target creation
2. **Headers**: Conditional includes and `#error` directives
3. **Sources**: Entire implementations wrapped in `#ifdef _WIN32`
4. **Application**: Main entry point also guarded

The project is fully protected against cross-platform compilation attempts.



