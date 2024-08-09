/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ChorusAlgos.h"

//==============================================================================
/**
*/
class GloriousAudioProcessor : public foleys::MagicProcessor
{
public:
    //==============================================================================
    GloriousAudioProcessor();
    ~GloriousAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    int msToSamples(float ms);

    //==============================================================================
    //juce::AudioProcessorEditor* createEditor() override;
    //bool hasEditor() const override;

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
    //void getStateInformation (juce::MemoryBlock& destData) override;
    //void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioParameterFloat *mix, *rate, *depth, *fdbk;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sMix, sRate, sDepth, sFdbk;

    Glorious chorusModule;

    int hostSampleRate{ 44100 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GloriousAudioProcessor)
};
