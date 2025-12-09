//  DebugContent.hpp
//  StringSauce
//
//  This is where we construct the content shown
//  in the Debug window
//

#ifndef DebugContent_h
#define DebugContent_h
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DebugContent : public juce::Component, private juce::Timer
{
public:
    DebugContent(StringSauceAudioProcessor& processorRef) : processor(processorRef)
    {
        startTimerHz(30); 
        setSize(330, 1600);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.9f));
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);

        int y = 12;

        auto line = [&](const juce::String& t)
        {
            g.drawText(t, 10, y, getWidth() - 20, 20, juce::Justification::left);
            y += 18;
        };

        const auto& apvts = processor.apvts;
        const auto& params = cachedParams;

        // -------------------------
        line("== Macro Parameters ==");
        line("Character: " + juce::String(*apvts.getRawParameterValue("character")));
        line("Thump:     " + juce::String(*apvts.getRawParameterValue("thump")));
        line("Body:      " + juce::String(*apvts.getRawParameterValue("body")));
        line("Shimmer:   " + juce::String(*apvts.getRawParameterValue("shimmer")));
        line("Space:     " + juce::String(*apvts.getRawParameterValue("space")));
        line("Slap:      " + juce::String(*apvts.getRawParameterValue("spank")));
        line("Mode:      " + juce::String((int)*apvts.getRawParameterValue("mode")));

        // -------------------------
        line("");
        line("== EQ Parameters ==");
        const auto& eq = params.eq;
        line("LowCut Hz:      " + juce::String(eq.lowCutFreq));
        line("LowShelf Gain:  " + juce::String(eq.lowShelfGain));
        line("Mid1 F/Q/G:     " + juce::String(eq.mid1Freq) + " / "
                                 + juce::String(eq.mid1Q) + " / "
                                 + juce::String(eq.mid1Gain));
        line("Mid2 F/Q/G:     " + juce::String(eq.mid2Freq) + " / "
                                 + juce::String(eq.mid2Q) + " / "
                                 + juce::String(eq.mid2Gain));
        line("HighShelf Gain: " + juce::String(eq.highShelfGain));
        line("AirBand Gain:   " + juce::String(eq.airBandGain));

        // -------------------------
        line("");
        line("== Dynamics Parameters ==");
        const auto& dyn = params.dynamics;
        line("Threshold:      " + juce::String(dyn.compThreshold));
        line("Ratio:          " + juce::String(dyn.compRatio));
        line("Attack/Release: " + juce::String(dyn.compAttack) + " / "
                                 + juce::String(dyn.compRelease));
        line("Makeup Gain:    " + juce::String(dyn.compMakeupGain));
        line("DeEss F/Th/R:   " + juce::String(dyn.deesserFreq) + " / "
                                 + juce::String(dyn.deesserThreshold) + " / "
                                 + juce::String(dyn.deesserRatio));
        line("Transient A/S:  " + juce::String(dyn.transientAttack) + " / "
                                 + juce::String(dyn.transientSustain));

        // -------------------------
        line("");
        line("== Saturation Parameters ==");
        const auto& sat = params.saturation;
        line("Type:           " + juce::String((int)sat.type));
        line("Drive:          " + juce::String(sat.drive));
        line("Tone:           " + juce::String(sat.tone));
        line("Bias:           " + juce::String(sat.bias));
        line("Mix:            " + juce::String(sat.mix));

        // -------------------------
        line("");
        line("== Spatial Parameters ==");
        const auto& sp = params.spatial;
        line("Rev Mix/Size/Damp: "
             + juce::String(sp.reverbMix) + " / "
             + juce::String(sp.reverbSize) + " / "
             + juce::String(sp.reverbDamping));
        line("Delay L/R (ms): " + juce::String(sp.delayTimeLeft) + " / "
                                 + juce::String(sp.delayTimeRight));
        line("Delay Mix/Fb:   " + juce::String(sp.delayMix) + " / "
                                 + juce::String(sp.delayFeedback));
        line("Chorus Mix:     " + juce::String(sp.chorusMix));
        line("Rate/Depth:     " + juce::String(sp.chorusRate) + " / "
                                 + juce::String(sp.chorusDepth));
        line("Stereo Width:   " + juce::String(sp.stereoWidth));
    }

private:
    StringSauceAudioProcessor& processor;
    ToneEngine::EngineParameters cachedParams;

    void timerCallback() override
    {
        cachedParams = processor.getEngineParams();
        repaint();
    }
};

#endif 
