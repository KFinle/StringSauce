//  DebugWindow.hpp
//  StringSauce
//
//  Used for toggling the Debug Window
//

#ifndef DebugWindow_h
#define DebugWindow_h

#pragma once
#include <JuceHeader.h>
#include "DebugPanel.hpp"

class DebugWindow : public juce::DocumentWindow
{
public:
    DebugWindow(StringSauceAudioProcessor& processor)
        : DocumentWindow("StringSauce Debug Inspector",
                         juce::Colours::black,
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);

        panel = std::make_unique<DebugPanel>(processor);
        setContentOwned(panel.get(), false);

        setResizable(true, true);
        centreWithSize(360, 800);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    std::unique_ptr<DebugPanel> panel;
};

#endif 
