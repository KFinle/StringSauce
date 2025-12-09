//
//  ModeProcessor.cpp
//  StringSauce
//
//  Implementation of the Mode Processor
#include "ModeProcessor.hpp"


ModeProcessor::ModeProcessor()
    : currentMode(ToneMode::RHYTHM)
{
}

// prepare mode chains
void ModeProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    rhythmChain.prepare(spec);
    leadChain.prepare(spec);
    cleanChain.prepare(spec);

    setProcessingOrder(rhythmChain, ToneMode::RHYTHM);
    setProcessingOrder(leadChain,   ToneMode::LEAD);
    setProcessingOrder(cleanChain,  ToneMode::CLEAN);
}

void ModeProcessor::setMode(ToneEngine::Mode mode)
{
    currentMode = mode;
}

// process call
void ModeProcessor::process(juce::dsp::ProcessContextReplacing<float>& context,
                            const ToneEngine::EngineParameters& params)
{
    auto& chain = getActiveChain();
    chain.setParameters(params);
    chain.process(context);

    // global autogain
    const float g = params.outputAutoGain;

    if (std::abs(g - 1.0f) > 0.0001f)
    {
        auto& block = context.getOutputBlock();
        const auto numCh = block.getNumChannels();
        const auto numSm = (int) block.getNumSamples();

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            juce::FloatVectorOperations::multiply(data, g, numSm);
        }
    }
}


// ======================================================
void ModeProcessor::reset()
{
    rhythmChain.reset();
    leadChain.reset();
    cleanChain.reset();
}

// get active chain
ModeProcessor::ModeChain& ModeProcessor::getActiveChain()
{
    switch (currentMode)
    {
        case ToneMode::LEAD:   return leadChain;
        case ToneMode::CLEAN:  return cleanChain;
        default:               return rhythmChain;
    }
}

// assign correct processor order
void ModeProcessor::setProcessingOrder(ModeChain& chain, ToneEngine::Mode mode)
{
    switch (mode)
    {
        case ToneMode::LEAD:   chain.order = ModeChain::LEAD_ORDER;   break;
        case ToneMode::CLEAN:  chain.order = ModeChain::CLEAN_ORDER;  break;
        case ToneMode::RHYTHM: chain.order = ModeChain::RHYTHM_ORDER; break;
    }
}

// prepare mode chain
void ModeProcessor::ModeChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    eq.prepare(spec);
    dynamics.prepare(spec);
    saturation.prepare(spec);
    spatial.prepare(spec);
}

void ModeProcessor::ModeChain::setParameters(const ToneEngine::EngineParameters& params)
{
    eq.setParameters(params.eq);
    dynamics.setParameters(params.dynamics);
    saturation.setParameters(params.saturation);
    spatial.setParameters(params.spatial);
}

void ModeProcessor::ModeChain::process(juce::dsp::ProcessContextReplacing<float>& context)
{
    switch (order)
    {
        case RHYTHM_ORDER:
            eq.process(context);
            dynamics.process(context);
            saturation.process(context);
            spatial.process(context);
            break;

        case LEAD_ORDER:
            saturation.process(context);
            eq.process(context);
            dynamics.process(context);
            spatial.process(context);
            break;

        case CLEAN_ORDER:
            eq.process(context);
            saturation.process(context);
            spatial.process(context);
            dynamics.process(context);
            break;
    }
}

void ModeProcessor::ModeChain::reset()
{
    eq.reset();
    dynamics.reset();
    saturation.reset();
    spatial.reset();
}
