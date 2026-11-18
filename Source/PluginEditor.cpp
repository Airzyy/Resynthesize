#include "PluginEditor.h"

namespace
{
    const juce::Colour bg(0xff171717), panel(0xff292929), orange(0xffff7900), green(0xff42e51d), red(0xffff1830);
}

ResynthesisAudioProcessorEditor::ResynthesisAudioProcessorEditor(ResynthesisAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(588, 690);
    title.setText("RESYNTHESIS", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(29.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    importButton.setColour(juce::TextButton::buttonColourId, orange);
    importButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    importButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import a wavetable", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav;*.aif;*.aiff");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    juce::String error;
                    if (!processor.importWavetable(file, error)) status.setText(error, juce::dontSendNotification);
                    else status.setText("Loaded: " + processor.getWavetableName(), juce::dontSendNotification);
                    repaint();
                }
            });
    };
    addAndMakeVisible(importButton);

    tableName.setText(processor.getWavetableName(), juce::dontSendNotification);
    tableName.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(tableName);

    auto setupSlider = [this](juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        s.setColour(juce::Slider::trackColourId, orange);
        s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        s.setName(name);
        addAndMakeVisible(s);
    };
    setupSlider(shift, "SHIFT"); setupSlider(transpose, "TRANSPOSE");
    setupSlider(smooth, "SMOOTH"); setupSlider(phase, "PHASE");
    setupSlider(cepstrum, "CEPSTRUM"); setupSlider(mix, "MIX");

    auto& params = processor.parameters;
    shiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "shift", shift);
    transposeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "transpose", transpose);
    smoothAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "smooth", smooth);
    phaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "phase", phase);
    cepstrumAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "cepstrum", cepstrum);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "mix", mix);
    lockAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(params, "lock", lock);
    infiniteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(params, "infinite", infinite);
    addAndMakeVisible(lock); addAndMakeVisible(infinite);
    addAndMakeVisible(status);
    status.setColour(juce::Label::textColourId, orange);
    startTimerHz(5);
}

void ResynthesisAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(14.0f, 16.0f, 520.0f, 40.0f, 3.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(29.0f, juce::Font::bold)));
    g.drawText("+", 18, 17, 42, 38, juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    g.drawText("RESYNTH", 105, 17, 330, 38, juce::Justification::centred);

    g.setColour(panel);
    g.fillRoundedRectangle(124.0f, 66.0f, 300.0f, 76.0f, 15.0f);
    drawWaveform(g, { 132.0f, 80.0f, 284.0f, 52.0f });

    g.setColour(panel);
    g.fillRoundedRectangle(124.0f, 151.0f, 300.0f, 296.0f, 18.0f);
    g.setColour(juce::Colour(0xff424242));
    g.drawLine(274, 211, 274, 388, 1.0f);
    g.drawLine(184, 300, 364, 300, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.55f));
    g.setFont(juce::Font(juce::FontOptions(15.0f)));
    g.drawText("PHASE", 220, 157, 110, 26, juce::Justification::centred);
    g.drawText("CEPSTRUM", 378, 250, 25, 100, juce::Justification::centred, true);

    g.setColour(panel.brighter(0.08f));
    g.fillRoundedRectangle(50.0f, 151.0f, 52.0f, 142.0f, 14.0f);
    g.fillRoundedRectangle(50.0f, 301.0f, 52.0f, 141.0f, 14.0f);
    g.fillRoundedRectangle(445.0f, 151.0f, 52.0f, 142.0f, 14.0f);
    g.fillRoundedRectangle(445.0f, 301.0f, 52.0f, 141.0f, 14.0f);

    g.setColour(red);
    g.fillEllipse(80, 503, 118, 118);
    g.fillEllipse(349, 503, 118, 118);
    g.setColour(juce::Colours::white);
    g.drawEllipse(80, 503, 118, 118, 7.0f);
    g.drawEllipse(349, 503, 118, 118, 7.0f);
    g.setColour(green);
    g.drawEllipse(216, 507, 116, 116, 5.0f);
    g.setColour(orange);
    g.fillEllipse(124, 418, 31, 31);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions(16.0f)));
    g.drawText("MID", 110, 625, 60, 25, juce::Justification::centred);
    g.drawText("SIDE", 380, 625, 60, 25, juce::Justification::centred);
    g.setColour(orange);
    g.fillRoundedRectangle(159.0f, 656.0f, 127.0f, 12.0f, 50.0f);
}

void ResynthesisAudioProcessorEditor::drawWaveform(juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto wave = processor.getWaveformSnapshot();
    juce::Path path;
    for (int i = 0; i < ResynthesisAudioProcessor::tableSize; ++i)
    {
        const float x = area.getX() + area.getWidth() * static_cast<float>(i) / ResynthesisAudioProcessor::tableSize;
        const float y = area.getCentreY() - wave[static_cast<size_t>(i)] * area.getHeight() * 0.46f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(orange);
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void ResynthesisAudioProcessorEditor::resized()
{
    title.setBounds(105, 16, 330, 40);
    importButton.setBounds(445, 77, 54, 52);
    tableName.setBounds(150, 65, 250, 18);
    shift.setBounds(49, 150, 54, 143);
    transpose.setBounds(49, 300, 54, 143);
    smooth.setBounds(444, 300, 54, 143);
    phase.setBounds(444, 150, 54, 143);
    cepstrum.setBounds(208, 460, 130, 160);
    mix.setBounds(260, 460, 70, 160);
    lock.setBounds(245, 470, 75, 25);
    infinite.setBounds(245, 585, 75, 25);
    status.setBounds(140, 670, 320, 18);
}

bool ResynthesisAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") || file.endsWithIgnoreCase(".aiff"))
            return true;
    return false;
}

void ResynthesisAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.isEmpty()) return;
    juce::String error;
    if (!processor.importWavetable(juce::File(files[0]), error))
        status.setText(error, juce::dontSendNotification);
    else
        status.setText("Loaded: " + processor.getWavetableName(), juce::dontSendNotification);
    repaint();
}

void ResynthesisAudioProcessorEditor::timerCallback()
{
    tableName.setText(processor.getWavetableName(), juce::dontSendNotification);
    repaint();
}
