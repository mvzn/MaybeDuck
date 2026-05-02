/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MaybeDuckAudioProcessor::MaybeDuckAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

MaybeDuckAudioProcessor::~MaybeDuckAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MaybeDuckAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ratio", "Ratio", juce::NormalisableRange<float>(1.0f, 50.0f, 0.1f), 4.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", juce::NormalisableRange<float>(0.0f, 5000.0f, 0.1f, 0.5f), 10.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", juce::NormalisableRange<float>(20.0f, 5000.0f, 1.0f, 0.5f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "knee", "Knee", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "sidechainEnable", "Enable Sidechain", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "softKnee", "Soft Knee", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "limiter", "Limiter", false));

    return {params.begin(), params.end()};
}

//==============================================================================
const juce::String MaybeDuckAudioProcessor::getName() const
{
    return "MaybeDuck";
}

bool MaybeDuckAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MaybeDuckAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool MaybeDuckAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MaybeDuckAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MaybeDuckAudioProcessor::getNumPrograms()
{
    return 1;
}

int MaybeDuckAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MaybeDuckAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MaybeDuckAudioProcessor::getProgramName(int index)
{
    return {};
}

void MaybeDuckAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

//==============================================================================
void MaybeDuckAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    leftProcessor.reset(sampleRate);
    rightProcessor.reset(sampleRate);

    crossover.prepare(sampleRate);

    currentSampleRate.store(sampleRate);
    currentBlockSize.store(samplesPerBlock);
}

MaybeDuckAudioProcessor::BandControlValues
MaybeDuckAudioProcessor::applyBandLink(const BandControlValues& gr, float linkAmount)
{
    // Placeholder for future n-band link control.
    // Currently passes through unchanged; replace with neighbor-linking or
    // weighted-average linking when the speech detection modulation is wired up.
    juce::ignoreUnused(linkAmount);
    return gr;
}

void MaybeDuckAudioProcessor::pushNextSampleIntoFifo(float sample,
                                                     float *fifo,
                                                     int &index,
                                                     float *fftBuffer,
                                                     std::atomic<bool> &readyFlag)
{
    if (index == fftSize)
        return;

    fifo[index++] = sample;

    if (index == fftSize)
    {
        const juce::ScopedLock sl(fftDataLock);
        std::copy(fifo, fifo + fftSize, fftBuffer);
        readyFlag.store(true);
        index = 0;
    }
}

bool MaybeDuckAudioProcessor::getInputFftData(float *dest)
{
    if (!inputBlockReady.exchange(false))
        return false;

    const juce::ScopedLock sl(fftDataLock);
    std::copy(inputFftBuffer, inputFftBuffer + fftSize, dest);
    return true;
}

bool MaybeDuckAudioProcessor::getSidechainFftData(float *dest)
{
    if (!sidechainBlockReady.exchange(false))
        return false;

    const juce::ScopedLock sl(fftDataLock);
    std::copy(sidechainFftBuffer, sidechainFftBuffer + fftSize, dest);
    return true;
}

bool MaybeDuckAudioProcessor::getOutputFftData(float *dest)
{
    if (!outputBlockReady.exchange(false))
        return false;

    const juce::ScopedLock sl(fftDataLock);
    std::copy(outputFftBuffer, outputFftBuffer + fftSize, dest);
    return true;
}

void MaybeDuckAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MaybeDuckAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void MaybeDuckAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const auto startTime = juce::Time::getHighResolutionTicks();
    const auto ticksPerSecond = juce::Time::getHighResolutionTicksPerSecond();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    DynamicsProcessorParameters params = leftProcessor.getParameters();

    params.threshold_dB    = *apvts.getRawParameterValue("threshold");
    params.ratio           = *apvts.getRawParameterValue("ratio");
    params.attack_ms       = *apvts.getRawParameterValue("attack");
    params.release_ms      = *apvts.getRawParameterValue("release");
    params.knee_width_dB   = *apvts.getRawParameterValue("knee");
    params.output_gain_dB  = *apvts.getRawParameterValue("output");
    params.enable_sc       = (*apvts.getRawParameterValue("sidechainEnable") > 0.5f);
    params.soft_knee       = (*apvts.getRawParameterValue("softKnee") > 0.5f);
    params.hard_limit_gate = (*apvts.getRawParameterValue("limiter") > 0.5f);
    params.calculation     = dynamicsProcessorType::kCompressor;

    leftProcessor.setParameters(params);
    rightProcessor.setParameters(params);

    juce::AudioBuffer<float> mainInput = getBusBuffer(buffer, true, 0);
    juce::AudioBuffer<float> scInput   = getBusBuffer(buffer, true, 1);

    auto *left  = mainInput.getWritePointer(0);
    auto *right = mainInput.getNumChannels() > 1 ? mainInput.getWritePointer(1) : nullptr;

    const bool hasSidechainBus = getBusCount(true) > 1;
    const bool scConnected     = hasSidechainBus && scInput.getNumChannels() > 0;

    float blockPeakIn    = 0.0f;
    float sidechainPeakIn = 0.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float scL = 0.0f;
        float scR = 0.0f;
        float inL = mainInput.getReadPointer(0)[sample];
        float inR = mainInput.getNumChannels() > 1 ? mainInput.getReadPointer(1)[sample] : inL;
        float inputMono = 0.5f * (inL + inR);
        blockPeakIn = std::max(blockPeakIn, std::abs(inputMono));

        if (params.enable_sc && scConnected)
        {
            scL = scInput.getReadPointer(0)[sample];
            scR = scInput.getNumChannels() > 1 ? scInput.getReadPointer(1)[sample] : scL;
            float scMono = 0.5f * (scL + scR);
            sidechainPeakIn = std::max(sidechainPeakIn, std::abs(scMono));
            leftProcessor.processSidechainInputSample(scL);
            rightProcessor.processSidechainInputSample(scR);
            pushNextSampleIntoFifo(scMono, sidechainFifo, sidechainFifoIndex, sidechainFftBuffer, sidechainBlockReady);
        }

        pushNextSampleIntoFifo(inputMono, inputFifo, inputFifoIndex, inputFftBuffer, inputBlockReady);

        left[sample] = (float)leftProcessor.processSample(left[sample]);

        if (right != nullptr)
            right[sample] = (float)rightProcessor.processSample(right[sample]);

        float outMono = right != nullptr ? 0.5f * (left[sample] + right[sample]) : left[sample];
        pushNextSampleIntoFifo(outMono, outputFifo, outputFifoIndex, outputFftBuffer, outputBlockReady);
    }

    auto lParams = leftProcessor.getParameters();
    auto rParams = rightProcessor.getParameters();
    const float avgGR = 0.5f * (float)(lParams.gain_reduction_dB + rParams.gain_reduction_dB);

    const float grAmountDb = std::max(0.0f, -avgGR);

    const float blockPeakDb = (scConnected && params.enable_sc ? sidechainPeakIn : blockPeakIn > 0.000001f)
                                  ? juce::Decibels::gainToDecibels(scConnected && params.enable_sc ? sidechainPeakIn : blockPeakIn)
                                  : -100.0f;

    const float thresholdDb    = params.threshold_dB;
    const float aboveThreshold = std::max(0.0f, blockPeakDb - thresholdDb);

    inputLevelSmoothedDb     = juce::jmax(blockPeakDb,   inputLevelSmoothedDb + 0.8f);
    gainReductionSmoothedDb  = juce::jmax(grAmountDb,    gainReductionSmoothedDb - 0.35f);

    gainReductionDb.store(avgGR);
    gainReductionAmountDb.store(gainReductionSmoothedDb);
    inputLevelDb.store(inputLevelSmoothedDb);

    const auto endTime = juce::Time::getHighResolutionTicks();
    const double elapsedMs = 1000.0 * (double)(endTime - startTime) / (double)ticksPerSecond;
    processTimeMs.store((float)elapsedMs);

    const double blockDurationMs = 1000.0 * (double)buffer.getNumSamples() / std::max(1.0, getSampleRate());

    cpuUsagePercent.store(blockDurationMs > 0.0
        ? (float)(100.0 * elapsedMs / blockDurationMs)
        : 0.0f);

    aboveThresholdDb.store(aboveThreshold);
    currentBlockSize.store(buffer.getNumSamples());
    sidechainConnected.store(scConnected);
}

//==============================================================================
bool MaybeDuckAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *MaybeDuckAudioProcessor::createEditor()
{
    return new MaybeDuckAudioProcessorEditor(*this);
}

//==============================================================================
void MaybeDuckAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MaybeDuckAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MaybeDuckAudioProcessor();
}
