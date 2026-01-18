# Using GitHub Actions to Build OpenMeters

## ✅ What GitHub Actions CAN Do

1. **Build the project** - Compiles your code on Windows runners
2. **Create executable** - Produces `openmeters.exe`
3. **Upload artifact** - Makes the `.exe` available for download
4. **Automated builds** - Builds on every push/PR

## ❌ What GitHub Actions CANNOT Do

1. **Run the application** - GitHub Actions runners don't have audio devices
2. **Test audio capture** - No WASAPI loopback available
3. **Verify functionality** - Can't test if audio metering works

## How to Use

### Step 1: Push to GitHub

```bash
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/YOUR_USERNAME/metrix.git
git push -u origin main
```

### Step 2: Trigger Build

The workflow runs automatically on:
- Push to `main` or `master` branch
- Pull requests
- Manual trigger (Actions tab → "Run workflow")

### Step 3: Download Executable

1. Go to your repository on GitHub
2. Click **Actions** tab
3. Click on the latest workflow run
4. Scroll down to **Artifacts**
5. Download `openmeters-windows-x64`
6. Extract `openmeters.exe`

### Step 4: Test Locally

Run the executable on your Windows machine:
```batch
openmeters.exe
```

## Workflow Files

Two workflows are provided:

1. **`.github/workflows/build.yml`** - Simple build and upload
2. **`.github/workflows/build-and-test.yml`** - More detailed (same functionality)

Both do the same thing - choose one and delete the other.

## What Gets Built

- **Platform**: Windows x64
- **Configuration**: Release (optimized)
- **Output**: `openmeters.exe`
- **Size**: ~100-500 KB (depends on static linking)

## Limitations

### No Runtime Testing
GitHub Actions can't test audio capture because:
- Runners are headless (no GUI)
- No audio hardware
- No audio devices available

### Solution
- Build on GitHub Actions ✅
- Download the `.exe` ✅
- Test locally on your machine ✅

## Future Enhancements

### Unit Tests (Can Run on GitHub Actions)
We could add unit tests for:
- Peak meter calculations
- RMS meter calculations
- Format conversion logic

These don't need audio devices and can run in CI.

### Example Test (Future):
```cpp
TEST_CASE("Peak meter") {
    PeakMeter meter;
    float buffer[] = {0.5f, -0.8f, 0.3f};
    auto result = meter.process(buffer, 3, format);
    REQUIRE(result.left == Approx(0.8f));
}
```

## Troubleshooting

### Build Fails
- Check Actions tab for error messages
- Ensure CMake syntax is correct
- Verify all source files are committed

### Executable Not Found
- Check build output for errors
- Verify CMake generated solution correctly
- Check that `build/bin/Release/` path exists

### Can't Download Artifact
- Artifacts expire after 90 days (default)
- Check if workflow run completed successfully
- Try re-running the workflow

## Alternative: Self-Hosted Runner

If you have a Windows machine always online, you could set up a self-hosted runner:
- Can have audio devices
- Can run actual tests
- More complex setup
- See: https://docs.github.com/en/actions/hosting-your-own-runners

## Summary

**GitHub Actions = Build Only**  
**Local Machine = Test & Run**

This is the standard approach for Windows audio applications!

