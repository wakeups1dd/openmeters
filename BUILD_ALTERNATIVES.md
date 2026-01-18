# Building OpenMeters Without Full Visual Studio

## The Challenge

OpenMeters uses **WASAPI** (Windows Audio Session API), which requires Windows SDK headers:
- `mmdeviceapi.h`
- `audioclient.h`
- `windows.h` (usually included with MinGW)

These headers are typically included with:
- Visual Studio (full or Build Tools)
- Windows SDK (standalone installer)

## Option 1: Minimal Setup - Windows SDK Only ⭐ RECOMMENDED

**What you need**: Windows SDK (free, ~1GB download)

1. **Download Windows SDK**:
   - Visit: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
   - Download Windows 10 SDK or Windows 11 SDK
   - Install (select "Windows SDK for Desktop C++")

2. **Use the simple build script**:
   ```batch
   build-simple.bat
   ```

3. **Or compile manually**:
   ```batch
   g++ -std=c++17 -O2 -Wall -c core\meters\peak-meter.cpp -o peak-meter.o
   g++ -std=c++17 -O2 -Wall -c core\meters\rms-meter.cpp -o rms-meter.o
   g++ -std=c++17 -O2 -Wall -c core\audio\wasapi-capture.cpp -o wasapi-capture.o -lole32 -loleaut32 -lavrt
   g++ -std=c++17 -O2 -Wall -c core\audio\audio-engine.cpp -o audio-engine.o
   g++ -std=c++17 -O2 -Wall -c app\main.cpp -o main.o
   g++ -o openmeters.exe *.o -lole32 -loleaut32 -lavrt
   ```

**Pros**: Lightweight, only what you need  
**Cons**: Still ~1GB download, no IDE

---

## Option 2: Visual Studio Build Tools (No IDE)

**What you need**: Visual Studio Build Tools (free, ~3GB)

1. **Download Build Tools**:
   - Visit: https://visualstudio.microsoft.com/downloads/
   - Scroll to "Tools for Visual Studio" → "Build Tools for Visual Studio"
   - Download and install
   - Select "Desktop development with C++" workload

2. **Build with CMake**:
   ```batch
   "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles"
   cmake --build .
   ```

**Pros**: Includes CMake, full Windows SDK, professional toolchain  
**Cons**: Larger download (~3GB)

---

## Option 3: Online Build Services (No Installation)

### GitHub Actions (Free for Public Repos)

1. Push code to GitHub
2. Create `.github/workflows/build.yml`:
   ```yaml
   name: Build
   on: [push]
   jobs:
     build:
       runs-on: windows-latest
       steps:
         - uses: actions/checkout@v3
         - name: Configure CMake
           run: cmake -B build -G "Visual Studio 17 2022" -A x64
         - name: Build
           run: cmake --build build --config Release
         - name: Upload Artifact
           uses: actions/upload-artifact@v3
           with:
             name: openmeters.exe
             path: build/bin/Release/openmeters.exe
   ```
3. Download the built executable from Actions tab

**Pros**: No installation, automated builds  
**Cons**: Requires GitHub account, internet connection

---

## Option 4: Pre-built Binary (If Available)

If someone has already built it, you can just download and run:
- Check Releases section (if project is on GitHub)
- Ask in project discussions/forums

**Pros**: Instant, no setup  
**Cons**: May not be available, need to trust the builder

---

## Option 5: Docker (Advanced)

If you have Docker Desktop installed:

```dockerfile
# Dockerfile
FROM mcr.microsoft.com/windows/servercore:ltsc2022
RUN powershell -Command "Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_buildtools.exe' -OutFile vs_buildtools.exe"
# ... build steps
```

**Pros**: Isolated environment  
**Cons**: Requires Docker, Windows containers, complex setup

---

## Quick Test: Check if Windows SDK is Available

Run this to check if you already have Windows SDK headers:

```batch
dir "C:\Program Files (x86)\Windows Kits\10\Include" /s /b | findstr mmdeviceapi.h
```

If it finds the file, you're good to go! Just point your compiler to the include path.

---

## Recommendation

**For minimal setup**: Option 1 (Windows SDK only)  
**For best experience**: Option 2 (Visual Studio Build Tools)  
**For no installation**: Option 3 (GitHub Actions)

---

## Troubleshooting

### "mmdeviceapi.h: No such file or directory"
→ Windows SDK not installed or not in include path

### "undefined reference to `CoInitializeEx`"
→ Missing library links (`-lole32 -loleaut32`)

### "cannot find -lavrt"
→ Missing `avrt` library (part of Windows SDK)

### MinGW version too old
→ Your MinGW GCC 6.3.0 is from 2016. Consider upgrading to MinGW-w64 or using Visual Studio Build Tools.

