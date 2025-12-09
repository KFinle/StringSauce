//
//  DynamicsProcessor.cpp
//  StringSauce
//
//  Implementation of the Dynamics module

#include "DynamicsProcessor.hpp"

DynamicsProcessor::DynamicsProcessor() {}

void DynamicsProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    comp.prepare(spec);
    deessHP.prepare(spec);
    deessLP.prepare(spec);
    deessShelf.prepare(spec);

    comp.reset();
    deessHP.reset();
    deessLP.reset();
    deessShelf.reset();

    comp.setAttack(10.0f);
    comp.setRelease(120.0f);
    comp.setThreshold(-18.0f);
    comp.setRatio(2.0f);

    auto neutralShelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 6000.0f, 0.707f, 1.0f);
    deessShelf.coefficients = neutralShelf;

    updateTransientEnvelopes();
    makeupLinear = 1.0f;

    isPrepared = true;
}

void DynamicsProcessor::reset()
{
    comp.reset();
    deessHP.reset();
    deessLP.reset();
    deessShelf.reset();

    envFast = envSlow = 0.0f;
    lastDeessGainLinear = 1.0f;
}

void DynamicsProcessor::setParameters(const DynamicsParameters& p)
{
    params = p;
    updateCompressor();
    updateDeesserFilters();
    updateTransientEnvelopes();
    updateMakeup();
}

void DynamicsProcessor::updateCompressor()
{
    comp.setThreshold(params.compThreshold);
    comp.setRatio(std::max(1.0f, params.compRatio));
    comp.setAttack(std::max(0.1f, params.compAttack));
    comp.setRelease(std::max(1.0f, params.compRelease));
}

void DynamicsProcessor::updateDeesserFilters()
{
    const float center = juce::jlimit(2000.0f, 16000.0f, params.deesserFreq);
    const float bw = 1.414f;

    const float hpFreq = center / bw;
    const float lpFreq = center * bw;

    deessHP.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpFreq);
    deessLP.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpFreq);
}

void DynamicsProcessor::updateDeesserShelf(float linearCut)
{
    const float shelfFreq = juce::jlimit(2000.0f, 16000.0f, params.deesserFreq);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, shelfFreq, 0.707f, juce::jlimit(0.1f, 1.0f, linearCut));

    deessShelf.coefficients = coeffs;
}

void DynamicsProcessor::updateTransientEnvelopes()
{
    const float aFast = 0.002f, rFast = 0.020f;
    const float aSlow = 0.020f, rSlow = 0.200f;

    atkCoeffFast = std::exp(-1.0f / (aFast * sampleRate));
    relCoeffFast = std::exp(-1.0f / (rFast * sampleRate));
    atkCoeffSlow = std::exp(-1.0f / (aSlow * sampleRate));
    relCoeffSlow = std::exp(-1.0f / (rSlow * sampleRate));
}

void DynamicsProcessor::updateMakeup()
{
    makeupLinear = dbToLin(params.compMakeupGain);
}

void DynamicsProcessor::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    if (!isPrepared)
        return;
    
    const auto& p = params;


    const bool isBypassed =
        std::abs(p.compRatio - 1.0f) < 0.01f &&
        p.compThreshold > 0.0f &&
        std::abs(p.transientAttack) < 0.001f &&
        std::abs(p.transientSustain) < 0.001f;

    if (isBypassed) return;



    auto& block = context.getOutputBlock();
    const int numCh = (int)block.getNumChannels();
    const int numSm = (int)block.getNumSamples();

    if (numCh <= 0 || numSm <= 0) return;

    // Step 1) Broadband Compression
    comp.process(context);

    // Step 2) De-esser
    static thread_local juce::HeapBlock<float> tmp;
    tmp.allocate(numSm, false);
    float* mono = tmp.getData();

    for (int i = 0; i < numSm; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            s += block.getChannelPointer(ch)[i];
        mono[i] = s / std::max(1, numCh);
    }

    float* channels[] = { mono };
    juce::dsp::AudioBlock<float> mBlock(channels, 1, (size_t)numSm);
    deessHP.process(juce::dsp::ProcessContextReplacing<float>(mBlock));
    deessLP.process(juce::dsp::ProcessContextReplacing<float>(mBlock));

    // Compute RMS in dB
    float accum = 0.0f;
    for (int i = 0; i < numSm; ++i) accum += mono[i] * mono[i];
    float rms = std::sqrt(accum / std::max(1, numSm));
    float rmsDb = linToDb(std::max(1.0e-8f, rms));

    float over = rmsDb - params.deesserThreshold;
    float cutDb = (over > 0.0f) ? (over * (params.deesserRatio - 1.0f)) : 0.0f;
    float targetCutLinear = dbToLin(-cutDb);

    lastDeessGainLinear = lastDeessGainLinear * 0.8f + targetCutLinear * 0.2f;
    updateDeesserShelf(lastDeessGainLinear);

    deessShelf.process(context);

    // Step 3) Transient shaping
    if (numCh > 0)
    {
        auto* ch0 = block.getChannelPointer(0);
        for (int i = 0; i < numSm; ++i)
        {
            const float x = std::abs(ch0[i]);

            // fast and slow envelopes
            envFast = (x > envFast)
                          ? (atkCoeffFast * envFast + (1.0f - atkCoeffFast) * x)
                          : (relCoeffFast * envFast + (1.0f - relCoeffFast) * x);

            envSlow = (x > envSlow)
                          ? (atkCoeffSlow * envSlow + (1.0f - atkCoeffSlow) * x)
                          : (relCoeffSlow * envSlow + (1.0f - relCoeffSlow) * x);

            const float trans = juce::jlimit(-1.0f, 1.0f, envFast - envSlow);

            float gAtk = 1.0f + params.transientAttack * trans * 2.0f;
            float gSus = 1.0f + params.transientSustain * (envSlow * 0.5f);

            float g = juce::jlimit(0.25f, 4.0f, gAtk * gSus);

            for (int ch = 0; ch < numCh; ++ch)
                block.getChannelPointer(ch)[i] *= g;
        }
    }

    // Step 4) Makeup gain 
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        juce::FloatVectorOperations::multiply(data, makeupLinear, numSm);
    }
}
