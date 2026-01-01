#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class GabbermasterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Button::Listener
{
public:
    GabbermasterAudioProcessorEditor (GabbermasterAudioProcessor&);
    ~GabbermasterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

private:
    GabbermasterAudioProcessor& audioProcessor;

    // GO Button
    juce::TextButton goButton;
    juce::Slider noteSlider;
    juce::Label noteLabel;

    // Kick Mode
    juce::ComboBox kickModeBox;
    juce::Label kickModeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kickModeAttachment;

    // Envelope
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider, volumeSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel, volumeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    // Filter
    juce::ComboBox filterTypeBox;
    juce::Slider filterCutoffSlider, filterEnvSlider, filterQSlider, filterTrackSlider;
    juce::Label filterTypeLabel, filterCutoffLabel, filterEnvLabel, filterQLabel, filterTrackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterQAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterTrackAttachment;

    // Distortion
    juce::Slider distPreSlider, distPostSlider;
    juce::Label distPreLabel, distPostLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distPreAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distPostAttachment;

    // Reverb
    juce::ToggleButton reverbOnButton;
    juce::Slider reverbRoomSlider, reverbDampSlider, reverbWidthSlider, reverbMixSlider;
    juce::Label reverbOnLabel, reverbRoomLabel, reverbDampLabel, reverbWidthLabel, reverbMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbRoomAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;

    // EQ
    juce::ToggleButton eqOnButton;
    juce::ComboBox eqModeBox;
    juce::Label eqOnLabel, eqModeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> eqModeAttachment;

    // EQ Bands (simplified - just band 1 for space)
    juce::Slider eq1FreqSlider, eq1GainSlider, eq1QSlider;
    juce::Label eq1FreqLabel, eq1GainLabel, eq1QLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eq1FreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eq1GainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eq1QAttachment;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GabbermasterAudioProcessorEditor)
};
