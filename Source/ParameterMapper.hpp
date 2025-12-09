#ifndef ParameterMapper_hpp
#define ParameterMapper_hpp
#pragma once

#include <JuceHeader.h>
#include "EQProcessor.hpp"
#include "DynamicsProcessor.hpp"
#include "SaturationProcessor.hpp"
#include "SpatialProcessor.hpp"
#include "ToneMode.hpp"

class ParameterMapper
{
public:
    static EQProcessor::EQParameters mapEQ(float character, float thump, float body, float shimmer,ToneMode mode);

    static DynamicsProcessor::DynamicsParameters mapDynamics(float thump, float body, float shimmer, float spank, ToneMode mode);

    static SaturationProcessor::SaturationParameters mapSaturation(float character, float body, float shimmer, ToneMode mode);

    static SpatialProcessor::SpatialParameters mapSpatial(float body, float shimmer, float space, ToneMode mode);

private:
    static SaturationProcessor::Type selectSaturationType(float character, ToneMode mode);
};
#endif
