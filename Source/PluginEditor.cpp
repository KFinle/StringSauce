#include "PluginEditor.h"

#include "ParameterID.hpp"
#include "SpriteKnob.hpp"
#include "ImageToggleButton.hpp"

using APVTS = juce::AudioProcessorValueTreeState;

StringSauceAudioProcessorEditor::StringSauceAudioProcessorEditor (StringSauceAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    audioProcessor.apvts.addParameterListener(ParamID::MODE, this);
    
    // load all PNG assets via BinaryData
    background = juce::ImageFileFormat::loadFrom(
        BinaryData::StringSauce_Background_png,
        BinaryData::StringSauce_Background_pngSize
    );

    modeOverlayRhythm = juce::ImageFileFormat::loadFrom(
        BinaryData::StringSauce_Button_Rhythm_png,
        BinaryData::StringSauce_Button_Rhythm_pngSize
    );

    modeOverlayLead = juce::ImageFileFormat::loadFrom(
        BinaryData::StringSauce_Button_Lead_png,
        BinaryData::StringSauce_Button_Lead_pngSize
    );

    modeOverlayClean = juce::ImageFileFormat::loadFrom(
        BinaryData::StringSauce_Button_Clean_png,
        BinaryData::StringSauce_Button_Clean_pngSize
    );

    // UI style + layout
    uiConfig.style = UIStyle::PNGLook;
    layout         = makeDefaultStringSauceLayout();

    // knobs
    static constexpr const char* knobParamIDs[6] =
    {
        ParamID::THUMP,
        ParamID::BODY,
        ParamID::CHARACTER,
        ParamID::SHIMMER,
        ParamID::SPANK,
        ParamID::SPACE
    };

    knobs.reserve(6);
    knobAttachments.reserve(6);

    for (int i = 0; i < 6; ++i)
    {
        auto knobComp   = UIFactory::createKnob(uiConfig.style);
        auto* knobSlider = dynamic_cast<juce::Slider*>(knobComp.get());
        jassert(knobSlider != nullptr);

        knobs.push_back(std::move(knobComp));
        addAndMakeVisible(*knobs.back());

        knobAttachments.push_back(std::make_unique<APVTS::SliderAttachment>(
            audioProcessor.apvts,
            knobParamIDs[i],
            *knobSlider
        ));
    }

    // mode buttons
    static constexpr const char* modeNames[3] = { "Rhythm", "Lead", "Clean" };

    modeButtons.reserve(3);

    for (int i = 0; i < 3; ++i)
    {
        auto btn    = UIFactory::createModeButton(uiConfig.style, modeNames[i]);
        auto* button = dynamic_cast<juce::Button*>(btn.get());
        jassert(button != nullptr);

        const int modeIndex = i;

        button->onClick = [this, modeIndex]()
        {
            auto* param = audioProcessor.apvts.getParameter(ParamID::MODE);
            jassert(param != nullptr);

            param->beginChangeGesture();
            const float normalized = param->convertTo0to1((float) modeIndex);
            param->setValueNotifyingHost(normalized);
            param->endChangeGesture();

            updateModeButtonStates();
            repaint();
        };

        modeButtons.push_back(std::move(btn));
        addAndMakeVisible(*modeButtons.back());
    }

    updateModeButtonStates();

    // debug Window Toggle
    if (debugModeEnabled)
    {
        addAndMakeVisible(debugButton);
        debugButton.setButtonText("Debug");
        debugButton.onClick = [this]()
        {
            if (!debugWindow)
            {
                debugWindow = std::make_unique<DebugWindow>(audioProcessor);
            }
            else
            {
                const bool show = !debugWindow->isVisible();
                debugWindow->setVisible(show);
                if (show) debugWindow->toFront(true);
            }
        };
    }

    // preset Bar
    startTimerHz(10);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(saveAsButton);
    addAndMakeVisible(deleteButton);
    populatePresetMenu();
    presetBox.onChange = [this]()
    {
        auto* pm = audioProcessor.presetManager.get();
        if (!pm) return;

        const int id = presetBox.getSelectedId();
        const int numFactory = (int)pm->getFactoryPresets().size();

       
        if (id >= 1 && id <= numFactory)
        {
            pm->loadFactoryPreset(id - 1);
            return;
        }

        if (id >= 1000)
        {
            int idx = id - 1000;
            const auto& files = pm->getUserPresetFiles();
            if (idx < (int)files.size()) pm->loadPresetFile(files[idx]);
        }

    };


    // save preset
    saveButton.onClick = [this]()
    {
        auto* pm = audioProcessor.presetManager.get();
        if (!pm) return;

        if (pm->saveCurrentPreset())
        {
            pm->reloadUserPresetList();
            populatePresetMenu();
        }
    };

    // save as
    saveAsButton.onClick = [this]()
    {
        auto* pm = audioProcessor.presetManager.get();
        if (!pm) return;

        auto presetDir = juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory)
            .getChildFile("StringSauce")
            .getChildFile("Presets");

        presetDir.createDirectory();

        auto chooser = std::make_shared<juce::FileChooser>(
            "Save preset...",
            presetDir,
            "*.stringsauce",
            true
        );

        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser, pm](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{}) return;

                if (!file.hasFileExtension("stringsauce"))
                    file = file.withFileExtension("stringsauce");

                if (pm->saveToFile(file))
                {
                    pm->reloadUserPresetList();
                    populatePresetMenu();
                }
            });
    };



    // delete current preset
    deleteButton.onClick = [this]()
    {
        auto* pm = audioProcessor.presetManager.get();
        if (!pm) return;

        auto name = presetBox.getText().trim();

        if (pm->deletePreset(name))
        {
            pm->reloadUserPresetList();
            populatePresetMenu();
        }
    };


    // aspect ratio
    const int bgW      = background.getWidth();
    const int bgH      = background.getHeight();
    const float aspect = (bgH > 0 ? (float) bgW / (float) bgH : 700.0f / 500.0f);

    int targetW = 700;
    int targetH = (int) (targetW / aspect);

    setSize(targetW, targetH);

    setResizable(true, true);
    if (auto* constrainer = getConstrainer()) constrainer->setFixedAspectRatio(aspect);
}

//==============================================================================

void StringSauceAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const int presetBarHeight = 60;
    bounds.removeFromTop(presetBarHeight);

    // paint background
    g.drawImage(background, bounds.toFloat());

    auto overlay = getCurrentModeOverlay();
    if (overlay.isValid()) g.drawImage(overlay, bounds.toFloat());

    // footer
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    auto footer = getLocalBounds().removeFromBottom(28);
    g.drawFittedText("FinleyAud.io", footer, juce::Justification::centred, 1);
}


//==============================================================================

void StringSauceAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // 1. PRESET BAR
    const int presetBarHeight = 60;
    auto presetBar = bounds.removeFromTop(presetBarHeight);

    // debug button
    if (debugModeEnabled)
    {
        auto debugArea = presetBar.removeFromLeft(80);
        debugButton.setBounds(debugArea.reduced(4));
    }

    // preset bar
    {
        auto area = presetBar.reduced(8);

        const int buttonW = 80;
        const int gap = 6;

        // place buttons on right
        auto deleteArea = area.removeFromRight(buttonW);
        deleteArea.reduce(gap, 0);
        deleteButton.setBounds(deleteArea);

        auto saveAsArea = area.removeFromRight(buttonW);
        saveAsArea.reduce(gap, 0);
        saveAsButton.setBounds(saveAsArea);

        auto saveArea = area.removeFromRight(buttonW);
        saveArea.reduce(gap, 0);
        saveButton.setBounds(saveArea);

        // remaining = preset selector
        presetBox.setBounds(area.reduced(gap, 0));
    }

    // 2. CONTENT AREA
    auto content = bounds.toFloat();

    auto place = [&](juce::Component& c, const UIArea& a)
    {
        c.setBounds(
            (int)(content.getX() + a.x * content.getWidth()),
            (int)(content.getY() + a.y * content.getHeight()),
            (int)(a.w * content.getWidth()),
            (int)(a.h * content.getHeight())
        );
    };

    // mode buttons
    for (int i = 0; i < modeButtons.size(); ++i)
        place(*modeButtons[i], layout.modeAreas[i]);

    // knobs
    for (int i = 0; i < knobs.size(); ++i)
        place(*knobs[i], layout.knobAreas[i]);
}


//==============================================================================

void StringSauceAudioProcessorEditor::updateModeButtonStates()
{
    auto* param = audioProcessor.apvts.getRawParameterValue(ParamID::MODE);
    jassert(param != nullptr);

    const int active = (int) param->load();

    for (int i = 0; i < (int) modeButtons.size(); ++i)
    {
        if (auto* btn = dynamic_cast<juce::Button*>(modeButtons[(size_t) i].get()))
            btn->setToggleState(i == active, juce::dontSendNotification);
    }
}

//==============================================================================

juce::Image StringSauceAudioProcessorEditor::getCurrentModeOverlay() const
{
    auto* param = audioProcessor.apvts.getRawParameterValue(ParamID::MODE);
    jassert(param != nullptr);

    const int m = (int) param->load();

    switch (m)
    {
        case 0: return modeOverlayRhythm;
        case 1: return modeOverlayLead;
        case 2: return modeOverlayClean;
        default: break;
    }

    return {};
}

// preset manager integration
void StringSauceAudioProcessorEditor::populatePresetMenu()
{
    presetBox.clear();

    auto* pm = audioProcessor.presetManager.get();
    if (!pm)
        return;

    // factory presets
    const auto& factory = pm->getFactoryPresets();
    for (int i = 0; i < (int)factory.size(); ++i)
        presetBox.addItem(factory[(size_t)i].name, i + 1);

    if (!factory.empty()) presetBox.addSeparator();

    // user presets
    const auto& userFiles = pm->getUserPresetFiles();
    for (int i = 0; i < (int)userFiles.size(); ++i)
        presetBox.addItem(
            userFiles[(size_t)i].getFileNameWithoutExtension(),
            1000 + i
        );

    // dirty indicator
    juce::String display = pm->getCurrentPresetName();
    if (pm->isCurrentPresetDirty()) display << " *";

    presetBox.setText(display, juce::dontSendNotification);
}


// DIRTY INDICATOR
void StringSauceAudioProcessorEditor::timerCallback()
{
    // 1) update preset name / dirty indicator
    if (auto* pm = audioProcessor.presetManager.get())
    {
        juce::String name = pm->getCurrentPresetName();
        if (pm->isCurrentPresetDirty()) name << " *";

        presetBox.setText(name, juce::dontSendNotification);
    }

    // 2) if mode changed, refresh mode buttons + overlay
    if (modeNeedsUIUpdate.exchange(false, std::memory_order_relaxed))
    {
        updateModeButtonStates();
        repaint();
    }
}

StringSauceAudioProcessorEditor::~StringSauceAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener(ParamID::MODE, this);
}
void StringSauceAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == ParamID::MODE)
    {
        modeNeedsUIUpdate.store(true, std::memory_order_relaxed);
    }
}

