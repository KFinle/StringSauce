#pragma once

#include <JuceHeader.h>
#include "ToneEngine.hpp"
#include "ModeProcessor.hpp"
#include "SpatialProcessor.hpp"
#include "PresetManager.hpp"

class StringSauceAudioProcessor : public juce::AudioProcessor
{
public:
    StringSauceAudioProcessor();
    ~StringSauceAudioProcessor() override = default;

    // JUCE Overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources () override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor () override;
    bool hasEditor () const override;

    const juce::String getName () const override;
    bool acceptsMidi () const override;
    bool producesMidi () const override;
    bool isMidiEffect () const override;
    double getTailLengthSeconds () const override;

    int getNumPrograms () override          { return 1; }
    int getCurrentProgram () override       { return 0; }
    void setCurrentProgram (int) override   {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout() };

    ToneEngine toneEngine;
    const ToneEngine::EngineParameters& getEngineParams () const
    {
        return toneEngine.getCurrentParameters();
    }

    // presets
    std::unique_ptr<PresetManager> presetManager;
    void registerFactoryPresets();

private:
    // DSP Components
    ModeProcessor modeProcessor;
    juce::dsp::Gain<float> inputGain, outputGain;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StringSauceAudioProcessor)
};
