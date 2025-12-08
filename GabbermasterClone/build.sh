#!/bin/bash

# Gabbermaster Clone - Build Script for macOS
# This script automates the entire build process

set -e  # Exit on error

echo "======================================"
echo "  Gabbermaster Clone Build Script"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if JUCE is installed
if [ ! -d "../JUCE" ]; then
    echo -e "${RED}Error: JUCE not found!${NC}"
    echo "Please clone JUCE to the parent directory:"
    echo "  cd .."
    echo "  git clone https://github.com/juce-framework/JUCE.git"
    exit 1
fi

# Check for Xcode command line tools
if ! xcode-select -p &> /dev/null; then
    echo -e "${RED}Error: Xcode command line tools not installed!${NC}"
    echo "Please run: xcode-select --install"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake not installed!${NC}"
    echo "Please install CMake:"
    echo "  brew install cmake"
    exit 1
fi

echo -e "${GREEN}✓ All dependencies found${NC}"
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure CMake
echo ""
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
      ..

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ CMake configured successfully${NC}"

# Build the plugin
echo ""
echo "Building plugin (this may take a few minutes)..."
cmake --build . --config Release -j$(sysctl -n hw.ncpu)

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build completed successfully${NC}"

# Install plugins
echo ""
echo "Installing plugins..."

VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3"
AU_PATH="$HOME/Library/Audio/Plug-Ins/Components"

# Create directories if they don't exist
mkdir -p "$VST3_PATH"
mkdir -p "$AU_PATH"

# Copy VST3
if [ -d "GabbermasterClone_artefacts/Release/VST3/GabbermasterClone.vst3" ]; then
    echo "Installing VST3..."
    cp -R "GabbermasterClone_artefacts/Release/VST3/GabbermasterClone.vst3" "$VST3_PATH/"
    echo -e "${GREEN}✓ VST3 installed to $VST3_PATH${NC}"
else
    echo -e "${YELLOW}⚠ VST3 not found${NC}"
fi

# Copy AU
if [ -d "GabbermasterClone_artefacts/Release/AU/GabbermasterClone.component" ]; then
    echo "Installing Audio Unit..."
    cp -R "GabbermasterClone_artefacts/Release/AU/GabbermasterClone.component" "$AU_PATH/"
    echo -e "${GREEN}✓ AU installed to $AU_PATH${NC}"
    
    # Validate Audio Unit
    echo ""
    echo "Validating Audio Unit..."
    auval -v aumu Gbmc Manu
else
    echo -e "${YELLOW}⚠ AU not found${NC}"
fi

# Copy Standalone
if [ -d "GabbermasterClone_artefacts/Release/Standalone/GabbermasterClone.app" ]; then
    echo "Standalone app available at:"
    echo "  $(pwd)/GabbermasterClone_artefacts/Release/Standalone/GabbermasterClone.app"
fi

echo ""
echo "======================================"
echo -e "${GREEN}  Build Complete!${NC}"
echo "======================================"
echo ""
echo "Plugin installed to:"
echo "  VST3: $VST3_PATH/GabbermasterClone.vst3"
echo "  AU: $AU_PATH/GabbermasterClone.component"
echo ""
echo "Next steps:"
echo "  1. Open Ableton Live 12"
echo "  2. Rescan plugins (Preferences > Plug-Ins > Rescan)"
echo "  3. Find 'Gabbermaster Clone' in your instruments"
echo ""
echo "Happy hardcore production! 🔊"