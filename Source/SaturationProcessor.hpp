//
//  SaturationProcessor.hpp
//  StringSauce
//
//  Defines the Saturation Module

#ifndef SaturationProcessor_hpp
#define SaturationProcessor_hpp
#pragma once

#include <JuceHeader.h>

class SaturationProcessor : public juce::dsp::ProcessorBase
{
public:
    enum class Type { Tape, Tube, Transistor, Exciter };

    struct SaturationParameters
    {
        Type type = Type::Tape;
        float drive = 0.0f;
        float mix = 1.0f;
        float tone = 0.5f;
        float bias = 0.0f;
    };

    SaturationProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(const juce::dsp::ProcessContextReplacing<float>& context) override;
    void reset() override;

    void setParameters(const SaturationParameters& newParams);
    void setType(Type newType);

private:
    void updateWaveshaper();
    void updateToneFilter();
    void updateDrive();

    SaturationParameters params;
    Type currentType = Type::Tape;

    juce::dsp::WaveShaper<float> shaper;
    juce::dsp::IIR::Filter<float> toneFilter;
    juce::dsp::DryWetMixer<float> dryWet;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    float driveGain = 1.0f;
    double sampleRate = 44100.0;
};

#endif 
