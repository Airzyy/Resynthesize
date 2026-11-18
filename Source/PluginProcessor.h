#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class ResynthesisAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int tableSize = 2048;

    ResynthesisAudioProcessor();
    ~ResynthesisAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    bool importWavetable(const juce::File& file, juce::String& error);
    juce::String getWavetableName() const;
    std::array<float, tableSize> getWaveformSnapshot() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void makeBuiltinTable(int type);
    float readTable(float phase) const noexcept;

    std::array<float, tableSize> wavetable {};
    juce::String wavetableName { "Sine" };
    double currentSampleRate { 44100.0 };
    float previousInput[2] { 0.0f, 0.0f };
    float smoothed[2] { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResynthesisAudioProcessor)
};
