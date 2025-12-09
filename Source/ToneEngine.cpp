//
//  ToneEngine.cpp
//  StringSauce

#include "ToneEngine.hpp"

namespace
{
    // EQ
    float computeEqComp(const EQProcessor::EQParameters& eq)
    {
        const float wLow   = 0.25f;
        const float wMid1  = 0.35f;
        const float wMid2  = 0.25f;
        const float wHigh  = 0.10f;
        const float wAir   = 0.05f;
        const float wSum   = wLow + wMid1 + wMid2 + wHigh + wAir;

        float g =
            eq.lowShelfGain  * wLow  +
            eq.mid1Gain      * wMid1 +
            eq.mid2Gain      * wMid2 +
            eq.highShelfGain * wHigh +
            eq.airBandGain   * wAir;

        g /= (wSum > 0.0f ? wSum : 1.0f);
        return juce::jlimit(0.5f, 2.0f, g);
    }

    // Dynamics
    float computeDynComp(const DynamicsProcessor::DynamicsParameters& d)
    {
        float ratioFactor = 1.0f / juce::jlimit(1.0f, 4.0f, d.compRatio);
        float sustainFactor = 1.0f + d.transientSustain * 0.4f;
        float attackFactor  = 1.0f - d.transientAttack  * 0.2f;
        float g = ratioFactor * sustainFactor * attackFactor;
        return juce::jlimit(0.7f, 1.3f, g);
    }

    // Saturation
    float computeSatComp(const SaturationProcessor::SaturationParameters& s)
    {
        float d = juce::jlimit(0.0f, 1.0f, s.drive);
        float m = juce::jlimit(0.0f, 1.0f, s.mix);
        float g = 1.0f - d * 0.20f - m * 0.15f;
        return juce::jlimit(0.7f, 1.2f, g);
    }


    // Spacial
    float computeSpatialComp(const SpatialProcessor::SpatialParameters& sp)
    {
        float r = juce::jlimit(0.0f, 1.0f, sp.reverbMix);
        float d = juce::jlimit(0.0f, 1.0f, sp.delayMix);
        float c = juce::jlimit(0.0f, 1.0f, sp.chorusMix);
        float g = 1.0f
                - r * 0.35f
                - d * 0.25f
                - c * 0.15f;
        return juce::jlimit(0.6f, 1.1f, g);
    }
}


ToneEngine::ToneEngine()
{
    currentMode = ToneMode::RHYTHM;

    // initialization for each processor section
    currentParams = {};
    currentParams.eq =
    {
        .lowCutFreq         = 80.0f,
        .lowShelfFreq       = 100.0f,
        .lowShelfGain       = 1.0f,
        .mid1Freq           = 1000.0f,
        .mid1Gain           = 1.0f,
        .mid1Q              = 1.0f,
        .mid2Freq           = 2000.0f,
        .mid2Gain           = 1.0f,
        .mid2Q              = 1.0f,
        .highShelfFreq      = 8000.0f,
        .highShelfGain      = 1.0f,
        .airBandFreq        = 12000.0f,
        .airBandGain        = 1.0f
    };

    currentParams.dynamics =
    {
        .compThreshold      = 0.0f,
        .compRatio          = 1.0f,
        .compAttack         = 10.0f,
        .compRelease        = 120.0f,
        .compMakeupGain     = 0.0f,
        .deesserFreq        = 5000.0f,
        .deesserThreshold   = -20.0f,
        .deesserRatio       = 1.0f,
        .transientAttack    = 0.0f,
        .transientSustain   = 0.0f
    };

    currentParams.saturation =
    {
        .type               = SaturationProcessor::Type::Tape,
        .drive              = 0.5f,
        .mix                = 0.0f,
        .tone               = 0.5f,
        .bias               = 0.0f
    };

    currentParams.spatial =
    {
        .reverbSize         = 0.0f,
        .reverbDamping      = 0.5f,
        .reverbWidth        = 1.0f,
        .reverbMix          = 0.0f,
        .stereoWidth        = 1.0f,
        .delayTimeLeft      = 0.0f,
        .delayTimeRight     = 0.0f,
        .delayFeedback      = 0.0f,
        .delayMix           = 0.0f,
        .chorusRate         = 0.0f,
        .chorusDepth        = 0.0f,
        .chorusMix          = 0.0f
    };

    currentParams.effects =
    {
        .shimmerPitch       = 0.0f,
        .shimmerMix         = 0.0f
    };
}

void ToneEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    initSmoothing(spec.sampleRate);
}

void ToneEngine::initSmoothing(double sampleRate)
{
    constexpr float smoothingTime = 0.02f;
    for (auto& s : smoothedParams)
        s.reset(sampleRate, smoothingTime);
}

void ToneEngine::updateParameters(float character, float thump, float body,
                                  float shimmer, float spank, float space,
                                  Mode mode)
{
    currentMode = mode;

    auto mappedEQ                           = ParameterMapper::mapEQ(character, thump, body, shimmer, mode);
    auto mappedDynamics                     = ParameterMapper::mapDynamics(thump, body, shimmer, spank, mode);
    auto mappedSat                          = ParameterMapper::mapSaturation(character, body, shimmer, mode);
    auto mappedSpatial                      = ParameterMapper::mapSpatial(body, shimmer, space, mode);

    // EQ
    currentParams.eq.lowCutFreq             = mappedEQ.lowCutFreq;
    currentParams.eq.lowShelfFreq           = mappedEQ.lowShelfFreq;
    currentParams.eq.lowShelfGain           = mappedEQ.lowShelfGain;
    currentParams.eq.mid1Freq               = mappedEQ.mid1Freq;
    currentParams.eq.mid1Gain               = mappedEQ.mid1Gain;
    currentParams.eq.mid1Q                  = mappedEQ.mid1Q;
    currentParams.eq.mid2Freq               = mappedEQ.mid2Freq;
    currentParams.eq.mid2Gain               = mappedEQ.mid2Gain;
    currentParams.eq.mid2Q                  = mappedEQ.mid2Q;
    currentParams.eq.highShelfFreq          = mappedEQ.highShelfFreq;
    currentParams.eq.highShelfGain          = mappedEQ.highShelfGain;
    currentParams.eq.airBandFreq            = mappedEQ.airBandFreq;
    currentParams.eq.airBandGain            = mappedEQ.airBandGain;

    // Dynamics
    currentParams.dynamics.compThreshold    = mappedDynamics.compThreshold;
    currentParams.dynamics.compRatio        = mappedDynamics.compRatio;
    currentParams.dynamics.compAttack       = mappedDynamics.compAttack;
    currentParams.dynamics.compRelease      = mappedDynamics.compRelease;
    currentParams.dynamics.compMakeupGain   = mappedDynamics.compMakeupGain;
    currentParams.dynamics.deesserFreq      = mappedDynamics.deesserFreq;
    currentParams.dynamics.deesserThreshold = mappedDynamics.deesserThreshold;
    currentParams.dynamics.deesserRatio     = mappedDynamics.deesserRatio;
    currentParams.dynamics.transientAttack  = mappedDynamics.transientAttack;
    currentParams.dynamics.transientSustain = mappedDynamics.transientSustain;

    // Saturation
    currentParams.saturation.drive          = mappedSat.drive;
    currentParams.saturation.mix            = mappedSat.mix;
    currentParams.saturation.tone           = mappedSat.tone;
    currentParams.saturation.bias           = mappedSat.bias;

    // Spatial
    currentParams.spatial.reverbSize        = mappedSpatial.reverbSize;
    currentParams.spatial.reverbDamping     = mappedSpatial.reverbDamping;
    currentParams.spatial.reverbWidth       = mappedSpatial.reverbWidth;
    currentParams.spatial.reverbMix         = mappedSpatial.reverbMix;
    currentParams.spatial.stereoWidth       = mappedSpatial.stereoWidth;
    currentParams.spatial.delayTimeLeft     = mappedSpatial.delayTimeLeft;
    currentParams.spatial.delayTimeRight    = mappedSpatial.delayTimeRight;
    currentParams.spatial.delayFeedback     = mappedSpatial.delayFeedback;
    currentParams.spatial.delayMix          = mappedSpatial.delayMix;
    currentParams.spatial.chorusRate        = mappedSpatial.chorusRate;
    currentParams.spatial.chorusDepth       = mappedSpatial.chorusDepth;
    currentParams.spatial.chorusMix         = mappedSpatial.chorusMix;

    currentParams.effects.shimmerPitch      = shimmer * 12.0f;
    currentParams.effects.shimmerMix        = shimmer * 0.4f;
    
    
    const float eqComp   = computeEqComp(currentParams.eq);
    const float dynComp  = computeDynComp(currentParams.dynamics);
    const float satComp  = computeSatComp(currentParams.saturation);
    const float spatComp = computeSpatialComp(currentParams.spatial);

    float total = eqComp * dynComp * satComp * spatComp;
    if (! std::isfinite(total) || total <= 0.0f) total = 1.0f;
    currentParams.outputAutoGain = juce::jlimit(0.25f, 4.0f, 1.0f / total);
}
