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

    setupSlider(thresholdSlider, "Threshold");
    setupSlider(ratioSlider,     "Ratio");
    setupSlider(attackSlider,    "Attack");
    setupSlider(releaseSlider,   "Release");
    setupSlider(kneeSlider,      "Knee");
    setupSlider(outputSlider,    "Output");

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

    setSize (520, 260);
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
    area.removeFromTop(30);

    auto top = area.removeFromTop(160);
    auto w = top.getWidth() / 6;

    thresholdSlider.setBounds(top.removeFromLeft(w).reduced(5));
    ratioSlider.setBounds    (top.removeFromLeft(w).reduced(5));
    attackSlider.setBounds   (top.removeFromLeft(w).reduced(5));
    releaseSlider.setBounds  (top.removeFromLeft(w).reduced(5));
    kneeSlider.setBounds     (top.removeFromLeft(w).reduced(5));
    outputSlider.setBounds   (top.removeFromLeft(w).reduced(5));

    auto bottom = area.removeFromTop(40);
    sidechainButton.setBounds(bottom.removeFromLeft(170));
    softKneeButton.setBounds (bottom.removeFromLeft(120));
    limiterButton.setBounds  (bottom.removeFromLeft(100));
}
