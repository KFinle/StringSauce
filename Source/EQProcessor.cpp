//
//  EQProcessor.cpp
//  StringSauce
//
//  Implementation of the EQ module


#include "EQProcessor.hpp"

EQProcessor::EQProcessor() {}

void EQProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSpec = spec;
    eqChain.prepare(spec);
    reset();
}

void EQProcessor::setParameters(const EQParameters& params)
{
    currentParams = params;
    updateFilterCoefficients();
}

void EQProcessor::process(juce::dsp::ProcessContextReplacing<float>& context)
{
    const auto& p = currentParams;

    // bypass if all filters are essentially neutral
    const bool isFlat =
        std::abs(p.lowShelfGain - 1.0f) < 0.001f &&
        std::abs(p.mid1Gain     - 1.0f) < 0.001f &&
        std::abs(p.mid2Gain     - 1.0f) < 0.001f &&
        std::abs(p.highShelfGain- 1.0f) < 0.001f &&
        std::abs(p.airBandGain  - 1.0f) < 0.001f;

    if (isFlat) return;

    // otherwise, process
    eqChain.process(context);
}


void EQProcessor::reset()
{
    eqChain.reset();
    updateFilterCoefficients();
}

void EQProcessor::updateFilterCoefficients()
{
    const double sr = currentSpec.sampleRate;

    // safety clamps
    auto safeGain = [](float g)
    {
        if (std::isnan(g) || std::isinf(g) || g <= 0.0f) return 1.0f;
        return juce::jlimit(0.05f, 8.0f, g);
    };
    auto safeFreq = [sr](float f)
    {
        return juce::jlimit(20.0f, (float)(sr * 0.45), f);
    };
    auto safeQ = [](float q)
    {
        return juce::jlimit(0.2f, 4.0f, q);
    };

    const float lowCut     = safeFreq(currentParams.lowCutFreq);
    const float lowShelf   = safeFreq(currentParams.lowShelfFreq);
    const float mid1       = safeFreq(currentParams.mid1Freq);
    const float mid2       = safeFreq(currentParams.mid2Freq);
    const float highShelf  = safeFreq(currentParams.highShelfFreq);
    const float airBand    = safeFreq(currentParams.airBandFreq);

    const float lowShelfG  = safeGain(currentParams.lowShelfGain);
    const float mid1G      = safeGain(currentParams.mid1Gain);
    const float mid2G      = safeGain(currentParams.mid2Gain);
    const float highShelfG = safeGain(currentParams.highShelfGain);
    const float airBandG   = safeGain(currentParams.airBandGain);

    const float q1 = safeQ(currentParams.mid1Q);
    const float q2 = safeQ(currentParams.mid2Q);

    // --- update filters
    *eqChain.get<lowCutIndex>().state   = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, lowCut);

    *eqChain.get<lowShelfIndex>().state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, lowShelf, 0.7f, lowShelfG);

    *eqChain.get<mid1Index>().state     = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, mid1, q1, mid1G);

    *eqChain.get<mid2Index>().state     = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, mid2, q2, mid2G);

    *eqChain.get<highShelfIndex>().state= *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, highShelf, 0.7f, highShelfG);

    *eqChain.get<airBandIndex>().state  = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, airBand, 0.7f, airBandG);
}
