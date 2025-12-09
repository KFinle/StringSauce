//
//  SpatialProcessor.cpp
//  StringSauce
//
//  Implementation of the Spacial Module
//
//  Created by Khris Finley on 2025-11-10.
//  © 2025 FinleyAud.io. All rights reserved.
//

#include "SpatialProcessor.hpp"

SpatialProcessor::SpatialProcessor() {}

void SpatialProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maxDelaySamples = (size_t) juce::roundToInt(sampleRate * 1.5);

    delayL.prepare(spec);
    delayR.prepare(spec);
    delayL.setMaximumDelayInSamples((int)maxDelaySamples);
    delayR.setMaximumDelayInSamples((int)maxDelaySamples);
    delayL.reset();
    delayR.reset();

    chorus.prepare(spec);
    reverb.prepare(spec);

    delayMix.prepare(spec);
    chorusMix.prepare(spec);
    reverbMix.prepare(spec);

    updateDelay();
    updateChorus();
    updateReverb();
}



void SpatialProcessor::reset()
{
    delayL.reset();
    delayR.reset();
    chorus.reset();
    reverb.reset();

    delayMix.reset();
    chorusMix.reset();
    reverbMix.reset();
}

void SpatialProcessor::setParameters(const SpatialParameters& p)
{
    params = p;

    updateDelay();
    updateChorus();
    updateReverb();

    delayMix.setWetMixProportion(params.delayMix);
    chorusMix.setWetMixProportion(params.chorusMix);
    reverbMix.setWetMixProportion(params.reverbMix);
}

void SpatialProcessor::updateDelay()
{
    // convert ms to samples
    const float dL = (params.delayTimeLeft  / 1000.0f) * (float) sampleRate;
    const float dR = (params.delayTimeRight / 1000.0f) * (float) sampleRate;

    delayL.setDelay(juce::jlimit(1.0f, (float)maxDelaySamples - 1.0f, dL));
    delayR.setDelay(juce::jlimit(1.0f, (float)maxDelaySamples - 1.0f, dR));
}

void SpatialProcessor::updateChorus()
{
    chorus.setRate (juce::jlimit(0.01f, 5.0f, params.chorusRate));
    chorus.setDepth(juce::jlimit(0.0f, 1.0f, params.chorusDepth));
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);
}

void SpatialProcessor::updateReverb()
{
    rvParams.roomSize   = juce::jlimit(0.0f, 1.0f, params.reverbSize);
    rvParams.damping    = juce::jlimit(0.0f, 1.0f, params.reverbDamping);
    rvParams.width      = juce::jlimit(0.0f, 1.0f, params.reverbWidth);
    rvParams.wetLevel   = 1.0f;
    rvParams.dryLevel   = 0.0f;
    rvParams.freezeMode = 0.0f;

    reverb.setParameters(rvParams);
}

void SpatialProcessor::applyStereoWidth(juce::dsp::AudioBlock<float>& block, float width)
{
    if (block.getNumChannels() < 2) return;

    auto* L = block.getChannelPointer(0);
    auto* R = block.getChannelPointer(1);
    const auto N = (int) block.getNumSamples();

    for (int i = 0; i < N; ++i)
    {
        const float M = 0.70710678f * (L[i] + R[i]);
        const float S = 0.70710678f * (L[i] - R[i]) * width;
        L[i] = M + S;
        R[i] = M - S;
    }
}

void SpatialProcessor::process(const juce::dsp::ProcessContextReplacing<float>& ctx)
{
    auto& block = ctx.getOutputBlock();
    const size_t numCh = block.getNumChannels();
    const size_t numSm = block.getNumSamples();
    
    const auto& p = params;
    const bool allDry =
        p.reverbMix <= 0.0001f &&
        p.delayMix  <= 0.0001f &&
        p.chorusMix <= 0.0001f &&
        std::abs(p.stereoWidth - 1.0f) < 0.001f;

    if (allDry) return;


    // delay
    delayMix.pushDrySamples(block);

    for (size_t ch = 0; ch < numCh; ++ch)
    {
        auto* data = block.getChannelPointer(ch);

        for (size_t i = 0; i < numSm; ++i)
        {
            const float in  = data[i];
            float dOut = 0.0f;

            if (ch == 0)
            {
                dOut = delayL.popSample(0);
                float fb = juce::jlimit(0.0f, 0.99f, params.delayFeedback);
                delayL.pushSample(0, in + dOut * fb);
            }
            else
            {
                dOut = delayR.popSample(0);
                float fb = juce::jlimit(0.0f, 0.99f, params.delayFeedback);
                delayR.pushSample(0, in + dOut * fb);
            }
            data[i] = dOut;
        }
    }

    delayMix.mixWetSamples(block);

    // chorus
    chorusMix.pushDrySamples(block);
    {
        juce::dsp::ProcessContextReplacing<float> chorusCtx(block);
        chorus.process(chorusCtx);
    }
    chorusMix.mixWetSamples(block);

    // reverb
    reverbMix.pushDrySamples(block);
    {
        juce::dsp::ProcessContextReplacing<float> rvCtx(block);
        reverb.process(rvCtx);
    }
    reverbMix.mixWetSamples(block);

    // widening
    applyStereoWidth(block, juce::jlimit(0.0f, 2.0f, params.stereoWidth));
}
