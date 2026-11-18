cmake_minimum_required(VERSION 3.22)
project(RESYNTHESIS VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RESYNTHESIS
    COMPANY_NAME "Airzyy"
    BUNDLE_ID "works.airzy.resynthesis"
    PLUGIN_MANUFACTURER_CODE Airz
    PLUGIN_CODE Rsyn
    FORMATS VST3
    PRODUCT_NAME "RESYNTHESIS"
    DESCRIPTION "Wavetable resynthesis effect"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(RESYNTHESIS)

target_sources(RESYNTHESIS PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(RESYNTHESIS PRIVATE cxx_std_20)
target_compile_definitions(RESYNTHESIS PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RESYNTHESIS PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
