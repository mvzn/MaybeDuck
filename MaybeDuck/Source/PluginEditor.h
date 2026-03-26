/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class MaybeDuckAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MaybeDuckAudioProcessorEditor (MaybeDuckAudioProcessor&);
    ~MaybeDuckAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MaybeDuckAudioProcessor& audioProcessor;

    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider, kneeSlider, outputSlider;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, kneeLabel, outputLabel;

    juce::ToggleButton sidechainButton, softKneeButton, limiterButton;
    juce::Label cpuLabel, blockLabel, sampleRateLabel, grLabel;
    
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> thresholdAttachment, ratioAttachment, attackAttachment,
                                      releaseAttachment, kneeAttachment, outputAttachment;

    std::unique_ptr<ButtonAttachment> sidechainAttachment, softKneeAttachment, limiterAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaybeDuckAudioProcessorEditor)
};
