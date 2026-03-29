/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MaybeDuckAudioProcessorEditor::MaybeDuckAudioProcessorEditor (MaybeDuckAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
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

    setupLabel(cpuLabel,       "CPU: --");
    setupLabel(blockLabel,     "Block: --");
    setupLabel(sampleRateLabel,"SR: --");
    setupLabel(grLabel,        "GR: -- dB");

    sidechainButton.setButtonText("Enable Sidechain");
    softKneeButton.setButtonText("Soft Knee");
    limiterButton.setButtonText("Limiter");

    addAndMakeVisible(sidechainButton);
    addAndMakeVisible(softKneeButton);
    addAndMakeVisible(limiterButton);

    thresholdAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttachment     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ratio", ratioSlider);
    attackAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attack", attackSlider);
    releaseAttachment   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "release", releaseSlider);
    kneeAttachment      = std::make_unique<SliderAttachment>(audioProcessor.apvts, "knee", kneeSlider);
    outputAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "output", outputSlider);

    sidechainAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "sidechainEnable", sidechainButton);
    softKneeAttachment  = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "softKnee", softKneeButton);
    limiterAttachment   = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "limiter", limiterButton);

    startTimerHz(20);

    setSize (760, 520);
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
    auto spectrumArea = area.removeFromTop(150);
    auto sliderArea = area.removeFromTop(170);
    auto buttonArea = area.removeFromTop(35);
    auto statsArea = area;

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

    auto statW = statsArea.getWidth() / 4;
    cpuLabel.setBounds(statsArea.removeFromLeft(statW));
    blockLabel.setBounds(statsArea.removeFromLeft(statW));
    sampleRateLabel.setBounds(statsArea.removeFromLeft(statW));
    grLabel.setBounds(statsArea.removeFromLeft(statW));
}

void MaybeDuckAudioProcessorEditor::timerCallback()
{
    cpuLabel.setText("CPU: " + juce::String(audioProcessor.getCpuUsagePercent(), 1) + " %",
                     juce::dontSendNotification);

    blockLabel.setText("Block: " + juce::String(audioProcessor.getCurrentBlockSize()),
                       juce::dontSendNotification);

    sampleRateLabel.setText("SR: " + juce::String(audioProcessor.getCurrentSampleRateHz(), 0) + " Hz",
                            juce::dontSendNotification);

    grLabel.setText("GR: " + juce::String(audioProcessor.getGainReductionDb(), 1) + " dB",
                    juce::dontSendNotification);
}