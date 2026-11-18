#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour background { 0xff101114 };
    const juce::Colour panel { 0xff1a1d22 };
    const juce::Colour panelLight { 0xff242932 };
    const juce::Colour outline { 0xff343b45 };
    const juce::Colour text { 0xffedf1f5 };
    const juce::Colour muted { 0xff89939f };
    const juce::Colour accent { 0xffff792e };
    const juce::Colour accentSoft { 0xff8d4829 };
    const juce::Colour green { 0xff9be15d };
    constexpr float pi = juce::MathConstants<float>::pi;
    const juce::StringArray waveNames { "SINE", "SAW", "SQUARE", "TRIANGLE", "NOISE", "IMPORTED" };
    const juce::StringArray stereoNames { "STEREO", "MID", "SIDE", "MID + SIDE" };
    const juce::StringArray ids { "shift", "transpose", "phase", "cepstrum", "smooth", "mix" };
    const juce::StringArray labels { "SHIFT", "TRANSPOSE", "PHASE", "CEPSTRUM", "SMOOTH", "MIX" };
    const juce::StringArray units { "x", "st", "%", "%", "%", "%" };
}

ResynthesisAudioProcessorEditor::ResynthesisAudioProcessorEditor(ResynthesisAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 700);
    setResizable(true, true);
    setResizeLimits(820, 620, 1400, 980);
}

ResynthesisAudioProcessorEditor::~ResynthesisAudioProcessorEditor()
{
    if (activeControl.isNotEmpty())
        if (auto* parameter = processor.parameters.getParameter(activeControl))
            parameter->endChangeGesture();
}

juce::Rectangle<float> ResynthesisAudioProcessorEditor::padBounds() const
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    return { w * 0.245f, h * 0.285f, w * 0.51f, h * 0.345f };
}

juce::Rectangle<float> ResynthesisAudioProcessorEditor::controlBounds(int index) const
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const float left = w * 0.045f;
    const float usable = w * 0.91f;
    const float slot = usable / 6.0f;
    return { left + slot * index + slot * 0.12f, h * 0.705f, slot * 0.76f, h * 0.235f };
}

float ResynthesisAudioProcessorEditor::getParameter(const juce::String& id) const
{
    if (auto* value = processor.parameters.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}

void ResynthesisAudioProcessorEditor::setParameter(const juce::String& id, float value)
{
    if (auto* parameter = processor.parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void ResynthesisAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    g.fillAll(background);

    // Header
    g.setColour(panel);
    g.fillRoundedRectangle(22.0f, 18.0f, w - 44.0f, 52.0f, 12.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(22.0f, 18.0f, 5.0f, 52.0f, 2.0f);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(23.0f, juce::Font::bold)));
    g.drawText("RESYNTHESIS", 44, 22, 250, 42, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    g.drawText("WAVETABLE  /  SPECTRAL PROCESSOR", 300, 25, 300, 32, juce::Justification::centredLeft);

    auto button = [&g](juce::Rectangle<float> r, const juce::String& caption, bool active, bool primary = false)
    {
        g.setColour(primary ? accent : (active ? accentSoft : panelLight));
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(primary ? juce::Colours::white : (active ? accent : outline));
        g.drawRoundedRectangle(r, 8.0f, 1.0f);
        g.setColour(text);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(caption, r.toNearestInt(), juce::Justification::centred);
    };

    const auto waveButton = juce::Rectangle<float>(w - 340.0f, 27.0f, 135.0f, 34.0f);
    const auto loadButton = juce::Rectangle<float>(w - 193.0f, 27.0f, 150.0f, 34.0f);
    button(waveButton, waveNames[juce::jlimit(0, waveNames.size() - 1, waveType)], false);
    button(loadButton, "＋  LOAD WAVETABLE", false, true);

    // Waveform monitor
    g.setColour(panel);
    g.fillRoundedRectangle(22.0f, 86.0f, w - 44.0f, 91.0f, 12.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(22.0f, 86.0f, w - 44.0f, 91.0f, 12.0f, 1.0f);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("CURRENT WAVETABLE", 40, 93, 180, 18, juce::Justification::centredLeft);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(processor.getWavetableName(), 40, 145, 210, 20, juce::Justification::centredLeft);
    drawWaveform(g, { 260.0f, 101.0f, w - 290.0f, 62.0f });

    // XY processing field
    const auto pad = padBounds();
    g.setColour(panel);
    g.fillRoundedRectangle(pad, 16.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(pad, 16.0f, 1.0f);
    g.saveState();
    g.reduceClipRegion(pad.toNearestInt());
    g.setColour(outline.withAlpha(0.55f));
    for (int i = 1; i < 8; ++i)
    {
        const float x = pad.getX() + pad.getWidth() * i / 8.0f;
        const float y = pad.getY() + pad.getHeight() * i / 8.0f;
        g.drawVerticalLine(static_cast<int>(x), pad.getY(), pad.getBottom());
        g.drawHorizontalLine(static_cast<int>(y), pad.getX(), pad.getRight());
    }
    g.restoreState();

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("PHASE", pad.getX(), pad.getY() - 23.0f, pad.getWidth(), 18, juce::Justification::centred);
    g.drawText("CEPSTRAL SHAPE", pad.getX(), pad.getBottom() + 7.0f, pad.getWidth(), 18, juce::Justification::centred);

    const float px = pad.getX() + getParameter("xyX") * pad.getWidth();
    const float py = pad.getBottom() - getParameter("xyY") * pad.getHeight();
    g.setColour(accent.withAlpha(0.18f));
    g.fillEllipse(px - 29.0f, py - 29.0f, 58.0f, 58.0f);
    g.setColour(accent);
    g.drawEllipse(px - 15.0f, py - 15.0f, 30.0f, 30.0f, 2.0f);
    g.fillEllipse(px - 4.0f, py - 4.0f, 8.0f, 8.0f);

    // XY axis side labels
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("X  /  PHASE OFFSET", pad.getX(), pad.getBottom() + 27.0f, pad.getWidth(), 15, juce::Justification::centred);
    g.drawText("DRAG TO MORPH", pad.getX(), pad.getY() + 9.0f, pad.getWidth(), 15, juce::Justification::centred);

    // Control bank
    for (int i = 0; i < ids.size(); ++i)
    {
        const auto bounds = controlBounds(i);
        const float minimum[] { -1.0f, -24.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        const float maximum[] { 1.0f, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        drawKnob(g, bounds, labels[i], units[i], getParameter(ids[i]), minimum[i], maximum[i]);
    }

    // Footer switches
    const float footerY = h - 37.0f;
    button({ 24.0f, footerY, 94.0f, 25.0f }, getParameter("lock") > 0.5f ? "LOCKED" : "LOCK", getParameter("lock") > 0.5f);
    button({ 128.0f, footerY, 94.0f, 25.0f }, getParameter("infinite") > 0.5f ? "INF  ON" : "INF  OFF", getParameter("infinite") > 0.5f);
    button({ w - 184.0f, footerY, 160.0f, 25.0f }, stereoNames[juce::jlimit(0, stereoNames.size() - 1, stereoType)], false);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("DROP A WAV / AIFF ANYWHERE IN THE WINDOW TO IMPORT", 280, footerY, w - 480.0f, 25, juce::Justification::centred);
}

void ResynthesisAudioProcessorEditor::drawWaveform(juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto wave = processor.getWaveformSnapshot();
    juce::Path path;
    for (int i = 0; i < ResynthesisAudioProcessor::tableSize; ++i)
    {
        const float x = area.getX() + area.getWidth() * static_cast<float>(i) / static_cast<float>(ResynthesisAudioProcessor::tableSize - 1);
        const float y = area.getCentreY() - wave[static_cast<size_t>(i)] * area.getHeight() * 0.46f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(accent.withAlpha(0.12f));
    g.strokePath(path, juce::PathStrokeType(5.0f));
    g.setColour(accent);
    g.strokePath(path, juce::PathStrokeType(1.7f));
}

void ResynthesisAudioProcessorEditor::drawKnob(juce::Graphics& g, juce::Rectangle<float> area,
                                                 const juce::String& name, const juce::String& unit,
                                                 float value, float minimum, float maximum)
{
    g.setColour(panel);
    g.fillRoundedRectangle(area, 12.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(area, 12.0f, 1.0f);

    const float diameter = juce::jmin(area.getWidth() * 0.58f, area.getHeight() * 0.58f);
    const float cx = area.getCentreX();
    const float cy = area.getY() + area.getHeight() * 0.48f;
    const float radius = diameter * 0.5f;
    const float norm = juce::jlimit(0.0f, 1.0f, (value - minimum) / (maximum - minimum));
    const float start = pi * 1.25f;
    const float end = pi * 2.75f;
    const float angle = start + norm * (end - start);

    g.setColour(outline);
    g.drawEllipse(cx - radius, cy - radius, diameter, diameter, 5.0f);
    juce::Path arc;
    arc.addCentredArc(cx, cy, radius, radius, 0.0f, start, angle, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(text);
    g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
    g.setColour(accent);
    g.drawLine(cx, cy, cx + std::cos(angle) * radius * 0.72f, cy + std::sin(angle) * radius * 0.72f, 2.5f);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawText(name, area.getX() + 4.0f, area.getY() + 7.0f, area.getWidth() - 8.0f, 15.0f, juce::Justification::centred);
    juce::String valueText;
    if (name == "SHIFT") valueText = juce::String(value, 2) + "x";
    else if (name == "TRANSPOSE") valueText = juce::String(value, 1) + " st";
    else valueText = juce::String(juce::roundToInt(value * 100.0f)) + "%";
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText(valueText, area.getX() + 4.0f, area.getBottom() - 25.0f, area.getWidth() - 8.0f, 18.0f, juce::Justification::centred);
}

void ResynthesisAudioProcessorEditor::resized()
{
    repaint();
}

void ResynthesisAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const auto p = e.position;

    if (juce::Rectangle<float>(w - 340.0f, 27.0f, 135.0f, 34.0f).contains(p))
    {
        waveType = (waveType + 1) % 5;
        processor.selectBuiltinWave(waveType);
        repaint();
        return;
    }
    if (juce::Rectangle<float>(w - 193.0f, 27.0f, 150.0f, 34.0f).contains(p))
    {
        auto chooser = std::make_shared<juce::FileChooser>("Load a wavetable", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav;*.aif;*.aiff");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) importFile(fc.getResult()); });
        return;
    }
    if (padBounds().contains(p))
    {
        draggingPad = true;
        mouseDrag(e);
        return;
    }

    const float minimum[] { -1.0f, -24.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    const float maximum[] { 1.0f, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < ids.size(); ++i)
    {
        if (controlBounds(i).contains(p))
        {
            activeControl = ids[i];
            dragStartY = p.y;
            dragStartValue = getParameter(activeControl);
            if (auto* parameter = processor.parameters.getParameter(activeControl))
                parameter->beginChangeGesture();
            mouseDrag(e);
            return;
        }
    }

    const float footerY = h - 37.0f;
    if (juce::Rectangle<float>(24.0f, footerY, 94.0f, 25.0f).contains(p))
    {
        setParameter("lock", getParameter("lock") > 0.5f ? 0.0f : 1.0f);
    }
    else if (juce::Rectangle<float>(128.0f, footerY, 94.0f, 25.0f).contains(p))
    {
        setParameter("infinite", getParameter("infinite") > 0.5f ? 0.0f : 1.0f);
    }
    else if (juce::Rectangle<float>(w - 184.0f, footerY, 160.0f, 25.0f).contains(p))
    {
        stereoType = (stereoType + 1) % stereoNames.size();
        setParameter("stereo", static_cast<float>(stereoType));
    }
    repaint();
}

void ResynthesisAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingPad)
    {
        const auto pad = padBounds();
        const float x = juce::jlimit(0.0f, 1.0f, (e.position.x - pad.getX()) / pad.getWidth());
        const float y = juce::jlimit(0.0f, 1.0f, 1.0f - (e.position.y - pad.getY()) / pad.getHeight());
        setParameter("xyX", x);
        setParameter("xyY", y);
        repaint();
        return;
    }

    if (activeControl.isNotEmpty())
    {
        const int index = ids.indexOf(activeControl);
        const float minimum[] { -1.0f, -24.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        const float maximum[] { 1.0f, 24.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        const float sensitivity = index == 1 ? 0.45f : (index == 0 ? 0.018f : 0.0038f);
        const float next = juce::jlimit(minimum[index], maximum[index], dragStartValue + (dragStartY - e.position.y) * sensitivity);
        setParameter(activeControl, next);
        repaint();
    }
}

void ResynthesisAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    draggingPad = false;
    if (activeControl.isNotEmpty())
    {
        if (auto* parameter = processor.parameters.getParameter(activeControl))
            parameter->endChangeGesture();
        activeControl.clear();
    }
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
    importFile(juce::File(files[0]));
}

void ResynthesisAudioProcessorEditor::importFile(const juce::File& file)
{
    juce::String error;
    if (!processor.importWavetable(file, error))
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Wavetable import failed", error);
    else
    {
        waveType = 5;
        repaint();
    }
}
