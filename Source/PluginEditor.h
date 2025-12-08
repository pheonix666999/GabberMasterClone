#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class GabbermasterAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    GabbermasterAudioProcessorEditor (GabbermasterAudioProcessor&);
    ~GabbermasterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
private:
    void timerCallback() override;
    void loadCustomSample();
    
    //==============================================================================
    // Custom rotary slider with arc motion tracking
    class ArcMotionSlider : public juce::Slider
    {
    public:
        ArcMotionSlider() = default;

        void mouseDown(const juce::MouseEvent& event) override
        {
            sliderBeingDragged = true;
            updateValueFromMouse(event);
        }

        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (sliderBeingDragged)
                updateValueFromMouse(event);
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            sliderBeingDragged = false;
        }

    private:
        bool sliderBeingDragged = false;

        void updateValueFromMouse(const juce::MouseEvent& event)
        {
            auto bounds = getLocalBounds().toFloat();
            auto centre = bounds.getCentre();

            // Calculate angle from center to mouse position
            auto dx = event.position.x - centre.x;
            auto dy = event.position.y - centre.y;
            auto mouseAngle = std::atan2(dx, -dy);

            auto params = getRotaryParameters();
            auto startAngle = params.startAngleRadians;
            auto endAngle = params.endAngleRadians;

            // Normalize all angles to [0, 2π] range
            auto normalizeAngle = [](float angle) -> float
            {
                while (angle < 0.0f)
                    angle += juce::MathConstants<float>::twoPi;
                while (angle >= juce::MathConstants<float>::twoPi)
                    angle -= juce::MathConstants<float>::twoPi;
                return angle;
            };

            mouseAngle = normalizeAngle(mouseAngle);
            auto normStart = normalizeAngle(startAngle);
            auto normEnd = normalizeAngle(endAngle);

            // Handle the case where the range crosses 0
            if (normEnd < normStart)
            {
                // Range crosses 0 (e.g., from 300° to 60°)
                if (mouseAngle >= normStart)
                {
                    // Mouse is in the first part (after start)
                    auto proportion = (mouseAngle - normStart) /
                                     ((juce::MathConstants<float>::twoPi - normStart) + normEnd);
                    setValue(proportionOfLengthToValue(proportion));
                }
                else if (mouseAngle <= normEnd)
                {
                    // Mouse is in the second part (before end)
                    auto proportion = ((juce::MathConstants<float>::twoPi - normStart) + mouseAngle) /
                                     ((juce::MathConstants<float>::twoPi - normStart) + normEnd);
                    setValue(proportionOfLengthToValue(proportion));
                }
            }
            else
            {
                // Normal case - range doesn't cross 0
                auto clampedAngle = juce::jlimit(normStart, normEnd, mouseAngle);
                auto proportion = (clampedAngle - normStart) / (normEnd - normStart);
                setValue(proportionOfLengthToValue(proportion));
            }
        }
    };

    // Custom rotary slider look and feel
    class GabberKnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider) override;
    };

    GabberKnobLookAndFeel knobLookAndFeel;
    
    //==============================================================================
    GabbermasterAudioProcessor& audioProcessor;
    
    // Preset selector
    juce::ComboBox presetSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;
    
    // Custom sample loader
    juce::TextButton loadSampleButton;
    juce::Label sampleNameLabel;
    
    // Top row - Envelope & Volume section (7 knobs)
    ArcMotionSlider attackKnob;
    ArcMotionSlider decayKnob;
    ArcMotionSlider releaseKnob;
    ArcMotionSlider sustainKnob;
    ArcMotionSlider volumeKnob;
    ArcMotionSlider preKnob;
    ArcMotionSlider postKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> postAttachment;

    // Middle row - Filter section (4 knobs)
    ArcMotionSlider cutoffKnob;
    ArcMotionSlider qKnob;
    ArcMotionSlider trackKnob;
    ArcMotionSlider envKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAttachment;

    // Bottom row - Effects section (4 knobs)
    ArcMotionSlider roomKnob;
    ArcMotionSlider widthKnob;
    ArcMotionSlider dampKnob;
    ArcMotionSlider mixKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Filter type selector
    juce::ComboBox filterTypeSelector;

    // Additional controls
    juce::TextButton goButton;
    juce::TextButton fileButton;
    juce::Label sampleNumberLabel;
    juce::TextEditor sampleNameField;

    // Radio buttons for SLOW/FAST
    juce::ToggleButton slowButton;
    juce::ToggleButton fastButton;

    // Toggle buttons for OFF/ON
    juce::ToggleButton offButton;
    juce::ToggleButton onButton;
    
    // Labels
    std::vector<std::unique_ptr<juce::Label>> labels;
    
    // Helper methods
    void setupRotarySlider(juce::Slider& slider, const juce::String& suffix = "");
    void addLabel(const juce::String& text, juce::Component& component);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GabbermasterAudioProcessorEditor)
};