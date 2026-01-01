#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KickSynthAudioProcessorEditor::KickSynthAudioProcessorEditor (KickSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 600);
    
    // Setup sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& name, 
                              juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag)
    {
        slider.setSliderStyle(style);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);
        
        label.setText(name, juce::dontSendNotification);
        label.attachToComponent(&slider, false);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };
    
    setupSlider(bodyLevelSlider, bodyLevelLabel, "Body Level");
    setupSlider(clickLevelSlider, clickLevelLabel, "Click Level");
    setupSlider(pitchStartHzSlider, pitchStartHzLabel, "Pitch Start Hz");
    setupSlider(pitchEndHzSlider, pitchEndHzLabel, "Pitch End Hz");
    setupSlider(pitchTauMsSlider, pitchTauMsLabel, "Pitch Tau Ms");
    setupSlider(attackMsSlider, attackMsLabel, "Attack Ms");
    setupSlider(t12MsSlider, t12MsLabel, "T12 Ms");
    setupSlider(t24MsSlider, t24MsLabel, "T24 Ms");
    setupSlider(tailMsSlider, tailMsLabel, "Tail Ms");
    setupSlider(clickDecayMsSlider, clickDecayMsLabel, "Click Decay Ms");
    setupSlider(clickHPFHzSlider, clickHPFHzLabel, "Click HPF Hz");
    setupSlider(velocitySensitivitySlider, velocitySensitivityLabel, "Velocity Sensitivity");
    setupSlider(driveSlider, driveLabel, "Drive");
    setupSlider(asymmetrySlider, asymmetryLabel, "Asymmetry");
    setupSlider(outputGainSlider, outputGainLabel, "Output Gain");
    setupSlider(outputHPFHzSlider, outputHPFHzLabel, "Output HPF Hz");
    goButton.setButtonText("Go");
    addAndMakeVisible(goButton);
    goButton.onClick = [this]() { audioProcessor.triggerGoHit(1.0f); };
    loadParamsButton.setButtonText("Load JSON");
    addAndMakeVisible(loadParamsButton);
    saveParamsButton.setButtonText("Save JSON");
    addAndMakeVisible(saveParamsButton);
    setupSlider(keyTrackingSlider, keyTrackingLabel, "Key Tracking");
    
    // Setup combos
    bodyOscTypeCombo.addItem("Sine", 1);
    bodyOscTypeCombo.addItem("Triangle", 2);
    addAndMakeVisible(bodyOscTypeCombo);
    
    distortionTypeCombo.addItem("Tanh", 1);
    distortionTypeCombo.addItem("Hard Clip", 2);
    distortionTypeCombo.addItem("Asymmetric", 3);
    addAndMakeVisible(distortionTypeCombo);
    
    // Setup buttons
    retriggerModeButton.setButtonText("Retrigger Mode");
    addAndMakeVisible(retriggerModeButton);
    
    limiterEnabledButton.setButtonText("Limiter Enabled");
    addAndMakeVisible(limiterEnabledButton);

    auto setParamValue = [this](const juce::String& paramId, float value)
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter(paramId))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(value));
            param->endChangeGesture();
        }
    };

    loadParamsButton.onClick = [this, setParamValue]()
    {
        fileChooser = std::make_unique<juce::FileChooser>("Load kick params JSON", juce::File{}, "*.json");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this, setParamValue](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (!file.existsAsFile())
                return;

            auto parsed = juce::JSON::parse(file.loadFileAsString());
            if (parsed.isVoid() || parsed.isUndefined())
                return;

            auto* obj = parsed.getDynamicObject();
            if (obj == nullptr)
                return;

            auto setFloatIfPresent = [&](const juce::String& jsonKey, const juce::String& paramId)
            {
                if (obj->hasProperty(jsonKey))
                    setParamValue(paramId, (float)obj->getProperty(jsonKey));
            };

            auto setBoolIfPresent = [&](const juce::String& jsonKey, const juce::String& paramId)
            {
                if (obj->hasProperty(jsonKey))
                    setParamValue(paramId, (bool)obj->getProperty(jsonKey) ? 1.0f : 0.0f);
            };

            auto setChoiceIfPresent = [&](const juce::String& jsonKey, const juce::String& paramId)
            {
                if (obj->hasProperty(jsonKey))
                    setParamValue(paramId, (float)(int)obj->getProperty(jsonKey));
            };

            setFloatIfPresent("bodyLevel", "bodyLevel");
            setFloatIfPresent("clickLevel", "clickLevel");
            setFloatIfPresent("pitchStartHz", "pitchStartHz");
            setFloatIfPresent("pitchEndHz", "pitchEndHz");
            setFloatIfPresent("pitchTauMs", "pitchTauMs");
            setFloatIfPresent("attackMs", "attackMs");
            setFloatIfPresent("t12Ms", "t12Ms");
            setFloatIfPresent("t24Ms", "t24Ms");
            setFloatIfPresent("tailMsToMinus60Db", "tailMsToMinus60Db");
            setFloatIfPresent("clickDecayMs", "clickDecayMs");
            setFloatIfPresent("clickHPFHz", "clickHPFHz");
            setFloatIfPresent("velocitySensitivity", "velocitySensitivity");
            setFloatIfPresent("outputHPFHz", "outputHPFHz");
            setFloatIfPresent("keyTracking", "keyTracking");
            setFloatIfPresent("drive", "drive");
            setFloatIfPresent("asymmetry", "asymmetry");

            if (obj->hasProperty("outputGain"))
                setParamValue("outputGain", (float)obj->getProperty("outputGain"));
            else if (obj->hasProperty("outputGainDb"))
                setParamValue("outputGain", (float)obj->getProperty("outputGainDb"));

            setChoiceIfPresent("bodyOscType", "bodyOscType");
            setChoiceIfPresent("distortionType", "distortionType");
            setBoolIfPresent("retriggerMode", "retriggerMode");
            setBoolIfPresent("limiterEnabled", "limiterEnabled");
        });
    };

    saveParamsButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>("Save kick params JSON", juce::File{}, "*.json");
        const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file == juce::File())
                return;

            auto obj = juce::DynamicObject::Ptr(new juce::DynamicObject());
            auto get = [&](const juce::String& paramId) -> float
            {
                if (auto* v = audioProcessor.getAPVTS().getRawParameterValue(paramId))
                    return v->load();
                return 0.0f;
            };

            obj->setProperty("bodyLevel", get("bodyLevel"));
            obj->setProperty("clickLevel", get("clickLevel"));
            obj->setProperty("pitchStartHz", get("pitchStartHz"));
            obj->setProperty("pitchEndHz", get("pitchEndHz"));
            obj->setProperty("pitchTauMs", get("pitchTauMs"));
            obj->setProperty("attackMs", get("attackMs"));
            obj->setProperty("t12Ms", get("t12Ms"));
            obj->setProperty("t24Ms", get("t24Ms"));
            obj->setProperty("tailMsToMinus60Db", get("tailMsToMinus60Db"));
            obj->setProperty("clickDecayMs", get("clickDecayMs"));
            obj->setProperty("clickHPFHz", get("clickHPFHz"));
            obj->setProperty("velocitySensitivity", get("velocitySensitivity"));
            obj->setProperty("bodyOscType", (int)get("bodyOscType"));
            obj->setProperty("keyTracking", get("keyTracking"));
            obj->setProperty("retriggerMode", get("retriggerMode") >= 0.5f);
            obj->setProperty("distortionType", (int)get("distortionType"));
            obj->setProperty("drive", get("drive"));
            obj->setProperty("asymmetry", get("asymmetry"));
            obj->setProperty("outputGain", get("outputGain"));
            obj->setProperty("outputGainDb", get("outputGain"));
            obj->setProperty("outputHPFHz", get("outputHPFHz"));
            obj->setProperty("limiterEnabled", get("limiterEnabled") >= 0.5f);

            file.replaceWithText(juce::JSON::toString(juce::var(obj.get()), true));
        });
    };
    
    // Create attachments
    bodyLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "bodyLevel", bodyLevelSlider);
    clickLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickLevel", clickLevelSlider);
    pitchStartHzAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "pitchStartHz", pitchStartHzSlider);
    pitchEndHzAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "pitchEndHz", pitchEndHzSlider);
    pitchTauMsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "pitchTauMs", pitchTauMsSlider);
    attackMsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "attackMs", attackMsSlider);
    t12MsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "t12Ms", t12MsSlider);
    t24MsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "t24Ms", t24MsSlider);
    tailMsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "tailMsToMinus60Db", tailMsSlider);
    clickDecayMsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickDecayMs", clickDecayMsSlider);
    clickHPFHzAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "clickHPFHz", clickHPFHzSlider);
    velocitySensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "velocitySensitivity", velocitySensitivitySlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "drive", driveSlider);
    asymmetryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "asymmetry", asymmetrySlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "outputGain", outputGainSlider);
    outputHPFHzAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "outputHPFHz", outputHPFHzSlider);
    keyTrackingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "keyTracking", keyTrackingSlider);
    bodyOscTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "bodyOscType", bodyOscTypeCombo);
    distortionTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "distortionType", distortionTypeCombo);
    retriggerModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "retriggerMode", retriggerModeButton);
    limiterEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "limiterEnabled", limiterEnabledButton);
}

KickSynthAudioProcessorEditor::~KickSynthAudioProcessorEditor()
{
}

//==============================================================================
void KickSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("Kick Synth", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void KickSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(40);
    
    const int sliderWidth = 80;
    const int sliderHeight = 100;
    const int spacing = 10;
    
    int x = bounds.getX();
    int y = bounds.getY();
    
    // Row 1: Body and Click
    bodyLevelSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    clickLevelSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    
    // Row 2: Pitch envelope
    x = bounds.getX();
    y += sliderHeight + 30;
    pitchStartHzSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    pitchEndHzSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    pitchTauMsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    
    // Row 3: Amplitude envelope
    x = bounds.getX();
    y += sliderHeight + 30;
    attackMsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    t12MsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    t24MsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    tailMsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    
    // Row 4: Click layer
    x = bounds.getX();
    y += sliderHeight + 30;
    clickDecayMsSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    clickHPFHzSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    velocitySensitivitySlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    
    // Row 5: Distortion
    x = bounds.getX();
    y += sliderHeight + 30;
    driveSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    asymmetrySlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    distortionTypeCombo.setBounds(x, y, sliderWidth, 30);
    x += sliderWidth + spacing;
    
    // Row 6: Output and misc
    x = bounds.getX();
    y += sliderHeight + 30;
    outputGainSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    outputHPFHzSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    keyTrackingSlider.setBounds(x, y, sliderWidth, sliderHeight);
    x += sliderWidth + spacing;
    bodyOscTypeCombo.setBounds(x, y, sliderWidth, 30);
    x += sliderWidth + spacing;
    retriggerModeButton.setBounds(x, y, 120, 30);
    x += 130;
    limiterEnabledButton.setBounds(x, y, 120, 30);
    x += 130;
    goButton.setBounds(x, y, 100, 30);
    x += 110;
    loadParamsButton.setBounds(x, y, 100, 30);
    x += 110;
    saveParamsButton.setBounds(x, y, 100, 30);
}


