#ifndef ModeProcessor_hpp
#define ModeProcessor_hpp
#pragma once
#include <JuceHeader.h>
#include "ToneEngine.hpp"
#include "EQProcessor.hpp"
#include "DynamicsProcessor.hpp"
#include "SaturationProcessor.hpp"
#include "SpatialProcessor.hpp"

class ModeProcessor
{
public:
    ModeProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void setMode(ToneEngine::Mode mode);

    void process(juce::dsp::ProcessContextReplacing<float>& context,
                 const ToneEngine::EngineParameters& params);

    void reset();

private:
    ToneMode currentMode = ToneMode::RHYTHM;

    struct ModeChain
    {
        EQProcessor eq;
        DynamicsProcessor dynamics;
        SaturationProcessor saturation;
        SpatialProcessor spatial;

        enum ProcessingOrder {
            RHYTHM_ORDER, // EQ -> Dynamics -> Saturation -> spatial
            LEAD_ORDER,   // Saturation -> EQ -> Dynamics -> Spatial
            CLEAN_ORDER   // EQ -> Saturation -> Spatial -> Dynamics
        } order = RHYTHM_ORDER;

        void prepare(const juce::dsp::ProcessSpec& spec);
        void setParameters(const ToneEngine::EngineParameters& params);
        void process(juce::dsp::ProcessContextReplacing<float>& context);
        void reset();
    };

    ModeChain rhythmChain, leadChain, cleanChain;

    ModeChain& getActiveChain();
    void setProcessingOrder(ModeChain& chain, ToneEngine::Mode mode);
};

#endif
