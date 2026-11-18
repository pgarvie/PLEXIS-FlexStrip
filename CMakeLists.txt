cmake_minimum_required(VERSION 3.22)

project(PlexisFlexStrip VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE: first look for a local copy in third_party/JUCE, otherwise fetch it
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    add_subdirectory(third_party/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(PlexisFlexStrip
    COMPANY_NAME "PLEXIS Labs"
    PLUGIN_MANUFACTURER_CODE Plxs
    PLUGIN_CODE Fxst
    FORMATS VST3 Standalone
    PRODUCT_NAME "PLEXIS FlexStrip"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx)

target_sources(PlexisFlexStrip PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(PlexisFlexStrip PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PlexisFlexStrip
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# DSP unit tests (no JUCE needed)
option(FLEXSTRIP_BUILD_TESTS "Build DSP unit tests" OFF)
if(FLEXSTRIP_BUILD_TESTS)
    add_executable(FlexStripDspTests Tests/DspTests.cpp)

    juce_add_console_app(FlexStripSnapshot PRODUCT_NAME "FlexStripSnapshot")
    target_sources(FlexStripSnapshot PRIVATE
        Tests/Snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(FlexStripSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(FlexStripSnapshot PRIVATE
        juce::juce_audio_utils juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()
