# Gabbermaster Clone - Build Instructions

A JUCE-based VST3/Standalone audio plugin clone of the classic Gabbermaster.

---

## Prerequisites

### Windows

1. **Visual Studio 2019 or 2022** (Community edition is free)
   - Download: https://visualstudio.microsoft.com/
   - During installation, select "Desktop development with C++"

2. **CMake 3.22 or higher**
   - Download: https://cmake.org/download/
   - Or install via winget: `winget install Kitware.CMake`
   - Make sure CMake is added to your PATH

### macOS

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **CMake 3.22 or higher**
   ```bash
   # Install via Homebrew
   brew install cmake
   ```

   If you don't have Homebrew:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

---

## Build Steps

### Windows

1. **Open Command Prompt or PowerShell** and navigate to the project:
   ```cmd
   cd C:\path\to\GC\GabbermasterClone
   ```

2. **Configure the build** (first time only):
   ```cmd
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the plugin**:
   ```cmd
   cmake --build build --config Release
   ```

4. **Find your built plugins**:
   - **VST3**: `GabbermasterClone\build\GabbermasterClone_artefacts\Release\VST3\Gabbermaster Clone.vst3\`
   - **Standalone**: `GabbermasterClone\build\GabbermasterClone_artefacts\Release\Standalone\Gabbermaster Clone.exe`

### macOS

1. **Open Terminal** and navigate to the project:
   ```bash
   cd /path/to/GC/GabbermasterClone
   ```

2. **Configure the build** (first time only):
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the plugin**:
   ```bash
   cmake --build build --config Release
   ```

4. **Find your built plugins**:
   - **VST3**: `GabbermasterClone/build/GabbermasterClone_artefacts/Release/VST3/Gabbermaster Clone.vst3`
   - **Standalone**: `GabbermasterClone/build/GabbermasterClone_artefacts/Release/Standalone/Gabbermaster Clone.app`
   - **AU** (if enabled): `GabbermasterClone/build/GabbermasterClone_artefacts/Release/AU/Gabbermaster Clone.component`

---

## Installation

### Windows

1. **VST3 Plugin**:
   Copy the entire `Gabbermaster Clone.vst3` folder to:
   ```
   C:\Program Files\Common Files\VST3\
   ```

2. **Standalone App**:
   Run `Gabbermaster Clone.exe` directly from the build folder, or copy it anywhere you like.

### macOS

1. **VST3 Plugin**:
   ```bash
   cp -R "build/GabbermasterClone_artefacts/Release/VST3/Gabbermaster Clone.vst3" ~/Library/Audio/Plug-Ins/VST3/
   ```

2. **AU Plugin** (if built):
   ```bash
   cp -R "build/GabbermasterClone_artefacts/Release/AU/Gabbermaster Clone.component" ~/Library/Audio/Plug-Ins/Components/
   ```

3. **Standalone App**:
   ```bash
   cp -R "build/GabbermasterClone_artefacts/Release/Standalone/Gabbermaster Clone.app" /Applications/
   ```

---

## Troubleshooting

### macOS: Plugin blocked by Gatekeeper

If macOS shows "cannot be opened because the developer cannot be verified":

**Option 1 - Remove quarantine attribute:**
```bash
xattr -cr ~/Library/Audio/Plug-Ins/VST3/Gabbermaster\ Clone.vst3
xattr -cr ~/Library/Audio/Plug-Ins/Components/Gabbermaster\ Clone.component
```

**Option 2 - Allow in System Preferences:**
1. Go to System Preferences → Security & Privacy → General
2. Click "Allow Anyway" next to the blocked plugin message

**Option 3 - Right-click to open:**
1. In Finder, right-click the plugin
2. Select "Open"
3. Click "Open" in the dialog

### DAW doesn't see the plugin

1. **Rescan plugins** in your DAW:
   - **Ableton Live**: Preferences → Plug-ins → Rescan
   - **FL Studio**: Options → Manage Plugins → Start Scan
   - **Logic Pro**: Restart Logic, it auto-scans
   - **Reaper**: Options → Preferences → Plug-ins → Re-scan

2. **Check plugin path**:
   - Make sure the plugin is in the correct folder
   - Verify your DAW is scanning that folder

3. **Architecture mismatch**:
   - Ensure your DAW and plugin are both 64-bit
   - On Apple Silicon Macs, some DAWs run in Rosetta mode

### Build errors

**"CMake not found"**:
- Ensure CMake is installed and in your PATH
- Restart your terminal after installing CMake

**"Visual Studio not found" (Windows)**:
- Install Visual Studio with "Desktop development with C++"
- Or specify the generator: `cmake -B build -G "Visual Studio 17 2022"`

**"Xcode not found" (macOS)**:
- Run: `xcode-select --install`
- Accept the Xcode license: `sudo xcodebuild -license accept`

---

## Clean Build

If you encounter issues, try a clean build:

### Windows
```cmd
rmdir /s /q build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### macOS
```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## Project Structure

```
GC/
├── GabbermasterClone/
│   ├── CMakeLists.txt      # Build configuration
│   └── build/              # Build output (created by cmake)
├── JUCE/                   # JUCE framework
└── Source/
    ├── PluginProcessor.cpp # Audio processing
    ├── PluginProcessor.h
    ├── PluginEditor.cpp    # UI
    ├── PluginEditor.h
    ├── SampleManager.cpp   # Sample loading
    ├── SampleManager.h
    ├── JuceHeader.h
    └── DSP/
        ├── Distortion.cpp
        ├── Distortion.h
        ├── Filter.cpp
        ├── Filter.h
        ├── Reverb.cpp
        └── Reverb.h
```

---

## Features

- **15 Rotary Knobs** with arc motion control
- **ADSR Envelope** with dynamic visualization
- **Filter Section**: High Pass, Low Pass, Band Pass modes
- **Distortion**: Pre/Post with multiple modes
- **Reverb**: Room, Width, Damping controls
- **SLOW/FAST**: Envelope time scaling (4x in SLOW mode)
- **OFF/ON**: Effect bypass toggle
- **48 Preset Samples** with custom sample loading
- **Pitch Envelope** for classic gabber pitch drops

---

## Support

For issues or questions, check the build output for specific error messages.
