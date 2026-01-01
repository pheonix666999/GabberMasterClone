#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KickSynthAudioProcessor::KickSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize voices
    for (int i = 0; i < maxVoices; ++i)
    {
        voices[i] = std::make_unique<KickVoice>();
    }
    
    // Initialize smoothed values
    bodyLevelSmoothed.reset(44100.0, 0.05);
    clickLevelSmoothed.reset(44100.0, 0.05);
    pitchStartSmoothed.reset(44100.0, 0.05);
    pitchEndSmoothed.reset(44100.0, 0.05);
    pitchTauSmoothed.reset(44100.0, 0.05);
    attackMsSmoothed.reset(44100.0, 0.05);
    t12MsSmoothed.reset(44100.0, 0.05);
    t24MsSmoothed.reset(44100.0, 0.05);
    tailMsSmoothed.reset(44100.0, 0.05);
    clickDecayMsSmoothed.reset(44100.0, 0.05);
    clickHPFHzSmoothed.reset(44100.0, 0.05);
    driveSmoothed.reset(44100.0, 0.05);
    asymmetrySmoothed.reset(44100.0, 0.05);
    outputGainSmoothed.reset(44100.0, 0.05);
    outputHPFHzSmoothed.reset(44100.0, 0.05);
    velocitySensitivitySmoothed.reset(44100.0, 0.05);
}

KickSynthAudioProcessor::~KickSynthAudioProcessor()
{
}

//==============================================================================
const juce::String KickSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KickSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KickSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KickSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KickSynthAudioProcessor::getTailLengthSeconds() const
{
    return 1.0; // 1 second tail
}

//==============================================================================
bool KickSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* KickSynthAudioProcessor::createEditor()
{
    return new KickSynthAudioProcessorEditor (*this);
}

//==============================================================================
int KickSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int KickSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KickSynthAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String KickSynthAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void KickSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void KickSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare DSP modules
    distortion.prepare(sampleRate, samplesPerBlock);
    limiter.prepare(sampleRate, samplesPerBlock);
    
    // Prepare voices
    for (auto& voice : voices)
    {
        if (voice)
            voice->noteOn(60, 1.0f, sampleRate); // Initialize
    }
    
    // Reset smoothed values
    bodyLevelSmoothed.reset(sampleRate, 0.05);
    clickLevelSmoothed.reset(sampleRate, 0.05);
    pitchStartSmoothed.reset(sampleRate, 0.05);
    pitchEndSmoothed.reset(sampleRate, 0.05);
    pitchTauSmoothed.reset(sampleRate, 0.05);
    attackMsSmoothed.reset(sampleRate, 0.05);
    t12MsSmoothed.reset(sampleRate, 0.05);
    t24MsSmoothed.reset(sampleRate, 0.05);
    tailMsSmoothed.reset(sampleRate, 0.05);
    clickDecayMsSmoothed.reset(sampleRate, 0.05);
    clickHPFHzSmoothed.reset(sampleRate, 0.05);
    driveSmoothed.reset(sampleRate, 0.05);
    asymmetrySmoothed.reset(sampleRate, 0.05);
    outputGainSmoothed.reset(sampleRate, 0.05);
    outputHPFHzSmoothed.reset(sampleRate, 0.05);
    velocitySensitivitySmoothed.reset(sampleRate, 0.05);

    outputHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    outputHPF.coefficients = outputHPFCoeffs;
    outputHPF.reset();
}

void KickSynthAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KickSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

void KickSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update voice parameters
    updateVoiceParameters();
    
    // Render voices
    renderVoices(buffer, midiMessages);
    
    // Output HPF (mono filter)
    if (buffer.getNumChannels() > 0)
    {
        auto* mainChannel = buffer.getWritePointer(0);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float filtered = outputHPF.processSample(mainChannel[sample]);
            mainChannel[sample] = filtered;
            for (int channel = 1; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample(channel, sample, filtered);
        }
    }
    
    // Apply distortion
    distortion.processBlock(buffer);
    
    // Apply limiter
    limiter.processBlock(buffer);
}

void KickSynthAudioProcessor::renderVoices(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (goRequest.exchange(false))
    {
        if (voices[0])
            voices[0]->noteOn(60, goVelocity.load(), currentSampleRate);
    }

    // Handle MIDI
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            float velocity = message.getFloatVelocity();
            
            // Trigger voice (monophonic for kick)
            if (voices[0])
            {
                voices[0]->noteOn(noteNumber, velocity, currentSampleRate);
            }
        }
        else if (message.isNoteOff())
        {
            // Note off (optional for kick, but handle it)
            if (voices[0])
            {
                voices[0]->noteOff();
            }
        }
    }
    
    // Render voices
    for (auto& voice : voices)
    {
        if (voice && voice->isActive())
        {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float voiceSample = voice->renderSample();
                
                // Write to all output channels
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    buffer.addSample(channel, sample, voiceSample);
                }
            }
        }
    }
}

void KickSynthAudioProcessor::updateVoiceParameters()
{
    // Get parameter values
    auto* bodyLevelParam = apvts.getRawParameterValue("bodyLevel");
    auto* clickLevelParam = apvts.getRawParameterValue("clickLevel");
    auto* pitchStartParam = apvts.getRawParameterValue("pitchStartHz");
    auto* pitchEndParam = apvts.getRawParameterValue("pitchEndHz");
    auto* pitchTauParam = apvts.getRawParameterValue("pitchTauMs");
    auto* attackMsParam = apvts.getRawParameterValue("attackMs");
    auto* t12MsParam = apvts.getRawParameterValue("t12Ms");
    auto* t24MsParam = apvts.getRawParameterValue("t24Ms");
    auto* tailMsParam = apvts.getRawParameterValue("tailMsToMinus60Db");
    auto* clickDecayMsParam = apvts.getRawParameterValue("clickDecayMs");
    auto* clickHPFHzParam = apvts.getRawParameterValue("clickHPFHz");
    auto* bodyOscTypeParam = apvts.getRawParameterValue("bodyOscType");
    auto* keyTrackingParam = apvts.getRawParameterValue("keyTracking");
    auto* retriggerModeParam = apvts.getRawParameterValue("retriggerMode");
    auto* distortionTypeParam = apvts.getRawParameterValue("distortionType");
    auto* driveParam = apvts.getRawParameterValue("drive");
    auto* asymmetryParam = apvts.getRawParameterValue("asymmetry");
    auto* outputGainParam = apvts.getRawParameterValue("outputGain");
    auto* outputHPFHzParam = apvts.getRawParameterValue("outputHPFHz");
    auto* velocitySensitivityParam = apvts.getRawParameterValue("velocitySensitivity");
    auto* limiterEnabledParam = apvts.getRawParameterValue("limiterEnabled");
    
    // Update smoothed values
    bodyLevelSmoothed.setTargetValue(bodyLevelParam->load());
    clickLevelSmoothed.setTargetValue(clickLevelParam->load());
    pitchStartSmoothed.setTargetValue(pitchStartParam->load());
    pitchEndSmoothed.setTargetValue(pitchEndParam->load());
    pitchTauSmoothed.setTargetValue(pitchTauParam->load());
    attackMsSmoothed.setTargetValue(attackMsParam->load());
    t12MsSmoothed.setTargetValue(t12MsParam->load());
    t24MsSmoothed.setTargetValue(t24MsParam->load());
    tailMsSmoothed.setTargetValue(tailMsParam->load());
    clickDecayMsSmoothed.setTargetValue(clickDecayMsParam->load());
    clickHPFHzSmoothed.setTargetValue(clickHPFHzParam->load());
    driveSmoothed.setTargetValue(driveParam->load());
    asymmetrySmoothed.setTargetValue(asymmetryParam->load());
    outputGainSmoothed.setTargetValue(outputGainParam->load());
    outputHPFHzSmoothed.setTargetValue(outputHPFHzParam->load());
    velocitySensitivitySmoothed.setTargetValue(velocitySensitivityParam->load());
    
    // Update voices
    for (auto& voice : voices)
    {
        if (voice)
        {
            voice->setBodyLevel(bodyLevelSmoothed.getNextValue());
            voice->setClickLevel(clickLevelSmoothed.getNextValue());
            voice->setPitchStartHz(pitchStartSmoothed.getNextValue());
            voice->setPitchEndHz(pitchEndSmoothed.getNextValue());
            voice->setPitchTauMs(pitchTauSmoothed.getNextValue());
            voice->setAttackMs(attackMsSmoothed.getNextValue());
            voice->setT12Ms(t12MsSmoothed.getNextValue());
            voice->setT24Ms(t24MsSmoothed.getNextValue());
            voice->setTailMsToMinus60Db(tailMsSmoothed.getNextValue());
            voice->setClickDecayMs(clickDecayMsSmoothed.getNextValue());
            voice->setClickHPFHz(clickHPFHzSmoothed.getNextValue());
            voice->setBodyOscillatorType(static_cast<int>(bodyOscTypeParam->load()));
            voice->setKeyTracking(keyTrackingParam->load());
            voice->setRetriggerMode(static_cast<bool>(retriggerModeParam->load()));
            voice->setVelocitySensitivity(velocitySensitivitySmoothed.getNextValue());
        }
    }
    
    // Update distortion
    distortion.setDistortionType(static_cast<int>(distortionTypeParam->load()));
    distortion.setDrive(driveSmoothed.getNextValue());
    distortion.setAsymmetry(asymmetrySmoothed.getNextValue());
    
    // Update limiter
    limiter.setLimiterEnabled(static_cast<bool>(limiterEnabledParam->load()));
    limiter.setOutputGain(outputGainSmoothed.getNextValue());

    // Update output HPF
    float hpfHz = outputHPFHzSmoothed.getNextValue();
    outputHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, hpfHz);
    outputHPF.coefficients = outputHPFCoeffs;
}

//==============================================================================
void KickSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void KickSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void KickSynthAudioProcessor::triggerGoHit(float velocity)
{
    goVelocity.store(juce::jlimit(0.0f, 1.0f, velocity));
    goRequest.store(true);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KickSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Body level: 0-1 (normalized)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "bodyLevel", "Body Level", 0.0f, 1.0f, 1.0f));
    
    // Click level: 0-1 (normalized)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "clickLevel", "Click Level", 0.0f, 1.0f, 0.5f));
    
    // Pitch envelope: Start Hz (20-500)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pitchStartHz", "Pitch Start Hz", 20.0f, 500.0f, 200.0f));
    
    // Pitch envelope: End Hz (20-200)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pitchEndHz", "Pitch End Hz", 20.0f, 200.0f, 50.0f));
    
    // Pitch envelope: Tau Ms (1-100)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pitchTauMs", "Pitch Tau Ms", 1.0f, 100.0f, 20.0f));
    
    // Amplitude envelope: Attack Ms (0-50)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attackMs", "Attack Ms", 0.0f, 50.0f, 0.0f));
    
    // Amplitude envelope: T12 Ms (1-50)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "t12Ms", "T12 Ms", 1.0f, 50.0f, 5.0f));
    
    // Amplitude envelope: T24 Ms (5-200)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "t24Ms", "T24 Ms", 5.0f, 200.0f, 20.0f));
    
    // Amplitude envelope: Tail Ms to -60dB (10-500)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "tailMsToMinus60Db", "Tail Ms To -60dB", 10.0f, 500.0f, 70.0f));
    
    // Click layer: Decay Ms (1-20)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "clickDecayMs", "Click Decay Ms", 1.0f, 20.0f, 3.0f));
    
    // Click layer: HPF Hz (500-10000)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "clickHPFHz", "Click HPF Hz", 500.0f, 10000.0f, 2000.0f));

    // Velocity sensitivity
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "velocitySensitivity", "Velocity Sensitivity", 0.0f, 1.0f, 1.0f));
    
    // Body oscillator type: 0=sine, 1=triangle
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "bodyOscType", "Body Osc Type", juce::StringArray("Sine", "Triangle"), 0));
    
    // Key tracking: -12 to +12 semitones
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "keyTracking", "Key Tracking", -12.0f, 12.0f, 0.0f));
    
    // Retrigger mode: on/off
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "retriggerMode", "Retrigger Mode", false));
    
    // Distortion type: 0=tanh, 1=hard clip, 2=asymmetric
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "distortionType", "Distortion Type", juce::StringArray("Tanh", "Hard Clip", "Asymmetric"), 0));
    
    // Distortion drive: 0-1
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive", 0.0f, 1.0f, 0.5f));
    
    // Distortion asymmetry: 0-1
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "asymmetry", "Asymmetry", 0.0f, 1.0f, 0.0f));
    
    // Output gain: -12 to +12 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain", -12.0f, 12.0f, 0.0f));

    // Output HPF: 20-200 Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "outputHPFHz", "Output HPF Hz", 20.0f, 200.0f, 20.0f));
    
    // Limiter enabled: on/off
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "limiterEnabled", "Limiter Enabled", true));
    
    return layout;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickSynthAudioProcessor();
}

