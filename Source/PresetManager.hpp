//  PresetManager.hpp
//  StringSauce
//
//  This class is where we allow for the creation of
//  user-defined and developer-defined presets.

#pragma once
#include <JuceHeader.h>

class PresetManager : private juce::ValueTree::Listener
{
public:
    static constexpr const char* presetExtension = "stringsauce";

    PresetManager(juce::AudioProcessorValueTreeState& stateToUse, const juce::String& pluginName) :
          apvts(stateToUse),
          pluginName(pluginName),
          presetDir(makePresetDirectory(pluginName))
    {
        if (!presetDir.exists()) presetDir.createDirectory();

        apvts.state.addListener(this);
        reloadUserPresetList();
    }

    struct FactoryPreset
    {
        juce::String name;
        juce::ValueTree state;
    };

    const std::vector<FactoryPreset>& getFactoryPresets() const
    {
        return factoryPresets;
    }

    // Create APVTS state
    juce::ValueTree makeFactoryPresetState(
        std::initializer_list<std::pair<const char*, float>> values)
    {
        suppressDirtyFlag = true;

        auto backup = apvts.copyState();

        // Apply parameter values
        for (auto& pair : values)
        {
            auto* param = apvts.getParameter(pair.first);

            if (param != nullptr)
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(pair.second);
                param->endChangeGesture();
            }
            else
            {
                DBG("PresetManager: Missing parameter ID in factory preset -> " << pair.first);
            }
        }

        auto presetState = apvts.copyState();
        apvts.replaceState(backup);

        suppressDirtyFlag = false;
        return presetState;
    }

    void addFactoryPreset(const juce::String& name, std::initializer_list<std::pair<const char*, float>> values)
    {
        factoryPresets.push_back({ name, makeFactoryPresetState(values) });
    }

    // USER PRESETS
    void reloadUserPresetList()
    {
        userFiles.clear();

        juce::Array<juce::File> found;
        presetDir.findChildFiles(found, juce::File::findFiles, false, "*." + juce::String(presetExtension));

        for (auto& f : found)
            userFiles.push_back(f);
    }

    const std::vector<juce::File>& getUserPresetFiles() const
    {
        return userFiles;
    }

    bool loadFactoryPreset(int index)
    {
        if (index < 0 || index >= (int)factoryPresets.size()) return false;

        suppressDirtyFlag = true;
        apvts.replaceState(factoryPresets[(size_t)index].state.createCopy());
        suppressDirtyFlag = false;

        currentPresetName = factoryPresets[(size_t)index].name;
        isDirty = false;
        return true;
    }

    // Load user preset
    bool loadPresetFile(const juce::File& file)
    {
        juce::XmlDocument doc(file);
        auto xml = doc.getDocumentElement();

        if (!xml) return false;

        auto vt = juce::ValueTree::fromXml(*xml);
        if (!vt.isValid()) return false;

        suppressDirtyFlag = true;
        apvts.replaceState(vt);
        suppressDirtyFlag = false;

        currentPresetName = file.getFileNameWithoutExtension();
        isDirty = false;
        return true;
    }

    // Save current preset
    bool saveCurrentPreset()
    {
        if (currentPresetName.isEmpty()) return true;

        suppressDirtyFlag = true;

        auto file = presetDir.getChildFile(currentPresetName + "." + presetExtension);

        bool ok = writePreset(file);

        suppressDirtyFlag = false;

        if (ok) isDirty = false;

        return ok;
    }

    // Save preset as new file
    bool saveToFile(const juce::File& file)
    {
        suppressDirtyFlag = true;
        bool ok = writePreset(file);
        suppressDirtyFlag = false;

        if (ok)
        {
            currentPresetName = file.getFileNameWithoutExtension();
            isDirty = false;
        }

        return ok;
    }

    // Delete preset by name
    bool deletePreset(const juce::String& name)
    {
        auto file = presetDir.getChildFile(name + "." + presetExtension);

        if (file.existsAsFile()) return file.deleteFile();

        return false;
    }

    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    bool isCurrentPresetDirty() const { return isDirty; }

private:
    // Data
    juce::AudioProcessorValueTreeState& apvts;
    juce::String pluginName;
    juce::File presetDir;

    std::vector<FactoryPreset> factoryPresets;
    std::vector<juce::File> userFiles;

    juce::String currentPresetName = "Unsaved Preset";
    bool isDirty = false;

    std::atomic<bool> suppressDirtyFlag { false };

    // Helpers
    static juce::File makePresetDirectory(const juce::String& pluginName)
    {
        auto docs = juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory);

        return docs.getChildFile(pluginName).getChildFile("Presets");
    }

    bool writePreset(const juce::File& file)
    {
        if (auto xml = apvts.copyState().createXml()) return xml->writeTo(file);

        return false;
    }

    // Dirty tracking
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        if (!suppressDirtyFlag) isDirty = true;
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override
    {
        if (!suppressDirtyFlag) isDirty = true;
    }

    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
    {
        if (!suppressDirtyFlag) isDirty = true;
    }

    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override
    {
        if (!suppressDirtyFlag) isDirty = true;
    }

    void valueTreeParentChanged(juce::ValueTree&) override
    {
        if (!suppressDirtyFlag) isDirty = true;
    }
};
