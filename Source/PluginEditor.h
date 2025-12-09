#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UIConfig.hpp"
#include "UIFactory.hpp"
#include "UILayout.hpp"
#include "DebugWindow.hpp"


class StringSauceAudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer,
    public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit StringSauceAudioProcessorEditor (StringSauceAudioProcessor&);
    ~StringSauceAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    StringSauceAudioProcessor& audioProcessor;

    // UI style + layout
    UIConfig uiConfig;
    UILayout layout;

    // knobs + attachments
    std::vector<std::unique_ptr<juce::Component>> knobs;
    std::vector<std::unique_ptr<juce::Component>> modeButtons;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAttachments;

    // images
    juce::Image background;
    juce::Image modeOverlayRhythm;
    juce::Image modeOverlayLead;
    juce::Image modeOverlayClean;

    // debug window
    bool debugModeEnabled = true;
    std::unique_ptr<DebugWindow> debugWindow;
    juce::TextButton debugButton { "Debug" };

    // preset bar
    juce::ComboBox  presetBox;
    juce::TextButton saveButton   { "Save" };
    juce::TextButton saveAsButton { "Save As..." };
    juce::TextButton deleteButton { "Delete" };

    // helpers
    void updateModeButtonStates();
    juce::Image getCurrentModeOverlay() const;

    void populatePresetMenu();
    void timerCallback() override;

    std::atomic<bool> modeNeedsUIUpdate { false };

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StringSauceAudioProcessorEditor)
};
