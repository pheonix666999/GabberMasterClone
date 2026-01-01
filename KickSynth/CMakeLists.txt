cmake_minimum_required(VERSION 3.22)

project(KickSynth VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set JUCE path
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE framework")

# Add JUCE
add_subdirectory(${JUCE_DIR} JUCE)

# Plugin formats to build
set(PLUGIN_FORMATS VST3 Standalone)

# Create plugin target
juce_add_plugin(KickSynth
    COMPANY_NAME "KickSynth"
    PLUGIN_MANUFACTURER_CODE KSyn
    PLUGIN_CODE KSyn
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Kick Synth"
    
    # Audio
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    
    # Editor
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    
    # Plugin characteristics
    VST3_CATEGORIES "Instrument" "Synth"
    
    # Copy plugin after build
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Plugin sources
target_sources(KickSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Include directories
target_include_directories(KickSynth
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# Kick DSP library (shared between plugin and tools)
add_library(KickDSPLib STATIC
    Source/DSP/KickVoice.cpp
    Source/DSP/KickDistortion.cpp
    Source/DSP/KickLimiter.cpp
)

target_include_directories(KickDSPLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(KickDSPLib
    PUBLIC
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
)

# Shared CLI helper library
add_library(KickToolsLib STATIC
    Tools/KickMetrics.cpp
    Tools/KickParams.cpp
    Tools/KickRenderEngine.cpp
)

target_include_directories(KickToolsLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(KickToolsLib
    PUBLIC
        KickDSPLib
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
)

# Compile definitions
target_compile_definitions(KickSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
)

# Link libraries
target_link_libraries(KickSynth
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
        KickDSPLib
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)


# CLI tools
foreach(tool kick_analyze kick_render kick_fit)
    add_executable(${tool} Tools/${tool}.cpp)
    target_link_libraries(${tool}
        PRIVATE
            KickToolsLib
            juce::juce_gui_basics
            juce::juce_audio_formats
            juce::juce_audio_basics
            juce::juce_core
            juce::juce_data_structures
            juce::juce_events
            juce::juce_dsp
    )
endforeach()

# macOS specific settings
if(APPLE)
    set_target_properties(KickSynth PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.13"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    )
endif()



