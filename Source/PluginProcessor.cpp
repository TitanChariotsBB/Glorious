/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GloriousAudioProcessor::GloriousAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Parameter initialization

    // Mix
    addParameter(mix = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.5f));

    // Rate
    juce::NormalisableRange<float> rateRange(0.05f, 3.0f);
    rateRange.skew = 0.5f;
    addParameter(rate = new juce::AudioParameterFloat("rate", "Rate", rateRange, 0.5f));

    // Depth
    addParameter(depth = new juce::AudioParameterFloat("depth", "Depth", 0.0f, 1.0f, 0.5f));

    // Feedback
    juce::NormalisableRange<float> fdbkRange(0.0f, 0.5f);
    fdbkRange.skew = 0.1f;
    addParameter(fdbk = new juce::AudioParameterFloat("fdbk", "Feedback", fdbkRange, 0.0f));
}

GloriousAudioProcessor::~GloriousAudioProcessor()
{
}

//==============================================================================
const juce::String GloriousAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GloriousAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GloriousAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GloriousAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GloriousAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GloriousAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GloriousAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GloriousAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GloriousAudioProcessor::getProgramName (int index)
{
    return {};
}

void GloriousAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GloriousAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    int bufferSizeInSamples = std::ceil(sampleRate * 0.1);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();

    chorusModule.prepare(spec, bufferSizeInSamples);
    chorusModule.setParams(GloriousParams(*rate, *depth, *fdbk, *mix));

    sRate.reset(400);
    sDepth.reset(400);
    sMix.reset(400);
    sFdbk.reset(400);
}

void GloriousAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GloriousAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GloriousAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    sMix.setTargetValue(*mix);
    sRate.setTargetValue(*rate);
    sDepth.setTargetValue(*depth);
    sFdbk.setTargetValue(*fdbk);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        chorusModule.setParams(GloriousParams(sRate.getNextValue(), sDepth.getNextValue(), sFdbk.getNextValue(), sMix.getNextValue()));

        std::array<float, 2> chorusInput;
        std::array<float, 2> chorusOutput;

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* currentInputChannel = buffer.getReadPointer(channel);
            chorusInput[channel] = currentInputChannel[sample];
        }

        chorusOutput = chorusModule.process(chorusInput);

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* currentOutputChannel = buffer.getWritePointer(channel);
            currentOutputChannel[sample] = chorusOutput[channel];
        }
    }
}

int GloriousAudioProcessor::msToSamples(float ms)
{
    return (int)(ms * hostSampleRate / 1000);
}

//==============================================================================
bool GloriousAudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GloriousAudioProcessor::createEditor()
{
    return new GloriousAudioProcessorEditor (*this);
}

//==============================================================================
void GloriousAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GloriousAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GloriousAudioProcessor();
}
