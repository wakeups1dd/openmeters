# Build Fix Summary

## Problem

Build was failing on Linux runners with:
```
fatal error: windows.h: No such file or directory
```

## Root Cause

A GitHub starter workflow template (`.github/workflows/cmake-single-platform.yml`) was configured to run on `ubuntu-latest`, attempting to build Windows-specific code.

## Solution Applied

### 1. Removed Conflicting Workflow ✅
- **Deleted**: `.github/workflows/cmake-single-platform.yml`
- **Kept**: `.github/workflows/build.yml` (properly configured for Windows)

### 2. Platform Guards Already in Place ✅

**Headers** (`core/audio/wasapi-capture.h`):
```cpp
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#else
#error "WASAPI capture is only supported on Windows platforms."
#endif
```

**Source Files** (`core/audio/wasapi-capture.cpp`, `core/audio/audio-engine.cpp`):
```cpp
#ifdef _WIN32
// ... implementation
#else
#error "Implementation only supported on Windows platforms."
#endif
```

**CMakeLists.txt**:
```cmake
if(WIN32)
    set(AUDIO_ENGINE_SOURCES
        core/audio/wasapi-capture.cpp
        core/audio/audio-engine.cpp
    )
else()
    message(FATAL_ERROR "Audio engine is currently Windows-only.")
endif()
```

## Current Workflow Status

✅ **`.github/workflows/build.yml`** - Windows-only build (correct)
- Runs on: `windows-latest`
- Uses: Visual Studio 2022
- Builds: Windows x64 executable

❌ **`.github/workflows/cmake-single-platform.yml`** - DELETED
- Was running on: `ubuntu-latest` (causing the error)
- No longer exists

## Verification

After these changes:
1. ✅ Only Windows workflow runs
2. ✅ CMake fails fast on non-Windows with clear error
3. ✅ Headers are properly guarded
4. ✅ No accidental Linux builds

## Next Steps

The build should now work correctly. If you see the error again:
1. Check that `cmake-single-platform.yml` is deleted
2. Verify `.github/workflows/build.yml` uses `runs-on: windows-latest`
3. Ensure no other workflows are running on Linux/macOS

## Files Modified

- ✅ Deleted: `.github/workflows/cmake-single-platform.yml`
- ✅ Already guarded: `core/audio/wasapi-capture.h`
- ✅ Already guarded: `core/audio/wasapi-capture.cpp`
- ✅ Already guarded: `core/audio/audio-engine.h`
- ✅ Already guarded: `core/audio/audio-engine.cpp`
- ✅ Already configured: `CMakeLists.txt`

