#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    void logMessage(const juce::String& message)
    {
        auto logFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("gabbermaster_log.txt");
        auto timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
        logFile.appendText(timestamp + " " + message + "\n");
    }
}

//==============================================================================
GabbermasterAudioProcessorEditor::GabbermasterAudioProcessorEditor (GabbermasterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logMessage("Editor ctor start");

    // Preset selector (Fag Tag dropdown)
    presetSelector.addItem("Fag Tag", 1);
    for (int i = 2; i <= 48; i++)
        presetSelector.addItem("Preset " + juce::String(i), i);
    presetSelector.setSelectedId(1);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetSelector.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
    addAndMakeVisible(presetSelector);
    presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "preset", presetSelector);
    logMessage("Editor: preset selector wired");

    // GO! button
    goButton.setButtonText("GO!");
    goButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc0000));
    goButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(goButton);

    // Sample number label
    sampleNumberLabel.setText("1/48", juce::dontSendNotification);
    sampleNumberLabel.setJustificationType(juce::Justification::centred);
    sampleNumberLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    sampleNumberLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a1a1a));
    sampleNumberLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(sampleNumberLabel);

    // Sample name field
    sampleNameField.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
    sampleNameField.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    sampleNameField.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff444444));
    sampleNameField.setText("name");
    sampleNameField.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(sampleNameField);

    // File button
    fileButton.setButtonText("File");
    fileButton.onClick = [this] { loadCustomSample(); };
    fileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    fileButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(fileButton);

    // Filter type selector (HI dropdown)
    filterTypeSelector.addItem("HI", 1);
    filterTypeSelector.addItem("LO", 2);
    filterTypeSelector.addItem("BP", 3);
    filterTypeSelector.setSelectedId(2); // Default to LO (Low Pass)
    filterTypeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    filterTypeSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
    filterTypeSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    filterTypeSelector.onChange = [this]() {
        // 0=HI (High Pass), 1=LO (Low Pass), 2=BP (Band Pass)
        audioProcessor.setFilterType(filterTypeSelector.getSelectedId() - 1);
    };
    addAndMakeVisible(filterTypeSelector);
    logMessage("Editor: filter type selector ready");

    // SLOW/FAST radio buttons
    slowButton.setButtonText("SLOW");
    slowButton.setRadioGroupId(1);
    slowButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    slowButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc0000));
    slowButton.onClick = [this]() {
        audioProcessor.setSlowMode(true);
        fastButton.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(slowButton);

    fastButton.setButtonText("FAST");
    fastButton.setRadioGroupId(1);
    fastButton.setToggleState(true, juce::dontSendNotification);
    fastButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    fastButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc0000));
    fastButton.onClick = [this]() {
        audioProcessor.setSlowMode(false);
        slowButton.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(fastButton);
    logMessage("Editor: slow/fast buttons ready");

    // OFF/ON toggle buttons
    offButton.setButtonText("OFF");
    offButton.setRadioGroupId(2);
    offButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    offButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc0000));
    offButton.onClick = [this]() {
        audioProcessor.setEffectsBypassed(true);
        onButton.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(offButton);

    onButton.setButtonText("ON");
    onButton.setRadioGroupId(2);
    onButton.setToggleState(true, juce::dontSendNotification);
    onButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc0000));
    onButton.onClick = [this]() {
        audioProcessor.setEffectsBypassed(false);
        offButton.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(onButton);
    logMessage("Editor: on/off buttons ready");

    // Kick mode buttons (Viper, Evil, Hard, Soft, Raw, Metal)
    auto setupKickModeButton = [this](juce::ToggleButton& button, const juce::String& name, int modeIndex) {
        button.setButtonText(name);
        button.setRadioGroupId(100); // Separate radio group for kick modes
        button.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        button.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffcc0000));
        button.onClick = [this, modeIndex, &button]() {
            if (auto* param = audioProcessor.getAPVTS().getParameter("kickMode"))
            {
                param->setValueNotifyingHost(static_cast<float>(modeIndex) / 5.0f);
            }
            // Update all buttons
            viperButton.setToggleState(modeIndex == 0, juce::dontSendNotification);
            evilButton.setToggleState(modeIndex == 1, juce::dontSendNotification);
            hardButton.setToggleState(modeIndex == 2, juce::dontSendNotification);
            softButton.setToggleState(modeIndex == 3, juce::dontSendNotification);
            rawButton.setToggleState(modeIndex == 4, juce::dontSendNotification);
            metalButton.setToggleState(modeIndex == 5, juce::dontSendNotification);
        };
        addAndMakeVisible(button);
    };

    setupKickModeButton(viperButton, "Viper", 0);
    setupKickModeButton(evilButton, "Evil", 1);
    setupKickModeButton(hardButton, "Hard", 2);
    setupKickModeButton(softButton, "Soft", 3);
    setupKickModeButton(rawButton, "Raw", 4);
    setupKickModeButton(metalButton, "Metal", 5);
    viperButton.setToggleState(true, juce::dontSendNotification); // Default to Viper
    logMessage("Editor: kick mode buttons ready");

    // Row 1 - Envelope section (5 knobs): Attack, Decay, Release, Sustain, Volume
    setupRotarySlider(attackKnob, " ms");
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "attack", attackKnob);

    setupRotarySlider(decayKnob, " ms");
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "decay", decayKnob);

    setupRotarySlider(releaseKnob, " ms");
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "release", releaseKnob);

    setupRotarySlider(sustainKnob, "");
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "sustain", sustainKnob);

    setupRotarySlider(volumeKnob, " dB");
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "output", volumeKnob);

    // Pre/Post distortion knobs
    setupRotarySlider(preKnob, "");
    preAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "drive", preKnob);

    setupRotarySlider(postKnob, "");
    postAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "saturate", postKnob);
    logMessage("Editor: row1 knobs wired");

    // Row 2 - Filter section (4 knobs): Cutoff, Q, Track, Env
    setupRotarySlider(cutoffKnob, " Hz");
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "lpf", cutoffKnob);

    setupRotarySlider(qKnob, "");
    qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "resonance", qKnob);

    setupRotarySlider(trackKnob, "%");
    trackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "track", trackKnob);

    setupRotarySlider(envKnob, "");
    envAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "filterEnv", envKnob);
    logMessage("Editor: row2 knobs wired");

    // Row 3 - Effects section (4 knobs): Room, Width, Damp, Mix
    setupRotarySlider(roomKnob, "");
    roomAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "room", roomKnob);

    setupRotarySlider(widthKnob, "");
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "width", widthKnob);

    setupRotarySlider(dampKnob, "");
    dampAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "damp", dampKnob);

    setupRotarySlider(mixKnob, "%");
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixKnob);
    logMessage("Editor: row3 knobs wired");

    // EQ Curve display
    eqCurve = std::make_unique<EQCurveComponent>(audioProcessor);
    addAndMakeVisible(eqCurve.get());
    logMessage("Editor: EQ curve created");

    // Layer controls - Sub
    setupRotarySlider(subVolKnob, " dB");
    subVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "subVol", subVolKnob);

    setupRotarySlider(subPitchKnob, " st");
    subPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "subPitch", subPitchKnob);

    setupRotarySlider(subDecayKnob, " ms");
    subDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "subDecay", subDecayKnob);

    // Layer controls - Body
    setupRotarySlider(bodyVolKnob, " dB");
    bodyVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "bodyVol", bodyVolKnob);

    setupRotarySlider(bodyPitchKnob, " st");
    bodyPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "bodyPitch", bodyPitchKnob);

    setupRotarySlider(bodyDecayKnob, " ms");
    bodyDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "bodyDecay", bodyDecayKnob);

    // Layer controls - Click
    setupRotarySlider(clickVolKnob, " dB");
    clickVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickVol", clickVolKnob);

    setupRotarySlider(clickPitchKnob, " st");
    clickPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickPitch", clickPitchKnob);

    setupRotarySlider(clickDecayKnob, " ms");
    clickDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickDecay", clickDecayKnob);

    setSize (850, 580);
    logMessage("Editor: after setSize");
    startTimer(50);
    logMessage("Editor ctor end");
}

GabbermasterAudioProcessorEditor::~GabbermasterAudioProcessorEditor()
{
}

//==============================================================================
void GabbermasterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Black background
    g.fillAll(juce::Colours::black);

    // Subtle red gradient overlay
    juce::ColourGradient redGlow(juce::Colour(0x00000000), 0.0f, 0.0f,
                                  juce::Colour(0x30600000), static_cast<float>(getWidth()), static_cast<float>(getHeight()), true);
    g.setGradientFill(redGlow);
    g.fillAll();

    // Waveform display area (top left)
    auto waveformBounds = juce::Rectangle<int>(12, 12, 180, 70);
    g.setColour(juce::Colour(0xff080808));
    g.fillRect(waveformBounds);
    g.setColour(juce::Colour(0xff333333));
    g.drawRect(waveformBounds, 1);

    // Draw waveform (orange/red color like original)
    g.setColour(juce::Colour(0xffcc4400));
    auto centerY = static_cast<float>(waveformBounds.getCentreY());

    // Get current sample from processor
    int presetIndex = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("preset")->load());
    auto* sample = audioProcessor.getSampleManager().getSample(presetIndex);

    if (sample && sample->getNumSamples() > 0)
    {
        // Draw actual sample waveform
        int numSamples = sample->getNumSamples();
        float samplesPerPixel = static_cast<float>(numSamples) / static_cast<float>(waveformBounds.getWidth());

        for (int x = 0; x < waveformBounds.getWidth(); x += 1)
        {
            int startSample = static_cast<int>(x * samplesPerPixel);
            int endSample = static_cast<int>((x + 1) * samplesPerPixel);
            endSample = juce::jmin(endSample, numSamples - 1);

            float minVal = 0.0f, maxVal = 0.0f;
            for (int s = startSample; s <= endSample; ++s)
            {
                float sampleVal = sample->getSample(0, s);
                if (sampleVal < minVal) minVal = sampleVal;
                if (sampleVal > maxVal) maxVal = sampleVal;
            }

            float y1 = centerY + minVal * 30.0f;
            float y2 = centerY + maxVal * 30.0f;
            g.drawLine(static_cast<float>(waveformBounds.getX() + x), y1,
                       static_cast<float>(waveformBounds.getX() + x), y2, 1.0f);
        }
    }
    else
    {
        // Fallback: Draw placeholder sine wave
        for (int x = 0; x < waveformBounds.getWidth(); x += 1)
        {
            float wave = std::sin(x * 0.12f) * std::sin(x * 0.025f) * 22.0f;
            float y1 = centerY - std::abs(wave);
            float y2 = centerY + std::abs(wave);
            g.drawLine(static_cast<float>(waveformBounds.getX() + x), y1,
                       static_cast<float>(waveformBounds.getX() + x), y2, 1.0f);
        }
    }

    // "KICK MODE" label above kick mode buttons
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("KICK MODE", 550, 58, 120, 14, juce::Justification::left);

    // "GABBER MASTER!" title - red gothic style
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::FontOptions(44.0f, juce::Font::bold));
    g.drawText("GABBER MASTER!", 195, 8, 350, 60, juce::Justification::centred);

    // Envelope display area (bottom right) - larger area
    auto envBounds = juce::Rectangle<int>(360, 220, 375, 145);
    g.setColour(juce::Colour(0xff080808));
    g.fillRect(envBounds);
    g.setColour(juce::Colour(0xff333333));
    g.drawRect(envBounds, 1);

    // Draw red circular background pattern
    g.setColour(juce::Colour(0xff300000));
    for (int i = 1; i <= 5; i++)
    {
        float radius = static_cast<float>(i * 28);
        g.drawEllipse(static_cast<float>(envBounds.getCentreX()) - radius,
                      static_cast<float>(envBounds.getCentreY()) - radius,
                      radius * 2.0f, radius * 2.0f, 0.5f);
    }

    // Draw ADSR envelope curve (red) - Dynamic based on current parameters
    g.setColour(juce::Colour(0xffcc0000));

    // Get current ADSR values
    float attack = audioProcessor.getAPVTS().getRawParameterValue("attack")->load();
    float decay = audioProcessor.getAPVTS().getRawParameterValue("decay")->load();
    float sustain = audioProcessor.getAPVTS().getRawParameterValue("sustain")->load();
    float release = audioProcessor.getAPVTS().getRawParameterValue("release")->load();

    // Normalize values for display (attack/decay/release are in ms, max ~5000ms)
    float attackNorm = juce::jlimit(0.0f, 1.0f, attack / 1000.0f);
    float decayNorm = juce::jlimit(0.0f, 1.0f, decay / 2000.0f);
    float releaseNorm = juce::jlimit(0.0f, 1.0f, release / 2000.0f);

    // Calculate envelope display points
    float envWidth = static_cast<float>(envBounds.getWidth()) - 30.0f;
    float envHeight = static_cast<float>(envBounds.getHeight()) - 40.0f;
    float startX = static_cast<float>(envBounds.getX()) + 15.0f;
    float startY = static_cast<float>(envBounds.getBottom()) - 20.0f;
    float topY = static_cast<float>(envBounds.getY()) + 20.0f;

    // Attack point (proportional width based on attack time)
    float attackX = startX + attackNorm * envWidth * 0.25f;
    float attackY = topY;

    // Decay point (after attack, width based on decay time)
    float decayX = attackX + decayNorm * envWidth * 0.25f;
    float decayY = topY + (1.0f - sustain) * envHeight;

    // Sustain end point (sustain level maintained until release)
    float sustainEndX = decayX + envWidth * 0.3f;
    float sustainY = decayY;

    // Release point (back to zero)
    float releaseX = sustainEndX + releaseNorm * envWidth * 0.2f;
    releaseX = juce::jmin(releaseX, static_cast<float>(envBounds.getRight()) - 15.0f);

    juce::Path envPath;
    envPath.startNewSubPath(startX, startY);
    envPath.lineTo(attackX, attackY);   // Attack
    envPath.lineTo(decayX, sustainY);   // Decay to sustain level
    envPath.lineTo(sustainEndX, sustainY);  // Sustain
    envPath.lineTo(releaseX, startY);   // Release
    g.strokePath(envPath, juce::PathStrokeType(3.0f));

    // Draw envelope activity indicator (glow effect when playing)
    float envValue = audioProcessor.getEnvelopeValue();
    if (envValue > 0.01f)
    {
        g.setColour(juce::Colour(0xffff3333).withAlpha(envValue * 0.5f));
        g.strokePath(envPath, juce::PathStrokeType(6.0f));
    }

    // Knob labels
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f));

    // Row 1 labels
    int row1LabelY = 150;
    int knobSize = 55;
    g.drawText("Attack", 15, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Decay", 75, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Release", 135, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Sustain", 195, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Volume", 255, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Pre", 340, row1LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Post", 405, row1LabelY, knobSize, 14, juce::Justification::centred);

    // Row 2 labels
    int row2LabelY = 225;
    g.drawText("Cutoff", 15, row2LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Q", 75, row2LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Track", 135, row2LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Env", 195, row2LabelY, knobSize, 14, juce::Justification::centred);

    // Row 3 labels
    int row3LabelY = 300;
    g.drawText("Room", 15, row3LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Width", 75, row3LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Damp", 135, row3LabelY, knobSize, 14, juce::Justification::centred);
    g.drawText("Mix", 195, row3LabelY, knobSize, 14, juce::Justification::centred);

    // OFF/ON labels under Width/Damp
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("OFF", 75, row3LabelY + 14, knobSize, 12, juce::Justification::centred);
    g.drawText("ON", 135, row3LabelY + 14, knobSize, 12, juce::Justification::centred);

    // EQ Section label
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("EQ CURVE", 360, 300, 100, 18, juce::Justification::left);

    // Layer controls section
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("LAYERS", 15, 360, 80, 16, juce::Justification::left);

    // Layer section labels
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(10.0f));

    int layerY = 380;
    int layerKnobSize = 45;
    int layerSpacing = 52;

    // Sub layer labels
    g.setColour(juce::Colour(0xff00cc00)); // Green for sub
    g.drawText("SUB", 15, layerY - 14, 50, 12, juce::Justification::left);
    g.setColour(juce::Colours::white);
    g.drawText("Vol", 15, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Pitch", 15 + layerSpacing, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Decay", 15 + layerSpacing * 2, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);

    // Body layer labels
    g.setColour(juce::Colour(0xffcccc00)); // Yellow for body
    g.drawText("BODY", 15, layerY + 70 - 14, 50, 12, juce::Justification::left);
    g.setColour(juce::Colours::white);
    g.drawText("Vol", 15, layerY + 70 + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Pitch", 15 + layerSpacing, layerY + 70 + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Decay", 15 + layerSpacing * 2, layerY + 70 + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);

    // Click layer labels
    g.setColour(juce::Colour(0xffcc6600)); // Orange for click
    g.drawText("CLICK", 185, layerY - 14, 50, 12, juce::Justification::left);
    g.setColour(juce::Colours::white);
    g.drawText("Vol", 185, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Pitch", 185 + layerSpacing, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);
    g.drawText("Decay", 185 + layerSpacing * 2, layerY + layerKnobSize, layerKnobSize, 12, juce::Justification::centred);

    // Website watermark
    g.setColour(juce::Colour(0xffcc0000));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("IHATEBREAKCORE.COM", getWidth() - 170, getHeight() - 22, 165, 18, juce::Justification::right);
}

void GabbermasterAudioProcessorEditor::resized()
{
    // GO! button (top right)
    goButton.setBounds(690, 12, 50, 28);

    // Right side controls
    presetSelector.setBounds(570, 12, 110, 24);
    sampleNumberLabel.setBounds(570, 42, 50, 22);
    sampleNameField.setBounds(625, 42, 115, 22);
    fileButton.setBounds(570, 70, 60, 24);

    // Filter type selector (HI dropdown)
    filterTypeSelector.setBounds(270, 170, 55, 22);

    // Knob layout
    int knobSize = 55;

    // Row 1 - 5 envelope knobs + 2 pre/post knobs
    int row1Y = 90;
    attackKnob.setBounds(15, row1Y, knobSize, knobSize);
    decayKnob.setBounds(75, row1Y, knobSize, knobSize);
    releaseKnob.setBounds(135, row1Y, knobSize, knobSize);
    sustainKnob.setBounds(195, row1Y, knobSize, knobSize);
    volumeKnob.setBounds(255, row1Y, knobSize, knobSize);

    preKnob.setBounds(340, row1Y, knobSize, knobSize);
    postKnob.setBounds(405, row1Y, knobSize, knobSize);

    // Row 2 - 4 filter knobs
    int row2Y = 165;
    cutoffKnob.setBounds(15, row2Y, knobSize, knobSize);
    qKnob.setBounds(75, row2Y, knobSize, knobSize);
    trackKnob.setBounds(135, row2Y, knobSize, knobSize);
    envKnob.setBounds(195, row2Y, knobSize, knobSize);

    // Row 3 - 4 effect knobs
    int row3Y = 240;
    roomKnob.setBounds(15, row3Y, knobSize, knobSize);
    widthKnob.setBounds(75, row3Y, knobSize, knobSize);
    dampKnob.setBounds(135, row3Y, knobSize, knobSize);
    mixKnob.setBounds(195, row3Y, knobSize, knobSize);

    // SLOW/FAST radio buttons
    slowButton.setBounds(15, 540, 70, 24);
    fastButton.setBounds(90, 540, 70, 24);

    // OFF/ON toggle buttons
    offButton.setBounds(180, 540, 60, 24);
    onButton.setBounds(250, 540, 60, 24);

    // Kick mode buttons (top row, after title)
    int kickModeY = 75;
    int kickModeX = 550;
    int kickBtnWidth = 55;
    int kickBtnHeight = 20;
    int kickBtnSpacing = 4;
    viperButton.setBounds(kickModeX, kickModeY, kickBtnWidth, kickBtnHeight);
    evilButton.setBounds(kickModeX + kickBtnWidth + kickBtnSpacing, kickModeY, kickBtnWidth, kickBtnHeight);
    hardButton.setBounds(kickModeX + (kickBtnWidth + kickBtnSpacing) * 2, kickModeY, kickBtnWidth, kickBtnHeight);
    softButton.setBounds(kickModeX, kickModeY + kickBtnHeight + 3, kickBtnWidth, kickBtnHeight);
    rawButton.setBounds(kickModeX + kickBtnWidth + kickBtnSpacing, kickModeY + kickBtnHeight + 3, kickBtnWidth, kickBtnHeight);
    metalButton.setBounds(kickModeX + (kickBtnWidth + kickBtnSpacing) * 2, kickModeY + kickBtnHeight + 3, kickBtnWidth, kickBtnHeight);

    // EQ Curve display (right side, large)
    if (eqCurve != nullptr)
        eqCurve->setBounds(360, 320, 475, 140);

    // Layer controls section (bottom left)
    int layerY = 380;
    int layerKnobSize = 45;
    int layerSpacing = 52;

    // Sub layer
    subVolKnob.setBounds(15, layerY, layerKnobSize, layerKnobSize);
    subPitchKnob.setBounds(15 + layerSpacing, layerY, layerKnobSize, layerKnobSize);
    subDecayKnob.setBounds(15 + layerSpacing * 2, layerY, layerKnobSize, layerKnobSize);

    // Body layer
    bodyVolKnob.setBounds(15, layerY + 70, layerKnobSize, layerKnobSize);
    bodyPitchKnob.setBounds(15 + layerSpacing, layerY + 70, layerKnobSize, layerKnobSize);
    bodyDecayKnob.setBounds(15 + layerSpacing * 2, layerY + 70, layerKnobSize, layerKnobSize);

    // Click layer
    clickVolKnob.setBounds(185, layerY, layerKnobSize, layerKnobSize);
    clickPitchKnob.setBounds(185 + layerSpacing, layerY, layerKnobSize, layerKnobSize);
    clickDecayKnob.setBounds(185 + layerSpacing * 2, layerY, layerKnobSize, layerKnobSize);
}

void GabbermasterAudioProcessorEditor::timerCallback()
{
    // Update sample number label
    int presetIndex = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("preset")->load());
    sampleNumberLabel.setText(juce::String(presetIndex + 1) + "/48", juce::dontSendNotification);

    // Sync UI state with processor state
    int filterType = audioProcessor.getCurrentFilterType();
    if (filterTypeSelector.getSelectedId() != filterType + 1)
        filterTypeSelector.setSelectedId(filterType + 1, juce::dontSendNotification);

    bool slowModeActive = audioProcessor.isSlowMode();
    slowButton.setToggleState(slowModeActive, juce::dontSendNotification);
    fastButton.setToggleState(!slowModeActive, juce::dontSendNotification);

    bool bypassed = audioProcessor.isEffectsBypassed();
    offButton.setToggleState(bypassed, juce::dontSendNotification);
    onButton.setToggleState(!bypassed, juce::dontSendNotification);

    // Sync kick mode buttons
    if (auto* kickModeParam = audioProcessor.getAPVTS().getRawParameterValue("kickMode"))
    {
        int currentKickMode = static_cast<int>(kickModeParam->load());
        viperButton.setToggleState(currentKickMode == 0, juce::dontSendNotification);
        evilButton.setToggleState(currentKickMode == 1, juce::dontSendNotification);
        hardButton.setToggleState(currentKickMode == 2, juce::dontSendNotification);
        softButton.setToggleState(currentKickMode == 3, juce::dontSendNotification);
        rawButton.setToggleState(currentKickMode == 4, juce::dontSendNotification);
        metalButton.setToggleState(currentKickMode == 5, juce::dontSendNotification);
    }

    repaint();
}

void GabbermasterAudioProcessorEditor::loadCustomSample()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select a sample file", juce::File(), "*.wav;*.aiff;*.aif");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            audioProcessor.loadCustomSample(file);
            sampleNameField.setText(file.getFileNameWithoutExtension());
        }
    });
}

void GabbermasterAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&knobLookAndFeel);
    slider.setTextValueSuffix(suffix);
    addAndMakeVisible(slider);
}

void GabbermasterAudioProcessorEditor::addLabel(const juce::String& text, juce::Component& component)
{
    auto label = std::make_unique<juce::Label>();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    label->setFont(juce::FontOptions(10.0f));
    label->setColour(juce::Label::textColourId, juce::Colours::white);
    label->attachToComponent(&component, false);
    addAndMakeVisible(label.get());
    labels.push_back(std::move(label));
}

//==============================================================================
void GabbermasterAudioProcessorEditor::GabberKnobLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(3);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto centerX = bounds.getCentreX();
    auto centerY = bounds.getCentreY();

    // Outer shadow ring
    for (int i = 3; i > 0; i--)
    {
        g.setColour(juce::Colour(0xff0a0a0a).withAlpha(0.3f * i / 3.0f));
        g.fillEllipse(bounds.expanded(static_cast<float>(i * 2)));
    }

    // Main knob body - metallic gradient
    juce::ColourGradient knobGradient(juce::Colour(0xff5a5a5a), centerX - radius * 0.5f, centerY - radius * 0.5f,
                                       juce::Colour(0xff1a1a1a), centerX + radius * 0.5f, centerY + radius * 0.5f, true);
    g.setGradientFill(knobGradient);
    g.fillEllipse(bounds);

    // Outer ring - beveled edge highlight
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawEllipse(bounds, 2.0f);

    // Inner ring highlight
    auto innerRing = bounds.reduced(radius * 0.15f);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawEllipse(innerRing, 1.0f);

    // Inner dark circle (recessed area)
    auto innerBounds = bounds.reduced(radius * 0.35f);
    juce::ColourGradient innerGradient(juce::Colour(0xff1a1a1a), centerX, innerBounds.getY(),
                                        juce::Colour(0xff2a2a2a), centerX, innerBounds.getBottom(), false);
    g.setGradientFill(innerGradient);
    g.fillEllipse(innerBounds);

    // Screwhead cross pattern (like original hardware knob)
    g.setColour(juce::Colour(0xff0a0a0a));
    auto crossSize = radius * 0.35f;
    g.drawLine(centerX - crossSize, centerY, centerX + crossSize, centerY, 2.5f);
    g.drawLine(centerX, centerY - crossSize, centerX, centerY + crossSize, 2.5f);

    // White indicator dot with glow
    if (slider.isEnabled())
    {
        auto dotRadius = radius * 0.1f;
        auto dotDistance = radius * 0.72f;

        juce::Point<float> dotPos(centerX + dotDistance * std::sin(toAngle),
                                   centerY - dotDistance * std::cos(toAngle));

        // Dot glow
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillEllipse(dotPos.x - dotRadius * 1.5f, dotPos.y - dotRadius * 1.5f,
                      dotRadius * 3.0f, dotRadius * 3.0f);

        // Main dot
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotPos.x - dotRadius, dotPos.y - dotRadius, dotRadius * 2, dotRadius * 2);
    }
}
