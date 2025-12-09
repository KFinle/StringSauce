#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterID.hpp"

// ============================================================
// Constructor
// ============================================================
StringSauceAudioProcessor::StringSauceAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
        #if ! JucePlugin_IsMidiEffect
         #if ! JucePlugin_IsSynth
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
         #endif
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        #endif
      )
#endif
{
    presetManager = std::make_unique<PresetManager>(apvts, "StringSauce");
    registerFactoryPresets();
}

// ============================================================
void StringSauceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (samplesPerBlock),
        static_cast<juce::uint32> (getTotalNumOutputChannels())
    };
    inputGain.prepare (spec);
    outputGain.prepare (spec);
    inputGain.setGainDecibels (0.0f);
    outputGain.setGainDecibels (0.0f);
    toneEngine.prepare (spec);
    modeProcessor.prepare (spec);
}

// ============================================================
void StringSauceAudioProcessor::releaseResources ()
{
}

// ============================================================
void StringSauceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any extra output channels
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    // 1. Fetch parameter values
    const float character = *apvts.getRawParameterValue ("character");
    const float thump     = *apvts.getRawParameterValue ("thump");
    const float body      = *apvts.getRawParameterValue ("body");
    const float shimmer   = *apvts.getRawParameterValue ("shimmer");
    const float spank     = *apvts.getRawParameterValue ("spank");
    const float space     = *apvts.getRawParameterValue ("space");
    const int   modeIndex = static_cast<int> (*apvts.getRawParameterValue ("mode"));

    const ToneEngine::Mode mode = static_cast<ToneEngine::Mode> (modeIndex);

    // 2. Input Gain
    inputGain.process (context);

    // 3. Update Tone Engine (macro -> sub-parameters)
    // this calls ParameterMapper::mapEQ/mapDynamics/mapSaturation/mapSpatial
    // and writes the results into toneEngine.currentParams.
    toneEngine.updateParameters (character, thump, body, shimmer, spank, space, mode);
    const auto& params = toneEngine.getCurrentParameters();

    // 4. Mode Processor
    // selects Rhythm / Lead / Clean chain and process the block
    // through EQ, Dynamics, Saturation, Spatial in the appropriate order.
    modeProcessor.setMode (mode);
    modeProcessor.process (context, params);

    // 5. Output Gain
    outputGain.process (context);
}

// ============================================================
bool StringSauceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only mono or stereo output is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
}

// ============================================================
juce::AudioProcessorValueTreeState::ParameterLayout
StringSauceAudioProcessor::createParameterLayout ()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto range = juce::NormalisableRange<float> (0.0f, 1.0f);
    auto def   = 0.5f;
    auto minDef = 0.0f;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("character", "Character", range, minDef));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("thump",     "Thump",     range, def));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("body",      "Body",      range, def));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("shimmer",   "Shimmer",   range, def));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("spank",     "Spank",     range, def));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("space",     "Space",     range, minDef));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Mode", juce::StringArray { "Rhythm", "Lead", "Clean" }, 0));
    return { params.begin(), params.end() };
}

// ============================================================
void StringSauceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void StringSauceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ============================================================
const juce::String StringSauceAudioProcessor::getName () const
{
    return JucePlugin_Name;
}

bool StringSauceAudioProcessor::acceptsMidi () const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StringSauceAudioProcessor::producesMidi () const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StringSauceAudioProcessor::isMidiEffect () const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double StringSauceAudioProcessor::getTailLengthSeconds () const
{
    return 0.0;
}

// ============================================================
bool StringSauceAudioProcessor::hasEditor () const
{
    return true;
}

juce::AudioProcessorEditor* StringSauceAudioProcessor::createEditor ()
{
    return new StringSauceAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new StringSauceAudioProcessor();
}
//==============================================================================
void StringSauceAudioProcessor::registerFactoryPresets()
{
    using namespace ParamID;

    presetManager->addFactoryPreset("Rhythm Warm",
    {
        { CHARACTER, 0.40f },
        { THUMP,     0.70f },
        { BODY,      0.50f },
        { SHIMMER,   0.20f },
        { SPANK,     0.40f },
        { SPACE,     0.00f },
        { MODE,      0.0f }
    });
    
    presetManager->addFactoryPreset("Rhythm Slappy",
    {
        { CHARACTER, 0.199766f },
        { THUMP,     0.475688f },
        { BODY,      0.693409f },
        { SHIMMER,   0.838713f },
        { SPANK,     1.0f },
        { SPACE,     0.0f },
        { MODE,      0.0f }

    });

    presetManager->addFactoryPreset("Lead Air",
    {
        { CHARACTER, 0.65f },
        { THUMP,     0.35f },
        { BODY,      0.40f },
        { SHIMMER,   0.75f },
        { SPANK,     0.45f },
        { SPACE,     0.25f },
        { MODE,      0.5f }
    });

    presetManager->addFactoryPreset("Clean Smooth",
    {
        { CHARACTER, 0.35f },
        { THUMP,     0.25f },
        { BODY,      0.50f },
        { SHIMMER,   0.55f },
        { SPANK,     0.10f },
        { SPACE,     0.40f },
        { MODE,      1.0f }   
    });
}
