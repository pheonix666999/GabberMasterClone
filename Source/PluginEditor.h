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
    // EQ Curve Display Component
    class EQCurveComponent : public juce::Component
    {
    public:
        EQCurveComponent(GabbermasterAudioProcessor& p) : processor(p) {}

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(2);

            // Dark background
            g.setColour(juce::Colour(0xff0a0a0a));
            g.fillRoundedRectangle(bounds, 4.0f);

            // Grid lines
            g.setColour(juce::Colour(0xff222222));

            // Horizontal grid (dB levels)
            for (int i = 1; i < 6; ++i)
            {
                float y = bounds.getY() + i * bounds.getHeight() / 6.0f;
                g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
            }

            // Vertical grid (frequency)
            std::array<float, 8> freqMarkers = {50, 100, 200, 500, 1000, 2000, 5000, 10000};
            for (float freq : freqMarkers)
            {
                float x = frequencyToX(freq, bounds);
                g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
            }

            // Draw EQ curve
            juce::Path curvePath;
            bool pathStarted = false;

            for (int x = 0; x < static_cast<int>(bounds.getWidth()); ++x)
            {
                float freq = xToFrequency(bounds.getX() + x, bounds);
                float magnitude = processor.getEQ().getMagnitudeForFrequency(freq);
                float dB = juce::Decibels::gainToDecibels(magnitude);
                dB = juce::jlimit(-18.0f, 18.0f, dB);

                float y = dBToY(dB, bounds);

                if (!pathStarted)
                {
                    curvePath.startNewSubPath(bounds.getX() + x, y);
                    pathStarted = true;
                }
                else
                {
                    curvePath.lineTo(bounds.getX() + x, y);
                }
            }

            // Fill under curve with gradient
            juce::Path fillPath = curvePath;
            fillPath.lineTo(bounds.getRight(), bounds.getCentreY());
            fillPath.lineTo(bounds.getX(), bounds.getCentreY());
            fillPath.closeSubPath();

            juce::ColourGradient fillGradient(
                juce::Colour(0x40cc0000), bounds.getCentreX(), bounds.getY(),
                juce::Colour(0x10cc0000), bounds.getCentreX(), bounds.getCentreY(),
                false);
            g.setGradientFill(fillGradient);
            g.fillPath(fillPath);

            // Draw the curve line
            g.setColour(juce::Colour(0xffcc0000));
            g.strokePath(curvePath, juce::PathStrokeType(2.0f));

            // Draw band control points
            auto& eq = processor.getEQ();
            for (int band = 0; band < ParametricEQ::numBands; ++band)
            {
                float freq = eq.getFrequency(band);
                float gain = eq.getGain(band);

                float x = frequencyToX(freq, bounds);
                float y = dBToY(gain, bounds);

                // Glow effect
                g.setColour(juce::Colour(0x40ff6666));
                g.fillEllipse(x - 8, y - 8, 16, 16);

                // Control point
                g.setColour(juce::Colour(0xffcc0000));
                g.fillEllipse(x - 5, y - 5, 10, 10);
                g.setColour(juce::Colours::white);
                g.drawEllipse(x - 5, y - 5, 10, 10, 1.0f);
            }

            // Border
            g.setColour(juce::Colour(0xff444444));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

            // Frequency labels
            g.setColour(juce::Colour(0xff888888));
            g.setFont(9.0f);
            g.drawText("50", frequencyToX(50, bounds) - 10, bounds.getBottom() - 12, 20, 10, juce::Justification::centred);
            g.drawText("100", frequencyToX(100, bounds) - 12, bounds.getBottom() - 12, 24, 10, juce::Justification::centred);
            g.drawText("1k", frequencyToX(1000, bounds) - 8, bounds.getBottom() - 12, 16, 10, juce::Justification::centred);
            g.drawText("10k", frequencyToX(10000, bounds) - 12, bounds.getBottom() - 12, 24, 10, juce::Justification::centred);
        }

        void mouseDown(const juce::MouseEvent& e) override { updateEQFromMouse(e); }
        void mouseDrag(const juce::MouseEvent& e) override { updateEQFromMouse(e); }

    private:
        GabbermasterAudioProcessor& processor;

        float frequencyToX(float freq, juce::Rectangle<float> bounds) const
        {
            float minLog = std::log10(20.0f);
            float maxLog = std::log10(20000.0f);
            float freqLog = std::log10(freq);
            return bounds.getX() + (freqLog - minLog) / (maxLog - minLog) * bounds.getWidth();
        }

        float xToFrequency(float x, juce::Rectangle<float> bounds) const
        {
            float minLog = std::log10(20.0f);
            float maxLog = std::log10(20000.0f);
            float proportion = (x - bounds.getX()) / bounds.getWidth();
            return std::pow(10.0f, minLog + proportion * (maxLog - minLog));
        }

        float dBToY(float dB, juce::Rectangle<float> bounds) const
        {
            float proportion = (18.0f - dB) / 36.0f;
            return bounds.getY() + proportion * bounds.getHeight();
        }

        float yTodB(float y, juce::Rectangle<float> bounds) const
        {
            float proportion = (y - bounds.getY()) / bounds.getHeight();
            return 18.0f - proportion * 36.0f;
        }

        void updateEQFromMouse(const juce::MouseEvent& e)
        {
            auto bounds = getLocalBounds().toFloat().reduced(2);
            float freq = xToFrequency(static_cast<float>(e.x), bounds);
            float dB = yTodB(static_cast<float>(e.y), bounds);

            // Find closest band
            auto& eq = processor.getEQ();
            int closestBand = 0;
            float closestDist = std::abs(std::log10(freq) - std::log10(eq.getFrequency(0)));

            for (int band = 1; band < ParametricEQ::numBands; ++band)
            {
                float dist = std::abs(std::log10(freq) - std::log10(eq.getFrequency(band)));
                if (dist < closestDist)
                {
                    closestDist = dist;
                    closestBand = band;
                }
            }

            // Update the closest band's gain
            juce::String gainId = "eq" + juce::String(closestBand + 1) + "Gain";
            if (auto* param = processor.getAPVTS().getParameter(gainId))
            {
                float normValue = param->getNormalisableRange().convertTo0to1(dB);
                param->setValueNotifyingHost(normValue);
            }

            repaint();
        }
    };

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

    // EQ Curve display
    std::unique_ptr<EQCurveComponent> eqCurve;

    // Layer controls - Sub
    ArcMotionSlider subVolKnob;
    ArcMotionSlider subPitchKnob;
    ArcMotionSlider subDecayKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subVolAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subDecayAttachment;

    // Layer controls - Body
    ArcMotionSlider bodyVolKnob;
    ArcMotionSlider bodyPitchKnob;
    ArcMotionSlider bodyDecayKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyVolAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyDecayAttachment;

    // Layer controls - Click/Transient
    ArcMotionSlider clickVolKnob;
    ArcMotionSlider clickPitchKnob;
    ArcMotionSlider clickDecayKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickVolAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clickDecayAttachment;

    // Labels
    std::vector<std::unique_ptr<juce::Label>> labels;
    
    // Helper methods
    void setupRotarySlider(juce::Slider& slider, const juce::String& suffix = "");
    void addLabel(const juce::String& text, juce::Component& component);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GabbermasterAudioProcessorEditor)
};