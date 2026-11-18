#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ResynthesisAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::FileDragAndDropTarget
{
public:
    explicit ResynthesisAudioProcessorEditor(ResynthesisAudioProcessor&);
    ~ResynthesisAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void drawWaveform(juce::Graphics&, juce::Rectangle<float>);
    void drawKnob(juce::Graphics&, juce::Rectangle<float>, const juce::String&,
                  const juce::String&, float, float, float);
    void setParameter(const juce::String&, float);
    float getParameter(const juce::String&) const;
    void importFile(const juce::File&);
    juce::Rectangle<float> padBounds() const;
    juce::Rectangle<float> controlBounds(int) const;

    ResynthesisAudioProcessor& processor;
    juce::String activeControl;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;
    bool draggingPad = false;
    int waveType = 0;
    int stereoType = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResynthesisAudioProcessorEditor)
};
