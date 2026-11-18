#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ResynthesisAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::FileDragAndDropTarget,
                                               private juce::Timer
{
public:
    explicit ResynthesisAudioProcessorEditor(ResynthesisAudioProcessor&);
    ~ResynthesisAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void timerCallback() override;
    void drawWaveform(juce::Graphics&, juce::Rectangle<float>);

    ResynthesisAudioProcessor& processor;
    juce::TextButton importButton { "LOAD WAVETABLE" };
    juce::ComboBox waveChoice;
    juce::Label title;
    juce::Label tableName;
    juce::Slider shift, transpose, smooth, phase, cepstrum, mix;
    juce::ToggleButton lock { "LOCK" }, infinite { "INF" };
    juce::Label status;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shiftAttachment, transposeAttachment,
        smoothAttachment, phaseAttachment, cepstrumAttachment, mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lockAttachment, infiniteAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResynthesisAudioProcessorEditor)
};
