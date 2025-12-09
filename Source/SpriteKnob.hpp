//
//  SpriteKnob.hpp
//  StringSauce
//
//  Created by Khris Finley on 2025-11-25.
//  Copyright © 2025 FinleyAud.io. All rights reserved.
//

#ifndef SpriteKnob_h
#define SpriteKnob_h
#pragma once
#include <JuceHeader.h>

class SpriteKnob : public juce::Slider
{
public:
    SpriteKnob(const juce::Image& spriteSheet,
               int frameSizePx,
               int columns,
               int rows)
        : sheet(spriteSheet),
          frameSize(frameSizePx),
          numColumns(columns),
          numRows(rows)
    {
        jassert(sheet.isValid());
        totalFrames = numColumns * numRows;

        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        // Let APVTS control the actual range; we just assume 0..1
        setRange(0.0, 1.0, 0.0);
    }

    void paint(juce::Graphics& g) override
    {
        if (! sheet.isValid())
            return;

        const float norm = (float) getValue();
        const int frameIndex = juce::jlimit(
            0, totalFrames - 1,
            (int) std::round(norm * (float)(totalFrames - 1))
        );

        const int row = frameIndex / numColumns;
        const int col = frameIndex % numColumns;

        const juce::Rectangle<int> src(
            col * frameSize,
            row * frameSize,
            frameSize,
            frameSize
        );

        auto dst = getLocalBounds();

        g.drawImage(sheet,
                    dst.getX(), dst.getY(), dst.getWidth(), dst.getHeight(),
                    src.getX(), src.getY(), src.getWidth(), src.getHeight());
    }

private:
    juce::Image sheet;
    int frameSize   = 256;
    int numColumns  = 18;
    int numRows     = 15;
    int totalFrames = 0;
};

#endif /* SpriteKnob_h */
