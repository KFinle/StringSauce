//
//  UILayout.hpp
//  StringSauce
//
//  Defines the layout of the plugin GUI

#ifndef UILayout_h
#define UILayout_h
#pragma once
#include <JuceHeader.h>

struct UIArea
{
    float x, y, w, h;
};

struct UILayout
{
    std::vector<UIArea> knobAreas;
    std::vector<UIArea> modeAreas;
};

inline UILayout makeDefaultStringSauceLayout()
{
    UILayout layout;

    layout.knobAreas =
    {
        // THUMP
        { 483.0f/2874.0f,  767.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f },

        // BODY
        { 1309.0f/2874.0f, 767.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f },

        // CHARACTER
        { 2136.0f/2874.0f, 767.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f },

        // SHIMMER
        { 483.0f/2874.0f, 1492.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f },

        // SLAP
        { 1309.0f/2874.0f, 1492.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f },

        // SPACE
        { 2136.0f/2874.0f, 1492.0f/2160.0f, 256.0f/2874.0f, 256.0f/2160.0f }
    };



     // Mode button click zones
     // These are invisible hitboxes – the actual visuals are
     // drawn by overlaying the full-frame mode images.
     layout.modeAreas =
     {
         // RHYTHM (left)
         { 0.211352f, 0.098275f, 0.175781f, 0.071475f },

         // LEAD (centre)
         { 0.414317f, 0.100916f, 0.175781f, 0.071475f },

         // CLEAN (right)
         { 0.602712f, 0.100359f, 0.175781f, 0.071475f }
     };

     return layout;
}

#endif 
