//
//  ParameterMapper.cpp
//  StringSauce
//
//  This is the implemention of the Parameter Mapper.
//  If we ever need to change the sound of the plugin,
//  this is where we adjust the values.

#include "ParameterMapper.hpp"
#include "ToneMode.hpp"
#include <cmath>

using Mode = ToneMode;

// utility helpers
static inline float centreAround0(float v) { return (v - 0.5f) * 2.0f; }
static inline float softSat(float x)      { return std::tanh(1.5f * x); }

static inline float musicalCurve(float x)
{
    const float dead = 0.15f;
    if (std::abs(x) < dead) return 0.0f;
    float s = (x < 0.f ? -1.f : 1.f);
    float m = (std::abs(x) - dead) / (1.f - dead);
    return softSat(juce::jlimit(0.f, 1.f, m) * s);
}

static inline float linFromDbClamped(float db)
{
    return juce::Decibels::decibelsToGain(db);
}

// ==========================================================
// EQ
// ==========================================================
EQProcessor::EQParameters ParameterMapper::mapEQ(
    float character, float thump, float body, float shimmer, Mode mode)
{
    EQProcessor::EQParameters p{};

    const float t  = musicalCurve(centreAround0(thump));
    const float b  = musicalCurve(centreAround0(body));
    const float sh = musicalCurve(centreAround0(shimmer));

    float lowCutMin, lowCutMax;
    float lowShelfF;
    float mid1Base, mid2Base;
    float hiShelfBase, airBase;

    float mid1RangeDb, mid2RangeDb, hiShelfRangeDb, airRangeDb;

    switch (mode)
    {
        case Mode::RHYTHM:
            lowCutMin       = 40.f;     lowCutMax   = 140.f;  lowShelfF = 100.f;
            mid1Base        = 800.f;    mid2Base    = 2000.f;
            hiShelfBase     = 8000.f;   airBase     = 12000.f;
            mid1RangeDb     = 5.f;      mid2RangeDb = 3.f;
            hiShelfRangeDb  = 2.5f;     airRangeDb  = 2.0f;
            break;

        case Mode::LEAD:
            lowCutMin       = 60.f;     lowCutMax   = 180.f;  lowShelfF = 110.f;
            mid1Base        = 1200.f;   mid2Base    = 2300.f;
            hiShelfBase     = 7500.f;   airBase     = 11000.f;
            mid1RangeDb     = 6.f;      mid2RangeDb = 4.f;
            hiShelfRangeDb  = 2.0f;     airRangeDb  = 1.5f;
            break;

        case Mode::CLEAN:
        default:
            lowCutMin       = 50.f;     lowCutMax   = 160.f;  lowShelfF = 120.f;
            mid1Base        = 3000.f;   mid2Base    = 1500.f;
            hiShelfBase     = 9000.f;   airBase     = 13000.f;
            mid1RangeDb     = 4.f;      mid2RangeDb = 3.f;
            hiShelfRangeDb  = 3.0f;     airRangeDb  = 2.0f;
            break;
    }

    // THUMP
    float tPos = std::max(0.f, t);
    p.lowCutFreq = juce::jmap(tPos, 0.f, 1.f, lowCutMin, lowCutMax);

    float lowShelfDb = (mode == Mode::RHYTHM ? 6.f : mode == Mode::LEAD   ? 5.f : 4.f) * t;

    p.lowShelfFreq = lowShelfF + t * 15.f;
    p.lowShelfGain = linFromDbClamped(lowShelfDb);

    float lowMidDb = t * 2.5f;

    // BODY
    p.mid1Freq = mid1Base + b * 350.f;
    p.mid2Freq = mid2Base + b * 250.f;

    const float baseQ1 = (mode == Mode::CLEAN ? 0.9f : 1.f);
    const float baseQ2 = (mode == Mode::CLEAN ? 1.1f : 1.f);

    p.mid1Q = juce::jlimit(0.4f, 2.5f, baseQ1 + (b < 0.f ? -0.3f * std::abs(b) : 0.2f * b));
    p.mid2Q = juce::jlimit(0.4f, 2.5f, baseQ2 + (b < 0.f ? -0.2f * std::abs(b) : 0.3f * b));

    float mid1Db = b * mid1RangeDb;
    float mid2Db = b * 0.5f * mid2RangeDb;

    if (mode == Mode::RHYTHM || mode == Mode::LEAD) mid1Db += lowMidDb;

    // SHIMMER
    p.highShelfFreq = hiShelfBase + sh * 1200.f;
    p.airBandFreq   = airBase     + sh * 1600.f;

    float hiShelfDb = sh * hiShelfRangeDb;
    float airDb     = sh * airRangeDb;

    p.mid1Gain      = linFromDbClamped(mid1Db);
    p.mid2Gain      = linFromDbClamped(mid2Db);
    p.highShelfGain = linFromDbClamped(hiShelfDb);
    p.airBandGain   = linFromDbClamped(airDb);

    return p;
}

// ==========================================================
// DYNAMICS
// ==========================================================
DynamicsProcessor::DynamicsParameters ParameterMapper::mapDynamics(
    float thump, float body, float shimmer, float spank, Mode mode)
{
    DynamicsProcessor::DynamicsParameters d{};

    const float t  = musicalCurve(centreAround0(thump));
    const float b  = musicalCurve(centreAround0(body));
    const float sh = musicalCurve(centreAround0(shimmer));
    const float k  = musicalCurve(centreAround0(spank));

    float baseThHi, baseThLo, baseRatio, ratioExtra, baseAtk, baseRel;

    switch (mode)
    {
        case Mode::RHYTHM:
            baseThHi    = +3.f;     baseThLo    = -24.f;
            baseRatio   = 1.8f;     ratioExtra  = 3.0f;
            baseAtk     = 12.f;     baseRel     = 160.f;
            break;

        case Mode::LEAD:
            baseThHi    = +1.f;     baseThLo    = -28.f;
            baseRatio   = 2.0f;     ratioExtra  = 1.3f;
            baseAtk     = 10.f;     baseRel     = 150.f;
            break;

        case Mode::CLEAN:
        default:
            baseThHi    = +4.f;     baseThLo    = -12.f;
            baseRatio   = 1.2f;     ratioExtra  = 0.6f;
            baseAtk     = 12.f;     baseRel     = 180.f;
            break;
    }

    // SLAP threshold/ratio
    float thresholdRange = baseThLo - baseThHi;
    float kWindow        = 0.30f;

    d.compThreshold = baseThHi + (0.5f + k * kWindow) * thresholdRange;

    float kPos  = std::max(0.f, k);
    d.compRatio = baseRatio + kPos * ratioExtra;
    d.compRatio = std::max(1.f, d.compRatio);

    // THUMP attack/release
    float atk = baseAtk;
    float rel = baseRel;

    atk += (-t) * 5.f;
    atk += ( t) * -6.f;

    rel += ( t) * 40.f;
    rel += (-t) * -20.f;

    // SLAP -> ATTACK
    if (mode == Mode::RHYTHM)
    {
        atk += juce::jmap(kPos, 0.f, 1.f, 0.f, 90.f);
        if (k < 0.f) atk += k * 10.f;
    }
    else if (mode == Mode::LEAD)
    {
        atk += juce::jmap(kPos, 0.f, 1.f, 0.f, 50.f);
        if (k < 0.f) atk += k * 8.f;
    }
    else
    {
        atk += juce::jmap(kPos, 0.f, 1.f, 0.f, 35.f);
        if (k < 0.f) atk += k * 6.f;
    }
    
    // SLAP -> RELEASE
    if (k > 0.f)
    {
        float maxReleaseShift = (mode == Mode::RHYTHM ? -120.f : mode == Mode::LEAD   ? -60.f : -40.f);
        rel += kPos * maxReleaseShift;
    }


    d.compAttack  = juce::jlimit(1.f, 150.f, atk);
    d.compRelease = juce::jlimit(50.f, 600.f, rel);

    // BODY / THUMP makeup
    float makeup = 0.f;

    switch (mode)
    {
        case Mode::RHYTHM: makeup = b * 3.f;   break;
        case Mode::LEAD:   makeup = b * 4.f;   break;
        case Mode::CLEAN:  makeup = b * 2.5f;  break;
    }

    float thumpMakeup = juce::jlimit(0.f, (mode == Mode::RHYTHM ? 4.f : mode == Mode::LEAD ? 3.5f : 3.f), t * 4.f);

    // SLAP makeup
    float spankMakeup = 0.f;

    if (k >= 0.f)
    {
        float maxBoost = (mode == Mode::RHYTHM ? 0.5f : mode == Mode::LEAD   ? 0.7f : 0.5f);
        spankMakeup = k * maxBoost;
    }
    else
    {
        float maxCut = (mode == Mode::RHYTHM ? 1.5f : mode == Mode::LEAD   ? 1.0f : 0.7f);
        spankMakeup = k * maxCut;
    }

    float totalMakeup = makeup + thumpMakeup + spankMakeup;
    d.compMakeupGain = juce::jlimit(0.f, 8.f, totalMakeup);

    // transients
    float atkAmt, susAmt;

    switch (mode)
    {
        case Mode::RHYTHM:
            atkAmt = juce::jlimit(-1.f, 1.f, k * 0.9f);
            susAmt = juce::jlimit(-1.f, 1.f, b * 0.5f + t * 0.3f);
            break;

        case Mode::LEAD:
            atkAmt = juce::jlimit(-1.f, 1.f, k * 0.9f);
            susAmt = juce::jlimit(-1.f, 1.f, b * 0.7f + kPos * 0.3f + t * 0.3f);
            break;

        case Mode::CLEAN:
        default:
            atkAmt = juce::jlimit(-1.f, 1.f, k * 0.7f);
            susAmt = juce::jlimit(-1.f, 1.f, b * 0.5f + t * 0.2f);
            break;
    }

    d.transientAttack  = atkAmt;
    d.transientSustain = susAmt;

    // SHIMMER → DE-ESSER
    float deessCenter = (mode == Mode::RHYTHM ? 5500.f : mode == Mode::LEAD ? 6000.f : 6500.f);

    d.deesserFreq = juce::jlimit(3000.f, 9000.f, deessCenter + sh * 1500.f);

    d.deesserThreshold = -20.f + (-sh * 3.f);
    d.deesserRatio     = (mode == Mode::CLEAN ? 1.5f : 2.f);

    return d;
}

// ==========================================================
// SATURATION
// ==========================================================
SaturationProcessor::SaturationParameters ParameterMapper::mapSaturation(
    float character, float body, float shimmer, Mode mode)
{
    SaturationProcessor::SaturationParameters s{};

    const float c     = juce::jlimit(0.f, 1.f, character);
    const float cNorm = std::pow(c, 0.75f);

    const float b     = musicalCurve(centreAround0(body));
    const float sh    = musicalCurve(centreAround0(shimmer));

    switch (mode)
    {
        case Mode::RHYTHM:
            s.type = (cNorm < 0.35f ? SaturationProcessor::Type::Tape :
                      cNorm < 0.70f ? SaturationProcessor::Type::Tube :
                                      SaturationProcessor::Type::Transistor);
            break;

        case Mode::LEAD:
            s.type = (cNorm < 0.30f ? SaturationProcessor::Type::Tape :
                      cNorm < 0.70f ? SaturationProcessor::Type::Tube :
                                      SaturationProcessor::Type::Exciter);
            break;

        case Mode::CLEAN:
        default:
            s.type = (cNorm < 0.50f ? SaturationProcessor::Type::Tape :
                                      SaturationProcessor::Type::Exciter);
            break;
    }

    float driveShape = std::pow(c, 0.9f);
    float maxDrive = (mode == Mode::RHYTHM ? 0.9f : mode == Mode::LEAD ? 1.f  : 0.4f);

    s.drive = juce::jlimit(0.f, 1.f, driveShape * maxDrive);

    float mixShape = std::pow(c, 0.6f);
    float mixBase, mixMax;

    if (mode == Mode::RHYTHM)      { mixBase = 0.f;   mixMax = 0.6f; }
    else if (mode == Mode::LEAD)   { mixBase = 0.1f; mixMax = 0.85f; }
    else                           { mixBase = 0.f;  mixMax = 0.35f; }

    if (c < 0.01f)
    {
        s.drive = 0.f;
        s.mix   = 0.f;
    }
    else
    {
        s.mix = juce::jlimit(0.f, 1.f, mixBase + mixShape * (mixMax - mixBase));
    }

    float toneSpan = (mode == Mode::RHYTHM ? 0.18f : mode == Mode::LEAD   ? 0.15f : 0.20f);

    s.tone = juce::jlimit(0.f, 1.f, 0.5f + sh * toneSpan);

    float biasRange = (mode == Mode::CLEAN ? 0.08f : 0.12f);
    s.bias = juce::jlimit(-0.25f, 0.25f, b * biasRange);

    return s;
}

// ==========================================================
// SPATIAL
// ==========================================================
SpatialProcessor::SpatialParameters ParameterMapper::mapSpatial(
    float body, float shimmer, float space, Mode mode)
{
    SpatialProcessor::SpatialParameters sp{};

    const float b   = musicalCurve(centreAround0(body));
    const float spc = juce::jlimit(0.f, 1.f, space);

    float maxRevMix,    maxChoMix, maxDlyMix, widthMax;
    float sizeMin,      sizeMax;
    float dlyBase,      dlySpan;
    float fbMin,        fbMax;

    switch (mode)
    {
        case Mode::RHYTHM:
            maxRevMix   = 0.35f;    maxChoMix   = 0.25f; maxDlyMix = 0.25f; widthMax = 1.10f;
            sizeMin     = 0.10f;    sizeMax     = 0.50f;
            dlyBase     = 260.f;    dlySpan     = 140.f;
            fbMin       = 0.15f;    fbMax       = 0.45f;
            break;

        case Mode::LEAD:
            maxRevMix   = 0.40f;    maxChoMix   = 0.35f; maxDlyMix = 0.30f; widthMax = 1.25f;
            sizeMin     = 0.25f;    sizeMax     = 0.80f;
            dlyBase     = 280.f;    dlySpan     = 180.f;
            fbMin       = 0.20f;    fbMax       = 0.55f;
            break;

        case Mode::CLEAN:
        default:
            maxRevMix   = 0.65f;    maxChoMix   = 0.50f; maxDlyMix = 0.40f; widthMax = 1.40f;
            sizeMin     = 0.40f;    sizeMax     = 1.00f;
            dlyBase     = 300.f;    dlySpan     = 200.f;
            fbMin       = 0.20f;    fbMax       = 0.60f;
            break;
    }

    float amt = spc;

    sp.reverbMix        = amt * maxRevMix;
    sp.reverbSize       = juce::jmap(amt, 0.f, 1.f, sizeMin, sizeMax);
    sp.reverbDamping    = juce::jlimit(0.f, 1.f, 0.55f + (-b)*0.25f);

    float dW = (mode == Mode::LEAD ? 0.8f : mode == Mode::CLEAN ? 0.7f : 0.6f);

    sp.delayMix = juce::jlimit(0.f, maxDlyMix, maxDlyMix * amt * dW);

    sp.delayTimeLeft  = dlyBase + amt * dlySpan;
    sp.delayTimeRight = sp.delayTimeLeft * 1.5f;
    sp.delayFeedback  = juce::jmap(amt, 0.f, 1.f, fbMin, fbMax);

    float choInfluence = amt;
    sp.chorusMix = maxChoMix * choInfluence;

    float minRate = 0.1f, maxRate  = 1.f;
    float minDepth= 0.2f, maxDepth = 0.6f;

    if (mode == Mode::CLEAN) { maxRate=1.2f; maxDepth=0.7f; }
    else if (mode == Mode::RHYTHM) { maxRate=0.9f; maxDepth=0.5f; }

    sp.chorusRate  = juce::jmap(amt, 0.f, 1.f, minRate,  maxRate);
    sp.chorusDepth = juce::jmap(amt, 0.f, 1.f, minDepth, maxDepth);

    sp.stereoWidth = juce::jmap(amt, 0.f, 1.f, 1.f, widthMax);

    return sp;
}

SaturationProcessor::Type ParameterMapper::selectSaturationType(
    float character, Mode mode)
{
    const float cNorm = std::pow(juce::jlimit(0.f, 1.f, character), 0.75f);

    switch (mode)
    {
        case Mode::RHYTHM:
            return (cNorm < 0.35f ? SaturationProcessor::Type::Tape :
                    cNorm < 0.70f ? SaturationProcessor::Type::Tube :
                                    SaturationProcessor::Type::Transistor);

        case Mode::LEAD:
            return (cNorm < 0.30f ? SaturationProcessor::Type::Tape :
                    cNorm < 0.70f ? SaturationProcessor::Type::Tube :
                                    SaturationProcessor::Type::Exciter);

        case Mode::CLEAN:
        default:
            return (cNorm < 0.50f ? SaturationProcessor::Type::Tape :
                                    SaturationProcessor::Type::Exciter);
    }
}
