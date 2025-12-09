//
//  ImageToggleButton.hpp
//  StringSauce
//
//  Created by Khris Finley on 2025-11-25.
//  Copyright © 2025 FinleyAud.io. All rights reserved.
//

#ifndef ImageToggleButton_h
#define ImageToggleButton_h

#pragma once
#include <JuceHeader.h>

class ImageToggleButton : public juce::Button
{
public:
    ImageToggleButton(const juce::Image& activeImage)
        : juce::Button("modeButton"), img(activeImage) {}

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override
    {
//        if (getToggleState() && img.isValid())
//            g.drawImage(img, getLocalBounds().toFloat());
    }

private:
    juce::Image img;
};

#endif /* ImageToggleButton_h */
