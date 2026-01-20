cmake_minimum_required(VERSION 3.20)
project(OpenMeters VERSION 0.1.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific settings
# OpenMeters is Windows-only and requires WASAPI
if(NOT WIN32)
    message(FATAL_ERROR "OpenMeters is Windows-only. This project requires Windows 10+ and WASAPI.")
endif()

# Use Windows SDK
set(CMAKE_SYSTEM_VERSION 10.0)

# Link against Windows audio libraries (WASAPI dependencies)
set(WINDOWS_AUDIO_LIBS
    ole32      # COM interfaces
    oleaut32   # COM automation
    avrt       # Multimedia Class Scheduler Service (for real-time audio)
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
)

# Common library (header-only types)
add_library(common INTERFACE)
target_include_directories(common INTERFACE
    ${CMAKE_SOURCE_DIR}/common
)

# Meters library
add_library(meters STATIC
    core/meters/peak-meter.cpp
    core/meters/rms-meter.cpp
)
target_include_directories(meters PUBLIC
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(meters PUBLIC
    common
)

# Audio engine library (Windows-only)
if(WIN32)
    add_library(audio_engine STATIC
        core/audio/wasapi-capture.cpp
        core/audio/audio-engine.cpp
    )
    target_include_directories(audio_engine PUBLIC
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(audio_engine PUBLIC
        common
        meters
    )
    target_link_libraries(audio_engine PRIVATE
        ${WINDOWS_AUDIO_LIBS}
    )
    target_compile_definitions(audio_engine PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    message(FATAL_ERROR "OpenMeters is Windows-only. This project requires Windows and WASAPI.")
endif()

# Application executable (Windows-only)
if(WIN32)
    add_executable(openmeters
        app/main.cpp
    )
    target_link_libraries(openmeters PRIVATE
        audio_engine
        meters
        common
    )
else()
    message(FATAL_ERROR "OpenMeters executable is Windows-only. This project requires Windows and WASAPI.")
endif()

# Install rules (optional, Windows-only)
if(WIN32)
    install(TARGETS openmeters
        RUNTIME DESTINATION bin
    )
endif()

# Compiler-specific options
if(MSVC)
    # Disable some MSVC warnings
    target_compile_options(openmeters PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
    target_compile_options(audio_engine PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
    target_compile_options(meters PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
    
    # Link against Windows SDK
    if(WIN32)
        target_compile_definitions(audio_engine PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()
else()
    # GCC/Clang options
    target_compile_options(openmeters PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    target_compile_options(audio_engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    target_compile_options(meters PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "OpenMeters Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "")

