//  DebugPanel.hpp
//  StringSauce
//
//  Defines the structure of the Debug window,
//  but not the content

#ifndef DebugPanel_h
#define DebugPanel_h
#pragma once
#include <JuceHeader.h>

class DebugPanel : public juce::Component
{
public:
    DebugPanel(StringSauceAudioProcessor& processor)
    {
        viewport = std::make_unique<juce::Viewport>();
        addAndMakeVisible(*viewport);

        content = std::make_unique<DebugContent>(processor);
        viewport->setViewedComponent(content.get(), true);

        viewport->setScrollBarsShown(true, true);
    }

    void resized() override
    {
        viewport->setBounds(getLocalBounds());
    }

private:
    std::unique_ptr<juce::Viewport> viewport;
    std::unique_ptr<DebugContent> content;
};

#endif 
