#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    float clamp01(float x) { return juce::jlimit(0.0f, 1.0f, x); }
}

ResynthesisAudioProcessor::ResynthesisAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    makeBuiltinTable(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout ResynthesisAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<Float>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    p.add(std::make_unique<Float>("shift", "Shift", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    p.add(std::make_unique<Float>("transpose", "Transpose", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    p.add(std::make_unique<Float>("phase", "Phase", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    p.add(std::make_unique<Float>("cepstrum", "Cepstrum", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    p.add(std::make_unique<Float>("smooth", "Smooth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    p.add(std::make_unique<Float>("xyX", "XY Phase", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    p.add(std::make_unique<Float>("xyY", "XY Cepstrum", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    p.add(std::make_unique<Choice>("wave", "Wave", juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Noise", "Imported" }, 0));
    p.add(std::make_unique<Choice>("stereo", "Stereo", juce::StringArray { "Stereo", "Mid", "Side", "Mid + Side" }, 0));
    p.add(std::make_unique<juce::AudioParameterBool>("lock", "Lock", false));
    p.add(std::make_unique<juce::AudioParameterBool>("infinite", "INF", false));
    return p;
}

void ResynthesisAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    previousInput[0] = previousInput[1] = 0.0f;
    smoothed[0] = smoothed[1] = 0.0f;
}

void ResynthesisAudioProcessor::releaseResources() {}

bool ResynthesisAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && in == out;
}

void ResynthesisAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    const float mix = parameters.getRawParameterValue("mix")->load();
    const float shift = parameters.getRawParameterValue("shift")->load();
    const float transpose = parameters.getRawParameterValue("transpose")->load();
    const float phase = parameters.getRawParameterValue("phase")->load();
    const float cepstrum = parameters.getRawParameterValue("cepstrum")->load();
    const float smooth = parameters.getRawParameterValue("smooth")->load();
    const float xyX = parameters.getRawParameterValue("xyX")->load();
    const float xyY = parameters.getRawParameterValue("xyY")->load();
    const int stereoMode = static_cast<int>(parameters.getRawParameterValue("stereo")->load());
    const bool infinite = parameters.getRawParameterValue("infinite")->load() > 0.5f;
    const float phaseOffset = phase + (xyX - 0.5f) * 0.5f;
    const float drive = juce::jlimit(0.05f, 4.0f, std::pow(2.0f, transpose / 12.0f) * (1.0f + shift * 0.75f));
    const float smoothCoeff = smooth <= 0.0001f ? 1.0f : 1.0f - std::exp(-1.0f / (1.0f + smooth * 180.0f));

    for (int i = 0; i < samples; ++i)
    {
        float l = buffer.getSample(0, i);
        float r = channels > 1 ? buffer.getSample(1, i) : l;
        float mid = 0.5f * (l + r);
        float side = 0.5f * (l - r);

        auto processOne = [&](float input, int channel)
        {
            const float envelope = infinite ? 1.0f : juce::jlimit(0.0f, 1.0f, std::abs(input) * (1.0f + cepstrum * 3.0f + xyY * 2.0f));
            float p = 0.5f + input * drive * 0.45f + phaseOffset;
            p -= std::floor(p);
            float shaped = readTable(p) * envelope;
            smoothed[channel] += (shaped - smoothed[channel]) * smoothCoeff;
            previousInput[channel] = input;
            return input + (smoothed[channel] - input) * mix;
        };

        float outL = l, outR = r;
        if (stereoMode == 1)
        {
            const float processed = processOne(mid, 0);
            outL = processed; outR = processed;
        }
        else if (stereoMode == 2)
        {
            const float processed = processOne(side, 0);
            outL = mid + processed; outR = mid - processed;
        }
        else if (stereoMode == 3)
        {
            const float processedMid = processOne(mid, 0);
            const float processedSide = processOne(side, 1);
            outL = processedMid + processedSide; outR = processedMid - processedSide;
        }
        else
        {
            outL = processOne(l, 0);
            if (channels > 1) outR = processOne(r, 1);
        }
        buffer.setSample(0, i, outL);
        if (channels > 1) buffer.setSample(1, i, outR);
    }
}

void ResynthesisAudioProcessor::makeBuiltinTable(int type)
{
    juce::Random random(0x51A7);
    for (int i = 0; i < tableSize; ++i)
    {
        const float p = static_cast<float>(i) / static_cast<float>(tableSize);
        float v = 0.0f;
        switch (type)
        {
            case 1: v = 2.0f * p - 1.0f; break;
            case 2: v = p < 0.5f ? 1.0f : -1.0f; break;
            case 3: v = 1.0f - 4.0f * std::abs(p - 0.5f); break;
            case 4: v = random.nextFloat() * 2.0f - 1.0f; break;
            default: v = std::sin(twoPi * p); break;
        }
        wavetable[static_cast<size_t>(i)] = v;
    }
    const juce::StringArray names { "Sine", "Saw", "Square", "Triangle", "Noise" };
    wavetableName = names[juce::jlimit(0, 4, type)];
}

float ResynthesisAudioProcessor::readTable(float phase) const noexcept
{
    const float position = clamp01(phase) * static_cast<float>(tableSize);
    const int index = static_cast<int>(position) % tableSize;
    const int next = (index + 1) % tableSize;
    const float frac = position - std::floor(position);
    return wavetable[static_cast<size_t>(index)] * (1.0f - frac) + wavetable[static_cast<size_t>(next)] * frac;
}

bool ResynthesisAudioProcessor::importWavetable(const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (reader == nullptr) { error = "Could not read this audio file."; return false; }
    if (reader->lengthInSamples < 2 || reader->lengthInSamples > 10000000) { error = "Unsupported file length."; return false; }

    const int readCount = static_cast<int>(juce::jmin<int64>(reader->lengthInSamples, 10000000));
    juce::AudioBuffer<float> source(static_cast<int>(reader->numChannels), readCount);
    reader->read(&source, 0, readCount, 0, true, true);

    std::vector<float> mono(static_cast<size_t>(readCount), 0.0f);
    for (int i = 0; i < readCount; ++i)
    {
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
            mono[static_cast<size_t>(i)] += source.getSample(ch, i) / static_cast<float>(source.getNumChannels());
    }

    std::array<float, tableSize> imported {};
    for (int i = 0; i < tableSize; ++i)
    {
        const double position = static_cast<double>(i) * static_cast<double>(readCount) / tableSize;
        const int a = juce::jlimit(0, readCount - 1, static_cast<int>(position));
        const int b = juce::jmin(a + 1, readCount - 1);
        const float frac = static_cast<float>(position - a);
        imported[static_cast<size_t>(i)] = mono[static_cast<size_t>(a)] * (1.0f - frac) + mono[static_cast<size_t>(b)] * frac;
    }
    float peak = 0.0f;
    for (float v : imported) peak = juce::jmax(peak, std::abs(v));
    if (peak > 0.00001f) for (auto& v : imported) v /= peak;

    wavetable = imported;
    wavetableName = file.getFileNameWithoutExtension();
    parameters.getParameterAsValue("wave") = 5;
    error.clear();
    return true;
}

juce::String ResynthesisAudioProcessor::getWavetableName() const { return wavetableName; }
std::array<float, ResynthesisAudioProcessor::tableSize> ResynthesisAudioProcessor::getWaveformSnapshot() const { return wavetable; }

void ResynthesisAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    juce::MemoryOutputStream stream(dest, false);
    stream.writeInt(tableSize);
    for (float v : wavetable) stream.writeFloat(v);
    stream.writeString(wavetableName);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void ResynthesisAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* ResynthesisAudioProcessor::createEditor()
{
    return new ResynthesisAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResynthesisAudioProcessor();
}
