/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DynamicsCore.h"

//==============================================================================
/**
*/
class MaybeDuckAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MaybeDuckAudioProcessor();
    ~MaybeDuckAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    // monitoring
    float getCpuUsagePercent() const noexcept   { return cpuUsagePercent.load(); }
    float getProcessTimeMs() const noexcept     { return processTimeMs.load(); }
    float getGainReductionDb() const noexcept   { return gainReductionDb.load(); }
    double getCurrentSampleRateHz() const noexcept { return currentSampleRate.load(); }
    int getCurrentBlockSize() const noexcept    { return currentBlockSize.load(); }
    bool isSidechainConnected() const noexcept  { return sidechainConnected.load(); }

private:
    DynamicsProcessor leftProcessor;
    DynamicsProcessor rightProcessor;

    // monitoring
    std::atomic<float> cpuUsagePercent { 0.0f };
    std::atomic<float> processTimeMs   { 0.0f };
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<int> currentBlockSize { 0 };
    std::atomic<bool> sidechainConnected { false };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaybeDuckAudioProcessor)
};
