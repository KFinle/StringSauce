//
//  EQProcessor.hpp
//  StringSauce
//
//  Defines the EQ module


#ifndef EQProcessor_hpp
#define EQProcessor_hpp
#include <JuceHeader.h>
#include "DynamicsProcessor.hpp"
#include "SaturationProcessor.hpp"
#include "SpatialProcessor.hpp"
class EQProcessor
{
public:
    struct EQParameters
    {
        float lowCutFreq    = 20.0f;
        float lowShelfFreq  = 80.0f,    lowShelfGain  = 0.0f;
        float mid1Freq      = 500.0f,   mid1Gain      = 0.0f, mid1Q = 1.0f;
        float mid2Freq      = 1500.0f,  mid2Gain      = 0.0f, mid2Q = 1.0f;
        float highShelfFreq = 8000.0f,  highShelfGain = 0.0f;
        float airBandFreq   = 12000.0f, airBandGain   = 0.0f;
    };

    EQProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const EQParameters& params);
    void process(juce::dsp::ProcessContextReplacing<float>& context);
    void reset();

    // mode-specific EQ
    void setRhythmEQ(float character, float thump, float body, float shimmer);
    void setLeadEQ(float character, float thump, float body, float shimmer);
    void setCleanEQ(float character, float thump, float body, float shimmer);

private:
    juce::dsp::ProcessorChain<

    // low cut
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>,

    // low shelf
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>,

    // mid 1
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>,

    // mid 2
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>,

    // high shelf
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>,

    // air band (high shelf)
    juce::dsp::ProcessorDuplicator<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::IIR::Coefficients<float>>
    > eqChain;

    enum
    {
        lowCutIndex,
        lowShelfIndex,
        mid1Index,
        mid2Index,
        highShelfIndex,
        airBandIndex
    };

    void updateFilterCoefficients();
    EQParameters currentParams;
    juce::dsp::ProcessSpec currentSpec;
};

#endif 
