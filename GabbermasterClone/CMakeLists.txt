cmake_minimum_required(VERSION 3.22)

project(GabbermasterClone VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set JUCE path - UPDATE THIS TO YOUR JUCE LOCATION
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE framework")

# Add JUCE
add_subdirectory(${JUCE_DIR} JUCE)

# Plugin formats to build
set(PLUGIN_FORMATS AU VST3 Standalone)

# Create plugin target
juce_add_plugin(GabbermasterClone
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Gbmc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Gabbermaster Clone"
    
    # Audio
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    
    # Editor
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    
    # Plugin characteristics
    VST3_CATEGORIES "Instrument" "Synth" "Sampler"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    
    # Copy plugin after build
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Source files
target_sources(GabbermasterClone
    PRIVATE
        ../Source/PluginProcessor.cpp
        ../Source/PluginProcessor.h
        ../Source/PluginEditor.cpp
        ../Source/PluginEditor.h
        ../Source/SampleManager.cpp
        ../Source/SampleManager.h
        ../Source/DSP/Distortion.cpp
        ../Source/DSP/Distortion.h
        ../Source/DSP/Filter.cpp
        ../Source/DSP/Filter.h
        ../Source/DSP/Reverb.cpp
        ../Source/DSP/Reverb.h
        ../Source/DSP/EQ.h
        ../Source/DSP/LayerProcessor.h
)

# Include directories
target_include_directories(GabbermasterClone
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)

# Compile definitions
target_compile_definitions(GabbermasterClone
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Link libraries
target_link_libraries(GabbermasterClone
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# macOS specific settings
if(APPLE)
    set_target_properties(GabbermasterClone PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.13"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    )
endif()

# Installation paths
install(TARGETS GabbermasterClone
    LIBRARY DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins"
    RUNTIME DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins"
)
