//
//  SaturationProcessor.cpp
//  StringSauce
//
//  Implementation of the Saturation Module

#include "SaturationProcessor.hpp"

namespace
{
    // waveshaping curves
    float tapeShape(float x)        { return std::tanh(0.9f * x) * 0.8f; }
    float tubeShape(float x)        { return std::tanh(1.5f * x - 0.2f * x * x * x); }
    float transistorShape(float x)  { return std::tanh(2.5f * x) + 0.05f * std::sin(6.0f * x); }
    float exciterShape(float x)     { return 0.6f * std::sin(x * 2.0f) + 0.4f * x; }
}

SaturationProcessor::SaturationProcessor()
{
    currentType = Type::Tape;
}

void SaturationProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    shaper.reset();
    toneFilter.reset();
    dryWet.reset();
    dryWet.prepare(spec);
    dryWet.setMixingRule(juce::dsp::DryWetMixingRule::linear);
    dryWet.setWetMixProportion(params.mix);
    updateWaveshaper();
    updateToneFilter();
    updateDrive();
    oversampler->reset();
}

void SaturationProcessor::reset()
{
    shaper.reset();
    toneFilter.reset();
    dryWet.reset();
    if (oversampler) oversampler->reset();
}

void SaturationProcessor::setParameters(const SaturationParameters& newParams)
{
    params = newParams;

    if (params.type != currentType)
    {
        currentType = params.type;
        updateWaveshaper();
    }

    updateDrive();
    updateToneFilter();
    dryWet.setWetMixProportion(params.mix);
}

void SaturationProcessor::setType(Type newType)
{
    if (currentType != newType)
    {
        currentType = newType;
        updateWaveshaper();
    }
}

void SaturationProcessor::updateDrive()
{
    driveGain = juce::Decibels::decibelsToGain(params.drive * 18.0f);
}

void SaturationProcessor::updateToneFilter()
{
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                                                                     sampleRate,
                                                                     juce::jmap(params.tone, 2000.0f, 8000.0f),
                                                                     0.707f,
                                                                     juce::jmap(params.tone,0.5f, 2.0f)
    );

    toneFilter.coefficients = coeffs;
}

void SaturationProcessor::updateWaveshaper()
{
    switch (currentType)
    {
        case Type::Tape:        shaper.functionToUse = tapeShape;       break;
        case Type::Tube:        shaper.functionToUse = tubeShape;       break;
        case Type::Transistor:  shaper.functionToUse = transistorShape; break;
        case Type::Exciter:     shaper.functionToUse = exciterShape;    break;
    }
}

void SaturationProcessor::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    if (!oversampler) return;
    
    const auto& p = params;

    if (p.mix <= 0.0001f || p.drive <= 0.0001f) return;


    auto& output = context.getOutputBlock();
    const auto numCh = output.getNumChannels();
    const auto numSm = output.getNumSamples();
    if (numCh == 0 || numSm == 0) return;

    dryWet.pushDrySamples(output);

    // oversample
    auto oversampledBlock = oversampler->processSamplesUp(output);
    auto osBlock = oversampledBlock;

    // apply drive
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        if (auto* data = osBlock.getChannelPointer(ch))
        {
            for (size_t i = 0; i < osBlock.getNumSamples(); ++i) data[i] = (data[i] + params.bias) * driveGain;
        }
    }

    // waveshaper
    shaper.process(juce::dsp::ProcessContextReplacing<float>(osBlock));

    // tone filter
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        if (auto* data = osBlock.getChannelPointer(ch))
        {
            juce::dsp::AudioBlock<float> chBlock(&data, 1, osBlock.getNumSamples());
            juce::dsp::ProcessContextReplacing<float> ctx(chBlock);
            toneFilter.process(ctx);
        }
    }

    // downsample
    oversampler->processSamplesDown(output);

    // output compensation
    const float driveCompensity = 0.6f;
    const float comp = 1.0f / juce::jmax(1.0f, driveGain * driveCompensity);

    for (size_t ch = 0; ch < output.getNumChannels(); ++ch)
        juce::FloatVectorOperations::multiply(output.getChannelPointer(ch), comp, (int)numSm);

    dryWet.mixWetSamples(output);

}

