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
     : MagicProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Link to gui
    FOLEYS_SET_SOURCE_PATH(__FILE__)
    magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>("input");

    // Parameter initialization

    // Mode
    addParameter(mode = new juce::AudioParameterChoice("mode", "Mode", {"Glorious", "June"}, 0));

    // Mix
    addParameter(mix = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.5f));

    // Rate
    juce::NormalisableRange<float> rateRange(0.03f, 2.0f);
    rateRange.skew = 0.5f;
    addParameter(rate = new juce::AudioParameterFloat("rate", "Rate", rateRange, 0.25f));

    // Depth
    addParameter(depth = new juce::AudioParameterFloat("depth", "Depth", 0.001f, 1.0f, 0.5f));

    // Mod (Feedback, Duplicate)
    addParameter(mod = new juce::AudioParameterFloat("mod", "Mod", 0.0, 1.0, 0.0f));
}

GloriousAudioProcessor::~GloriousAudioProcessor()
{
}

//==============================================================================
const juce::String GloriousAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GloriousAudioProcessor::acceptsMidi() const { return false; }

bool GloriousAudioProcessor::producesMidi() const { return false; }

bool GloriousAudioProcessor::isMidiEffect() const { return false; }

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

    oscilloscope->prepareToPlay(sampleRate, samplesPerBlock);

    glorious.prepare(spec, bufferSizeInSamples);
    glorious.setParams(GloriousParams(*rate, *depth, *mod, *mix));

    june.prepare(spec, bufferSizeInSamples);
    june.setParams(JuneParams(*rate, *depth, *mod, *mix));

    sRate.reset(400);
    sDepth.reset(400);
    sMix.reset(400);
    sMod.reset(400);
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
    sMod.setTargetValue(*mod);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        std::array<float, 2> chorusInput;
        std::array<float, 2> chorusOutput;

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* currentInputChannel = buffer.getReadPointer(channel);
            chorusInput[channel] = currentInputChannel[sample];
        }

        if (mode->getIndex() == 0)
        {
            glorious.setParams(GloriousParams(sRate.getNextValue(), sDepth.getNextValue(), sMod.getNextValue(), sMix.getNextValue()));
            chorusOutput = glorious.process(chorusInput);
        }
        else
        {
            june.setParams(JuneParams(sRate.getNextValue(), sDepth.getNextValue(), sMod.getNextValue(), sMix.getNextValue()));
            chorusOutput = june.process(chorusInput);
        }

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* currentOutputChannel = buffer.getWritePointer(channel);
            currentOutputChannel[sample] = chorusOutput[channel];
        }
    }

    oscilloscope->pushSamples(buffer);
}

int GloriousAudioProcessor::msToSamples(float ms)
{
    return (int)(ms * hostSampleRate / 1000);
}

//==============================================================================
//bool GloriousAudioProcessor::hasEditor() const
//{
//    return false; // (change this to false if you choose to not supply an editor)
//}

juce::AudioProcessorEditor* GloriousAudioProcessor::createEditor()
{
    auto builder = std::make_unique<foleys::MagicGUIBuilder>(magicState);

    builder->registerJUCEFactories();
    builder->registerJUCELookAndFeels();

    builder->registerLookAndFeel("AbbottLNF", std::make_unique<AbbottLNF>());

    return new foleys::MagicPluginEditor(magicState, std::move(builder));
}

//==============================================================================
//void GloriousAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
//{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
//}

//void GloriousAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
//{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
//}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GloriousAudioProcessor();
}
