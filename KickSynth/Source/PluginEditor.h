#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class KickSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    KickSynthAudioProcessorEditor (KickSynthAudioProcessor&);
    ~KickSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KickSynthAudioProcessor& audioProcessor;
    
    // Parameter controls
    juce::Slider bodyLevelSlider;
    juce::Slider clickLevelSlider;
    juce::Slider pitchStartHzSlider;
    juce::Slider pitchEndHzSlider;
    juce::Slider pitchTauMsSlider;
    juce::Slider attackMsSlider;
    juce::Slider t12MsSlider;
    juce::Slider t24MsSlider;
    juce::Slider tailMsSlider;
    juce::Slider clickDecayMsSlider;
    juce::Slider clickHPFHzSlider;
    juce::Slider velocitySensitivitySlider;
    juce::Slider driveSlider;
    juce::Slider asymmetrySlider;
    juce::Slider outputGainSlider;
    juce::Slider outputHPFHzSlider;
    juce::Slider keyTrackingSlider;
    juce::TextButton goButton;
    juce::TextButton loadParamsButton;
    juce::TextButton saveParamsButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    juce::ComboBox bodyOscTypeCombo;
    juce::ComboBox distortionTypeCombo;
    juce::ToggleButton retriggerModeButton;
    juce::ToggleButton limiterEnabledButton;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchStartHzAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchEndHzAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchTauMsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackMsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> t12MsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> t24MsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tailMsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickDecayMsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickHPFHzAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocitySensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> asymmetryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputHPFHzAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> keyTrackingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bodyOscTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distortionTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> retriggerModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterEnabledAttachment;
    
    // Labels
    juce::Label bodyLevelLabel;
    juce::Label clickLevelLabel;
    juce::Label pitchStartHzLabel;
    juce::Label pitchEndHzLabel;
    juce::Label pitchTauMsLabel;
    juce::Label attackMsLabel;
    juce::Label t12MsLabel;
    juce::Label t24MsLabel;
    juce::Label tailMsLabel;
    juce::Label clickDecayMsLabel;
    juce::Label clickHPFHzLabel;
    juce::Label velocitySensitivityLabel;
    juce::Label driveLabel;
    juce::Label asymmetryLabel;
    juce::Label outputGainLabel;
    juce::Label outputHPFHzLabel;
    juce::Label keyTrackingLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickSynthAudioProcessorEditor)
};

