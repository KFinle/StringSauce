//
//  ToneEngine.hpp
//  StringSauce
//
//  Defines the ToneEngine, which is responsible for
//  consolidating the processors

#ifndef ToneEngine_hpp
#define ToneEngine_hpp
#pragma once

#include <JuceHeader.h>
#include "EQProcessor.hpp"
#include "DynamicsProcessor.hpp"
#include "SaturationProcessor.hpp"
#include "SpatialProcessor.hpp"
#include "ParameterMapper.hpp"
#include "ToneMode.hpp"

class ToneEngine
{
public:
    using Mode = ToneMode;

    struct EngineParameters
    {
        EQProcessor::EQParameters eq;
        DynamicsProcessor::DynamicsParameters dynamics;
        SaturationProcessor::SaturationParameters saturation;
        SpatialProcessor::SpatialParameters spatial;

        struct EffectsParams
        {
            float shimmerPitch = 0.0f;
            float shimmerMix   = 0.0f;
        } effects;
        
        float outputAutoGain = 1.0f;
    };

    ToneEngine();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void updateParameters(float character, float thump, float body, float shimmer, float spank, float space, Mode mode);

    const EngineParameters& getCurrentParameters() const { return currentParams; }

private:
    void initSmoothing(double sampleRate);

    EngineParameters currentParams;
    Mode currentMode;
    std::array<juce::LinearSmoothedValue<float>, 16> smoothedParams;
};

#endif 
