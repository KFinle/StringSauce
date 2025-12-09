//
//  DynamicsProcessor.hpp
//  StringSauce
//
//  This class defines the Dynamics module.
//  It handles broadband compression, deessing,
//  and transient shaping behaviour.


#ifndef DynamicsProcessor_hpp
#define DynamicsProcessor_hpp
#pragma once

#include <JuceHeader.h>

class DynamicsProcessor : public juce::dsp::ProcessorBase
{
public:
    struct DynamicsParameters
    {
        // broadband compressor
        float compThreshold     = -18.0f;  // dB
        float compRatio         = 2.0f;    // 2:1
        float compAttack        = 10.0f;   // ms
        float compRelease       = 120.0f;  // ms
        float compMakeupGain    = 0.0f;    // dB

        // deesser
        float deesserFreq       = 5500.0f; //hz
        float deesserThreshold  = -20.0f;  //dB
        float deesserRatio      = 2.0f;    //ducking

        // transient designer
        float transientAttack   = 0.0f;    //[-1..+1] boost/cut attack
        float transientSustain  = 0.0f;    //[-1..+1] boost/cut sustain
    };

    DynamicsProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) override;
    void reset   () override;

    void setParameters(const DynamicsParameters& p);

private:
    juce::dsp::Compressor<float> comp;
    juce::dsp::IIR::Filter<float> deessHP;
    juce::dsp::IIR::Filter<float> deessLP;
    juce::dsp::IIR::Filter<float> deessShelf;
    float lastDeessGainLinear = 1.0f;
    float envFast = 0.0f, envSlow = 0.0f;
    float atkCoeffFast = 0.0f, relCoeffFast = 0.0f;
    float atkCoeffSlow = 0.0f, relCoeffSlow = 0.0f;
    float makeupLinear = 1.0f;
    DynamicsParameters params;
    double sampleRate = 44100.0;
    bool isPrepared = false;

    // helpers
    void updateCompressor();
    void updateDeesserFilters();
    void updateDeesserShelf(float linearCut);
    void updateTransientEnvelopes();
    void updateMakeup();

    inline float dbToLin(float dB) const noexcept { return juce::Decibels::decibelsToGain(dB); }
    inline float linToDb(float g)  const noexcept { return juce::Decibels::gainToDecibels(g, -150.0f); }
    
};

#endif
