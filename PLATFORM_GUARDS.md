# Platform Guards Implementation

## Problem

The build was failing when attempting to compile Windows-specific code (WASAPI) on non-Windows platforms because:
- `wasapi-capture.h` included `<windows.h>` unconditionally
- CMake tried to compile `wasapi-capture.cpp` on all platforms
- GitHub Actions or other CI might accidentally try to build on Linux/macOS

## Solution Implemented

### 1. Preprocessor Guards in Headers

**`core/audio/wasapi-capture.h`**:
```cpp
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#else
#error "WASAPI capture is only supported on Windows platforms."
#endif
```

**`core/audio/audio-engine.h`**:
```cpp
#ifdef _WIN32
#include "wasapi-capture.h"
#else
#error "Audio engine currently requires Windows WASAPI."
#endif
```

### 2. Preprocessor Guards in Source Files

**`core/audio/wasapi-capture.cpp`**:
```cpp
#include "wasapi-capture.h"

#ifdef _WIN32
// ... implementation
#else
#error "WASAPI capture implementation is only supported on Windows platforms."
#endif
```

**`core/audio/audio-engine.cpp`**:
```cpp
#include "audio-engine.h"

#ifdef _WIN32
// ... implementation
#else
#error "Audio engine implementation is only supported on Windows platforms."
#endif
```

### 3. CMake Conditional Compilation

**`CMakeLists.txt`**:
```cmake
if(WIN32)
    set(AUDIO_ENGINE_SOURCES
        core/audio/wasapi-capture.cpp
        core/audio/audio-engine.cpp
    )
else()
    # On non-Windows platforms, only include platform-agnostic sources
    set(AUDIO_ENGINE_SOURCES
        core/audio/audio-engine.cpp
    )
    message(WARNING "Audio capture not implemented for this platform.")
endif()

add_library(audio_engine STATIC
    ${AUDIO_ENGINE_SOURCES}
)
```

## Benefits

1. **Prevents Build Failures**: Won't try to compile Windows code on Linux/macOS
2. **Clear Error Messages**: `#error` directives provide helpful messages
3. **Future-Ready**: Easy to add Linux/macOS implementations later
4. **CMake Safety**: CMake won't include Windows sources on other platforms

## Testing

### On Windows:
- ✅ Builds normally
- ✅ All Windows-specific code compiles
- ✅ No warnings or errors

### On Linux/macOS:
- ✅ CMake configures successfully (with warning)
- ✅ Compilation fails early with clear error message
- ✅ Prevents confusion about missing Windows headers

## Future Multi-Platform Support

When adding Linux/macOS support:

1. **Create platform-specific implementations**:
   - `core/audio/pulse-capture.h/cpp` (Linux)
   - `core/audio/coreaudio-capture.h/cpp` (macOS)

2. **Update CMakeLists.txt**:
   ```cmake
   if(WIN32)
       list(APPEND AUDIO_ENGINE_SOURCES core/audio/wasapi-capture.cpp)
   elseif(UNIX AND NOT APPLE)
       list(APPEND AUDIO_ENGINE_SOURCES core/audio/pulse-capture.cpp)
   elseif(APPLE)
       list(APPEND AUDIO_ENGINE_SOURCES core/audio/coreaudio-capture.cpp)
   endif()
   ```

3. **Remove `#error` directives** and replace with platform abstraction

## Files Modified

- ✅ `core/audio/wasapi-capture.h` - Added `#ifdef _WIN32` guard
- ✅ `core/audio/wasapi-capture.cpp` - Added `#ifdef _WIN32` guard
- ✅ `core/audio/audio-engine.h` - Added conditional include
- ✅ `core/audio/audio-engine.cpp` - Added `#ifdef _WIN32` guard
- ✅ `CMakeLists.txt` - Conditional source file inclusion

## Summary

The codebase is now properly guarded against cross-platform compilation issues. Windows-specific code will only compile on Windows, and clear error messages guide developers if they accidentally try to build on other platforms.



