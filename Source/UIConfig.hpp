//
//  UIConfig.hpp
//  StringSauce
//
//  Toggle between the default DAW look or the PNGs for the GUI


#ifndef UIConfig_h
#define UIConfig_h

#pragma once

enum class UIStyle
{
    DefaultLook,
    PNGLook        
};

struct UIConfig
{
    UIStyle style = UIStyle::DefaultLook;
};


#endif
