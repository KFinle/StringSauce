//
//  UIFactory.hpp
//  StringSauce
//
//  Used for creating and placing the GUI components of the plugin

#ifndef UIFactory_h
#define UIFactory_h
#pragma once
#include <JuceHeader.h>

#include "UIConfig.hpp"
#include "PlaceholderKnob.hpp"
#include "PlaceholderButton.hpp"
#include "SpriteKnob.hpp"
#include "ImageToggleButton.hpp"


struct UIFactory
{

    std::unique_ptr<juce::Component>
    static createModeButton(UIStyle style, const juce::String& modeName)
    {
        juce::Image img;
        
        if (modeName == "Rhythm")
            img = juce::ImageFileFormat::loadFrom(BinaryData::StringSauce_Button_Rhythm_png, BinaryData::StringSauce_Button_Rhythm_pngSize);
        else if (modeName == "Lead")
            img = juce::ImageFileFormat::loadFrom(BinaryData::StringSauce_Button_Lead_png, BinaryData::StringSauce_Button_Lead_pngSize);
        else if (modeName == "Clean")
            img = juce::ImageFileFormat::loadFrom(BinaryData::StringSauce_Button_Clean_png, BinaryData::StringSauce_Button_Clean_pngSize);

        auto* button = new ImageToggleButton(img);
        
        button->setClickingTogglesState(true);
        return std::unique_ptr<juce::Component>(button);
    }
    
    static std::unique_ptr<juce::Component> createKnob(UIStyle style)
    {
        if (style == UIStyle::PNGLook)
        {
            juce::Image sheet = juce::ImageFileFormat::loadFrom(BinaryData::StringSauce_Knob_SpriteSheet_png, BinaryData::StringSauce_Knob_SpriteSheet_pngSize);
            
            // 256px frames, 18 columns, 15 rows
            auto knob = std::make_unique<SpriteKnob>(sheet, 256, 18, 15);
            
            return knob;
        }
        
        // fallback – default look
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        return slider;
    }
};


#endif
