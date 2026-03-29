/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrumComponent::SpectrumComponent(MaybeDuckAudioProcessor& p)
    : processor(p)
{
    const int numBins = MaybeDuckAudioProcessor::fftSize / 2;
    inputMagnitudes.resize(numBins, -100.0f);
    sidechainMagnitudes.resize(numBins, -100.0f);
    outputMagnitudes.resize(numBins, -100.0f);

    inputPeaks.resize(numBins, -100.0f);
    sidechainPeaks.resize(numBins, -100.0f);
    outputPeaks.resize(numBins, -100.0f);
}

void SpectrumComponent::update()
{
    bool changed = false;

    if (processor.getInputFftData(scratchBuffer.data()))
    {
        processSpectrumFrame(scratchBuffer.data(), inputMagnitudes, inputPeaks);
        changed = true;
    }

    if (processor.getSidechainFftData(scratchBuffer.data()))
    {
        processSpectrumFrame(scratchBuffer.data(), sidechainMagnitudes, sidechainPeaks);
        changed = true;
    }

    if (processor.getOutputFftData(scratchBuffer.data()))
    {
        processSpectrumFrame(scratchBuffer.data(), outputMagnitudes, outputPeaks);
        changed = true;
    }

    if (changed)
        repaint();
}

void SpectrumComponent::processSpectrumFrame(const float* fftData,
                                             std::vector<float>& magnitudes,
                                             std::vector<float>& peaks)
{
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::copy(fftData, fftData + MaybeDuckAudioProcessor::fftSize, fftBuffer.begin());

    window.multiplyWithWindowingTable(fftBuffer.data(), MaybeDuckAudioProcessor::fftSize);
    fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

    const int numBins = MaybeDuckAudioProcessor::fftSize / 2;
    constexpr float minDb = -90.0f;
    constexpr float maxDb = 0.0f;

    for (int i = 1; i < numBins; ++i)
    {
        const float mag = fftBuffer[(size_t) i] / (float) MaybeDuckAudioProcessor::fftSize;
        const float db = juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(mag, minDb));

        magnitudes[(size_t) i] = db;
        peaks[(size_t) i] = juce::jmax(db, peaks[(size_t) i] - 0.5f);
    }
}

float SpectrumComponent::binToX(int bin, float width) const
{
    const auto sampleRate = juce::jmax(1.0, processor.getCurrentSampleRateHz());
    const float freq = (float) bin * (float) sampleRate / (float) MaybeDuckAudioProcessor::fftSize;

    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;

    const float clampedFreq = juce::jlimit(minFreq, maxFreq, freq);
    const float norm = std::log(clampedFreq / minFreq) / std::log(maxFreq / minFreq);
    return norm * width;
}

float SpectrumComponent::magnitudeToY(float db, float height) const
{
    return juce::jmap(db, 0.0f, -90.0f, 0.0f, height);
}

juce::Path SpectrumComponent::buildSpectrumPath(const std::vector<float>& values, juce::Rectangle<float> area) const
{
    juce::Path p;
    bool started = false;

    const int numBins = (int) values.size();
    for (int i = 1; i < numBins; ++i)
    {
        const float x = area.getX() + binToX(i, area.getWidth());
        const float y = area.getY() + magnitudeToY(values[(size_t) i], area.getHeight());

        if (!started)
        {
            p.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            p.lineTo(x, y);
        }
    }

    return p;
}

juce::Path SpectrumComponent::buildFillPath(const juce::Path& linePath, juce::Rectangle<float> area) const
{
    juce::Path fill(linePath);

    if (!linePath.isEmpty())
    {
        fill.lineTo(area.getRight(), area.getBottom());
        fill.lineTo(area.getX(), area.getBottom());
        fill.closeSubPath();
    }

    return fill;
}

void SpectrumComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colour(0xff121212));
    g.fillRoundedRectangle(bounds, 8.0f);

    auto area = bounds.reduced(10.0f);

    // grid
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    for (float hz : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float norm = std::log(hz / 20.0f) / std::log(20000.0f / 20.0f);
        const float x = area.getX() + norm * area.getWidth();
        g.drawVerticalLine((int) x, area.getY(), area.getBottom());
    }

    for (float db : { -72.0f, -54.0f, -36.0f, -18.0f })
    {
        const float y = area.getY() + magnitudeToY(db, area.getHeight());
        g.drawHorizontalLine((int) y, area.getX(), area.getRight());
    }

    auto drawTrace = [&](const std::vector<float>& mags,
                         const std::vector<float>& peaks,
                         juce::Colour fillColour,
                         juce::Colour peakColour)
    {
        auto magPath = buildSpectrumPath(mags, area);
        auto fillPath = buildFillPath(magPath, area);
        auto peakPath = buildSpectrumPath(peaks, area);

        g.setColour(fillColour.withAlpha(0.27f));
        g.fillPath(fillPath);

        g.setColour(peakColour.withAlpha(0.95f));
        g.strokePath(peakPath, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    drawTrace(inputMagnitudes,     inputPeaks,     juce::Colour(0xff3cba54), juce::Colour(0xff7ee28f));
    drawTrace(sidechainMagnitudes, sidechainPeaks, juce::Colour(0xff4285f4), juce::Colour(0xff8ab4ff));
    drawTrace(outputMagnitudes,    outputPeaks,    juce::Colour(0xfffbbc04), juce::Colour(0xffffd86a));

    // legend
    auto legend = area.removeFromTop(16.0f);
    g.setFont(12.0f);

    g.setColour(juce::Colour(0xff7ee28f));
    g.drawText("Input", legend.removeFromLeft(60), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff8ab4ff));
    g.drawText("Sidechain", legend.removeFromLeft(80), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffffd86a));
    g.drawText("Output", legend.removeFromLeft(60), juce::Justification::centredLeft);
}

//==============================================================================
MaybeDuckAudioProcessorEditor::MaybeDuckAudioProcessorEditor (MaybeDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), spectrumComponent(p)
{
    auto setupSlider = [this](juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        s.setName(name);
        addAndMakeVisible(s);
    };

    auto setupLabel = [this](juce::Label& l, const juce::String& text)
    {
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(l);
    };

    setupSlider(thresholdSlider, "Threshold");
    setupSlider(ratioSlider,     "Ratio");
    setupSlider(attackSlider,    "Attack");
    setupSlider(releaseSlider,   "Release");
    setupSlider(kneeSlider,      "Knee");
    setupSlider(outputSlider,    "Output");

    setupLabel(thresholdLabel, "Threshold");
    setupLabel(ratioLabel,     "Ratio");
    setupLabel(attackLabel,    "Attack");
    setupLabel(releaseLabel,   "Release");
    setupLabel(kneeLabel,      "Knee");
    setupLabel(outputLabel,    "Output");

    setupLabel(aboveThresholdLabel, "Above Thr: -- dB");
    setupLabel(grLabel,             "GR: -- dB");

    setupLabel(cpuLabel,       "CPU: --");
    setupLabel(blockLabel,     "Block: --");
    setupLabel(sampleRateLabel,"SR: --");

    sidechainButton.setButtonText("Enable Sidechain");
    softKneeButton.setButtonText("Soft Knee");
    limiterButton.setButtonText("Limiter");

    addAndMakeVisible(sidechainButton);
    addAndMakeVisible(softKneeButton);
    addAndMakeVisible(limiterButton);

    addAndMakeVisible(compressorMeter);
    addAndMakeVisible(spectrumComponent);

    thresholdAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttachment     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ratio", ratioSlider);
    attackAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attack", attackSlider);
    releaseAttachment   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "release", releaseSlider);
    kneeAttachment      = std::make_unique<SliderAttachment>(audioProcessor.apvts, "knee", kneeSlider);
    outputAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "output", outputSlider);

    sidechainAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "sidechainEnable", sidechainButton);
    softKneeAttachment  = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "softKnee", softKneeButton);
    limiterAttachment   = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "limiter", limiterButton);

    startTimerHz(30);

    setSize (820, 560);
}

MaybeDuckAudioProcessorEditor::~MaybeDuckAudioProcessorEditor()
{
}

//==============================================================================
void MaybeDuckAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaqu, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("Sidechain Compressor", getLocalBounds().removeFromTop(30),
                      juce::Justification::centred, 1);
}

void MaybeDuckAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto titleArea = area.removeFromTop(30);
    juce::ignoreUnused(titleArea);

    auto topArea = area.removeFromTop(210);
    auto meterArea = topArea.removeFromRight(80).reduced(8);
    auto spectrumArea = topArea.reduced(4);

    spectrumComponent.setBounds(spectrumArea);
    compressorMeter.setBounds(meterArea);

    auto sliderArea = area.removeFromTop(190);
    auto buttonArea = area.removeFromTop(36);
    auto statsArea = area.removeFromTop(28);

    auto columnWidth = sliderArea.getWidth() / 6;

    auto layoutSliderWithLabel = [&](juce::Slider& slider, juce::Label& label)
    {
        auto col = sliderArea.removeFromLeft(columnWidth).reduced(4);
        auto labelArea = col.removeFromTop(20);
        auto knobArea  = col;
        label.setBounds(labelArea);
        slider.setBounds(knobArea);
    };

    layoutSliderWithLabel(thresholdSlider, thresholdLabel);
    layoutSliderWithLabel(ratioSlider,     ratioLabel);
    layoutSliderWithLabel(attackSlider,    attackLabel);
    layoutSliderWithLabel(releaseSlider,   releaseLabel);
    layoutSliderWithLabel(kneeSlider,      kneeLabel);
    layoutSliderWithLabel(outputSlider,    outputLabel);

    sidechainButton.setBounds(buttonArea.removeFromLeft(170));
    softKneeButton.setBounds (buttonArea.removeFromLeft(120));
    limiterButton.setBounds  (buttonArea.removeFromLeft(100));

    auto statW = statsArea.getWidth() / 5;
    aboveThresholdLabel.setBounds(statsArea.removeFromLeft(statW));
    grLabel.setBounds(statsArea.removeFromLeft(statW));
    cpuLabel.setBounds(statsArea.removeFromLeft(statW));
    blockLabel.setBounds(statsArea.removeFromLeft(statW));
    sampleRateLabel.setBounds(statsArea.removeFromLeft(statW));
}

void MaybeDuckAudioProcessorEditor::timerCallback()
{
    aboveThresholdLabel.setText("Above Thr: " + juce::String(audioProcessor.getAboveThresholdDb(), 1) + " dB", juce::dontSendNotification);

    grLabel.setText("GR: " + juce::String(audioProcessor.getGainReductionAmountDb(), 1) + " dB", juce::dontSendNotification);
    cpuLabel.setText("CPU: " + juce::String(audioProcessor.getCpuUsagePercent(), 1) + " %", juce::dontSendNotification);
    blockLabel.setText("Block: " + juce::String(audioProcessor.getCurrentBlockSize()), juce::dontSendNotification);
    sampleRateLabel.setText("SR: " + juce::String(audioProcessor.getCurrentSampleRateHz(), 0) + " Hz", juce::dontSendNotification);

    compressorMeter.setLevels(audioProcessor.getInputLevelDb(), audioProcessor.getGainReductionAmountDb());

    spectrumComponent.update();
}