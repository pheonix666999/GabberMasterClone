#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GabbermasterAudioProcessorEditor::GabbermasterAudioProcessorEditor (GabbermasterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set look and feel colors
    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colour(0xffcc0000));
    getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffcc0000));
    getLookAndFeel().setColour(juce::Slider::trackColourId, juce::Colour(0xff444444));

    // GO Button
    goButton.setButtonText("GO");
    goButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc0000));
    goButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    goButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    goButton.addListener(this);
    addAndMakeVisible(goButton);

    // Note slider
    noteSlider.setRange(24, 96, 1);
    noteSlider.setValue(48);
    noteSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    noteSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    addAndMakeVisible(noteSlider);
    noteLabel.setText("Note", juce::dontSendNotification);
    noteLabel.attachToComponent(&noteSlider, true);
    addAndMakeVisible(noteLabel);

    // Kick Mode
    kickModeBox.addItem("Viper", 1);
    kickModeBox.addItem("Noise", 2);
    kickModeBox.addItem("Bounce", 3);
    kickModeBox.addItem("Rotterdam", 4);
    kickModeBox.addItem("Mutha", 5);
    kickModeBox.addItem("Stompin", 6);
    kickModeBox.addItem("Merrik", 7);
    kickModeBox.addItem("Massive", 8);
    addAndMakeVisible(kickModeBox);
    kickModeLabel.setText("Kick Mode", juce::dontSendNotification);
    kickModeLabel.attachToComponent(&kickModeBox, true);
    addAndMakeVisible(kickModeLabel);
    kickModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "KickSelector", kickModeBox);

    // Envelope sliders
    setupRotarySlider(attackSlider, attackLabel, "Attack");
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Envelope-ADSRAttack", attackSlider);

    setupRotarySlider(decaySlider, decayLabel, "Decay");
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Envelope-ADSRDecay", decaySlider);

    setupRotarySlider(sustainSlider, sustainLabel, "Sustain");
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Envelope-ADSRSustain", sustainSlider);

    setupRotarySlider(releaseSlider, releaseLabel, "Release");
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Envelope-ADSRRelease", releaseSlider);

    setupRotarySlider(volumeSlider, volumeLabel, "Volume");
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Envelope-ADSRVolume", volumeSlider);

    // Filter
    filterTypeBox.addItem("LP", 1);
    filterTypeBox.addItem("HP", 2);
    filterTypeBox.addItem("BP", 3);
    addAndMakeVisible(filterTypeBox);
    filterTypeLabel.setText("Filter", juce::dontSendNotification);
    filterTypeLabel.attachToComponent(&filterTypeBox, true);
    addAndMakeVisible(filterTypeLabel);
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "Filter-FilterType", filterTypeBox);

    setupRotarySlider(filterCutoffSlider, filterCutoffLabel, "Cutoff");
    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Filter-FilterCutoff", filterCutoffSlider);

    setupRotarySlider(filterEnvSlider, filterEnvLabel, "Env");
    filterEnvAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Filter-FilterEnvelope", filterEnvSlider);

    setupRotarySlider(filterQSlider, filterQLabel, "Q");
    filterQAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Filter-FilterQ", filterQSlider);

    setupRotarySlider(filterTrackSlider, filterTrackLabel, "Track");
    filterTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Filter-FilterTrack", filterTrackSlider);

    // Distortion
    setupRotarySlider(distPreSlider, distPreLabel, "Pre EQ");
    distPreAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "DistPreEQ", distPreSlider);

    setupRotarySlider(distPostSlider, distPostLabel, "Post EQ");
    distPostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "DistPostEQ", distPostSlider);

    // Reverb
    reverbOnButton.setButtonText("On");
    addAndMakeVisible(reverbOnButton);
    reverbOnLabel.setText("Reverb", juce::dontSendNotification);
    reverbOnLabel.attachToComponent(&reverbOnButton, true);
    addAndMakeVisible(reverbOnLabel);
    reverbOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "Reverb-ReverbOn", reverbOnButton);

    setupRotarySlider(reverbRoomSlider, reverbRoomLabel, "Room");
    reverbRoomAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Reverb-ReverbRoom", reverbRoomSlider);

    setupRotarySlider(reverbDampSlider, reverbDampLabel, "Damp");
    reverbDampAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Reverb-ReverbDamp", reverbDampSlider);

    setupRotarySlider(reverbWidthSlider, reverbWidthLabel, "Width");
    reverbWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Reverb-ReverbWidth", reverbWidthSlider);

    setupRotarySlider(reverbMixSlider, reverbMixLabel, "Mix");
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Reverb-ReverbMix", reverbMixSlider);

    // EQ
    eqOnButton.setButtonText("On");
    addAndMakeVisible(eqOnButton);
    eqOnLabel.setText("EQ", juce::dontSendNotification);
    eqOnLabel.attachToComponent(&eqOnButton, true);
    addAndMakeVisible(eqOnLabel);
    eqOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "EQ_EQON", eqOnButton);

    eqModeBox.addItem("Fast", 1);
    eqModeBox.addItem("Slow", 2);
    addAndMakeVisible(eqModeBox);
    eqModeLabel.setText("Mode", juce::dontSendNotification);
    eqModeLabel.attachToComponent(&eqModeBox, true);
    addAndMakeVisible(eqModeLabel);
    eqModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "EQ_EQMode", eqModeBox);

    // EQ Band 1 (simplified)
    setupRotarySlider(eq1FreqSlider, eq1FreqLabel, "Freq");
    eq1FreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "EQ-Band1-Freq", eq1FreqSlider);

    setupRotarySlider(eq1GainSlider, eq1GainLabel, "Gain");
    eq1GainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "EQ-Band1-Gain", eq1GainSlider);

    setupRotarySlider(eq1QSlider, eq1QLabel, "Q");
    eq1QAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "EQ-Band1-Q", eq1QSlider);

    setSize (800, 500);
}

GabbermasterAudioProcessorEditor::~GabbermasterAudioProcessorEditor()
{
    goButton.removeListener(this);
}

void GabbermasterAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.attachToComponent(&slider, true);
    addAndMakeVisible(label);
}

void GabbermasterAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void GabbermasterAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &goButton)
    {
        int note = static_cast<int>(noteSlider.getValue());
        audioProcessor.triggerKick(note, 1.0f);
    }
}

//==============================================================================
void GabbermasterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff1a1a1a));

    // Title
    g.setColour (juce::Colour(0xffcc0000));
    g.setFont (juce::FontOptions(28.0f, juce::Font::bold));
    g.drawFittedText ("GABBERMASTER CLONE", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    // Section backgrounds
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(45);

    // Draw section labels
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));

    g.drawText("ENVELOPE", 10, 90, 100, 20, juce::Justification::left);
    g.drawText("FILTER", 10, 180, 100, 20, juce::Justification::left);
    g.drawText("DISTORTION", 420, 90, 100, 20, juce::Justification::left);
    g.drawText("REVERB", 10, 280, 100, 20, juce::Justification::left);
    g.drawText("EQ", 10, 380, 100, 20, juce::Justification::left);
}

void GabbermasterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Top row: GO button, Note slider, Kick Mode
    auto topRow = bounds.removeFromTop(45);
    goButton.setBounds(topRow.removeFromLeft(80).reduced(5));
    noteLabel.setBounds(topRow.removeFromLeft(40));
    noteSlider.setBounds(topRow.removeFromLeft(150).reduced(5));
    topRow.removeFromLeft(20);
    kickModeLabel.setBounds(topRow.removeFromLeft(70));
    kickModeBox.setBounds(topRow.removeFromLeft(120).reduced(5));

    bounds.removeFromTop(10);

    // Envelope row
    auto envRow = bounds.removeFromTop(80);
    envRow.removeFromLeft(10); // margin

    int knobWidth = 65;
    int knobHeight = 70;

    attackLabel.setBounds(envRow.removeFromLeft(knobWidth).removeFromTop(15));
    attackSlider.setBounds(10, 105, knobWidth, knobHeight);

    decayLabel.setBounds(75, 90, knobWidth, 15);
    decaySlider.setBounds(75, 105, knobWidth, knobHeight);

    sustainLabel.setBounds(140, 90, knobWidth, 15);
    sustainSlider.setBounds(140, 105, knobWidth, knobHeight);

    releaseLabel.setBounds(205, 90, knobWidth, 15);
    releaseSlider.setBounds(205, 105, knobWidth, knobHeight);

    volumeLabel.setBounds(270, 90, knobWidth, 15);
    volumeSlider.setBounds(270, 105, knobWidth, knobHeight);

    // Distortion (right side of envelope row)
    distPreLabel.setBounds(420, 105, knobWidth, 15);
    distPreSlider.setBounds(420, 120, knobWidth, knobHeight);

    distPostLabel.setBounds(485, 105, knobWidth, 15);
    distPostSlider.setBounds(485, 120, knobWidth, knobHeight);

    // Filter row
    filterTypeLabel.setBounds(10, 195, 50, 20);
    filterTypeBox.setBounds(60, 195, 70, 25);

    filterCutoffLabel.setBounds(140, 195, knobWidth, 15);
    filterCutoffSlider.setBounds(140, 210, knobWidth, knobHeight);

    filterEnvLabel.setBounds(205, 195, knobWidth, 15);
    filterEnvSlider.setBounds(205, 210, knobWidth, knobHeight);

    filterQLabel.setBounds(270, 195, knobWidth, 15);
    filterQSlider.setBounds(270, 210, knobWidth, knobHeight);

    filterTrackLabel.setBounds(335, 195, knobWidth, 15);
    filterTrackSlider.setBounds(335, 210, knobWidth, knobHeight);

    // Reverb row
    reverbOnLabel.setBounds(10, 300, 50, 20);
    reverbOnButton.setBounds(60, 300, 50, 25);

    reverbRoomLabel.setBounds(120, 295, knobWidth, 15);
    reverbRoomSlider.setBounds(120, 310, knobWidth, knobHeight);

    reverbDampLabel.setBounds(185, 295, knobWidth, 15);
    reverbDampSlider.setBounds(185, 310, knobWidth, knobHeight);

    reverbWidthLabel.setBounds(250, 295, knobWidth, 15);
    reverbWidthSlider.setBounds(250, 310, knobWidth, knobHeight);

    reverbMixLabel.setBounds(315, 295, knobWidth, 15);
    reverbMixSlider.setBounds(315, 310, knobWidth, knobHeight);

    // EQ row
    eqOnLabel.setBounds(10, 400, 30, 20);
    eqOnButton.setBounds(45, 400, 50, 25);

    eqModeLabel.setBounds(100, 400, 40, 20);
    eqModeBox.setBounds(145, 400, 70, 25);

    eq1FreqLabel.setBounds(230, 395, knobWidth, 15);
    eq1FreqSlider.setBounds(230, 410, knobWidth, knobHeight);

    eq1GainLabel.setBounds(295, 395, knobWidth, 15);
    eq1GainSlider.setBounds(295, 410, knobWidth, knobHeight);

    eq1QLabel.setBounds(360, 395, knobWidth, 15);
    eq1QSlider.setBounds(360, 410, knobWidth, knobHeight);
}
