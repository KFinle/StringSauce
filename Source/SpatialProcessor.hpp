//
//  SpatialProcessor.hpp
//  StringSauce
//
//  Defines the Spacial Module

#ifndef SpatialProcessor_hpp
#define SpatialProcessor_hpp
#pragma once

#include <JuceHeader.h>

class SpatialProcessor : public juce::dsp::ProcessorBase
{
public:
    struct SpatialParameters
    {
        // reverb
        float reverbSize        = 0.5f;
        float reverbDamping     = 0.5f;  
        float reverbWidth       = 1.0f;   
        float reverbMix         = 0.0f;  

        // image
        float stereoWidth       = 1.0f;

        // delay
        float delayTimeLeft     = 250.0f;
        float delayTimeRight    = 375.0f;
        float delayFeedback     = 0.3f; 
        float delayMix          = 0.0f; 

        // chorus
        float chorusRate        = 0.5f;
        float chorusDepth       = 0.3f;    
        float chorusMix         = 0.0f;    
    };

    SpatialProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (const juce::dsp::ProcessContextReplacing<float>& ctx) override;
    void reset   () override;

    void setParameters(const SpatialParameters& p);

private:
    SpatialParameters params;

    // delay lines
    using DL = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>;
    DL delayL { 48000 }, delayR { 48000 }; 

    // chorus and reverb
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters rvParams;

    // mixers
    juce::dsp::DryWetMixer<float> delayMix;
    juce::dsp::DryWetMixer<float> chorusMix;
    juce::dsp::DryWetMixer<float> reverbMix;

    double sampleRate = 44100.0;
    size_t maxDelaySamples = 48000;

    // helpers
    void updateDelay();
    void updateChorus();
    void updateReverb();
    void applyStereoWidth(juce::dsp::AudioBlock<float>& block, float width);
};

#endif 
