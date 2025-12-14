#include "PluginProcessor.h"
#include "PluginEditor.h"

// Debug logging helper
static void logMessage(const juce::String& msg)
{
    juce::Logger::writeToLog(msg);
    DBG(msg);
}

//==============================================================================
GabbermasterAudioProcessor::GabbermasterAudioProcessor()
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
    // Register audio formats
    formatManager.registerBasicFormats();

    // Load default samples
    sampleManager.loadDefaultSamples();

    // Initialize smoothed values
    driveSmoothed.reset(44100.0, 0.05);
    outputGainSmoothed.reset(44100.0, 0.05);
    mixSmoothed.reset(44100.0, 0.05);
}

GabbermasterAudioProcessor::~GabbermasterAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GabbermasterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Preset selector (48 slots like original UI)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "preset", "Preset", 0, 47, 0));

    // Kick Mode selector (Viper, Evil, Hard, Soft, Raw, Metal)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "kickMode", "Kick Mode",
        juce::StringArray{"Viper", "Evil", "Hard", "Soft", "Raw", "Metal"}, 0));

    // Destroy section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive", 0.0f, 100.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "distMode", "Distortion Mode",
        juce::StringArray{"Soft", "Hard", "Fuzz", "Decimator"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bitCrush", "Bit Crush", 1.0f, 16.0f, 16.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "saturate", "Saturate", 0.0f, 100.0f, 0.0f));

    // Pre/Post distortion position
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "distPosition", "Distortion Position",
        juce::StringArray{"Pre", "Post", "Both"}, 1));

    // Filter section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hpf", "High Pass", 20.0f, 2000.0f, 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lpf", "Low Pass", 200.0f, 20000.0f, 20000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "resonance", "Resonance", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterEnv", "Filter Envelope", 0.0f, 1.0f, 0.0f));

    // Track knob - tonal/pitch tuning (-12 to +12 semitones)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "track", "Track", -12.0f, 12.0f, 0.0f));

    // Envelope section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", 0.0f, 1000.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decay", "Decay", 10.0f, 5000.0f, 500.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", 10.0f, 5000.0f, 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sustain", "Sustain", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitchEnv", "Pitch Envelope", -24.0f, 24.0f, 0.0f));

    // Output section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", -60.0f, 12.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", 0.0f, 100.0f, 100.0f));

    // Reverb/Effects section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "room", "Room Size", 0.0f, 1.0f, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "width", "Stereo Width", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "damp", "Damping", 0.0f, 1.0f, 0.5f));

    // Enhanced EQ - 8 bands for detailed kick sculpting
    // Band 1 - Sub Bass (30-60 Hz) - for deep sub weight
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Freq", "Sub Frequency",
        juce::NormalisableRange<float>(20.0f, 80.0f, 0.1f, 0.5f), 40.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Gain", "Sub Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Q", "Sub Q", 0.1f, 10.0f, 0.7f));

    // Band 2 - Bass Punch (60-120 Hz) - fundamental punch
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Freq", "Punch Frequency",
        juce::NormalisableRange<float>(50.0f, 150.0f, 0.1f, 0.5f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Gain", "Punch Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Q", "Punch Q", 0.1f, 10.0f, 1.0f));

    // Band 3 - Low Body (120-300 Hz) - body/weight
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Freq", "Body Frequency",
        juce::NormalisableRange<float>(100.0f, 400.0f, 0.1f, 0.5f), 200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Gain", "Body Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Q", "Body Q", 0.1f, 10.0f, 1.0f));

    // Band 4 - Low-Mid (300-800 Hz) - mud/warmth control
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Freq", "Low-Mid Frequency",
        juce::NormalisableRange<float>(250.0f, 1000.0f, 0.1f, 0.5f), 500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Gain", "Low-Mid Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Q", "Low-Mid Q", 0.1f, 10.0f, 1.0f));

    // Band 5 - Mid (800-2000 Hz) - attack definition
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Freq", "Mid Frequency",
        juce::NormalisableRange<float>(600.0f, 2500.0f, 0.1f, 0.5f), 1200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Gain", "Mid Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Q", "Mid Q", 0.1f, 10.0f, 1.0f));

    // Band 6 - High-Mid Click (2-5 kHz) - click/attack
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq6Freq", "Click Frequency",
        juce::NormalisableRange<float>(1500.0f, 6000.0f, 1.0f, 0.5f), 3000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq6Gain", "Click Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq6Q", "Click Q", 0.1f, 10.0f, 1.5f));

    // Band 7 - Presence (5-10 kHz) - brightness/presence
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq7Freq", "Presence Frequency",
        juce::NormalisableRange<float>(4000.0f, 12000.0f, 1.0f, 0.5f), 7000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq7Gain", "Presence Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq7Q", "Presence Q", 0.1f, 10.0f, 1.0f));

    // Band 8 - Air (10-20 kHz) - air/sparkle
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq8Freq", "Air Frequency",
        juce::NormalisableRange<float>(8000.0f, 20000.0f, 1.0f, 0.5f), 12000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq8Gain", "Air Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq8Q", "Air Q", 0.1f, 10.0f, 0.7f));

    // Layer controls
    // Sub layer (low frequencies)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "subVol", "Sub Volume", -60.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "subPitch", "Sub Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "subDecay", "Sub Decay", 10.0f, 2000.0f, 500.0f));

    // Body layer (mid frequencies)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bodyVol", "Body Volume", -60.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bodyPitch", "Body Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bodyDecay", "Body Decay", 10.0f, 2000.0f, 300.0f));

    // Click/Transient layer (high frequencies)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "clickVol", "Click Volume", -60.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "clickPitch", "Click Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "clickDecay", "Click Decay", 1.0f, 500.0f, 50.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String GabbermasterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GabbermasterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GabbermasterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GabbermasterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GabbermasterAudioProcessor::getTailLengthSeconds() const
{
    return 0.5; // Account for reverb tail
}

int GabbermasterAudioProcessor::getNumPrograms()
{
    return 1;
}

int GabbermasterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GabbermasterAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String GabbermasterAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void GabbermasterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void GabbermasterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare DSP
    distortion.prepare(sampleRate, samplesPerBlock);
    filter.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);
    parametricEQ.prepare(sampleRate, samplesPerBlock);
    curveEQ.prepare(sampleRate, samplesPerBlock);
    curveEQ.setCurveTemplate(CurveEQ::CurveTemplate::PunchBoost); // Default to punch boost
    layerProcessor.prepare(sampleRate, samplesPerBlock);

    // Reset smoothed values
    driveSmoothed.reset(sampleRate, 0.05);
    outputGainSmoothed.reset(sampleRate, 0.05);
    mixSmoothed.reset(sampleRate, 0.05);

    // Prepare voices
    for (auto& voice : voices)
    {
        voice.reset();
        voice.envelope.setSampleRate(sampleRate);
        voice.pitchEnvelope.setSampleRate(sampleRate);
    }
}

void GabbermasterAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GabbermasterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void GabbermasterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Handle MIDI
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        handleMidiEvent(message);
    }

    // Render voices
    buffer.clear();
    renderVoices(buffer, buffer.getNumSamples());

    // Check if effects are bypassed
    if (effectsBypassed.load())
    {
        // Just apply output gain and mix
        float outputDb = apvts.getRawParameterValue("output")->load();
        float mix = apvts.getRawParameterValue("mix")->load() / 100.0f;
        float gain = juce::Decibels::decibelsToGain(outputDb);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                channelData[i] = channelData[i] * gain * mix;
            }
        }
        return;
    }

    // Apply DSP effects
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // Get parameters
    float drive = apvts.getRawParameterValue("drive")->load();
    int distMode = (int)apvts.getRawParameterValue("distMode")->load();
    float bitCrush = apvts.getRawParameterValue("bitCrush")->load();
    float saturate = apvts.getRawParameterValue("saturate")->load();
    int distPosition = (int)apvts.getRawParameterValue("distPosition")->load(); // 0=Pre, 1=Post, 2=Both
    float hpf = apvts.getRawParameterValue("hpf")->load();
    float lpf = apvts.getRawParameterValue("lpf")->load();
    float resonance = apvts.getRawParameterValue("resonance")->load();
    float outputDb = apvts.getRawParameterValue("output")->load();
    float mix = apvts.getRawParameterValue("mix")->load() / 100.0f;
    int kickMode = (int)apvts.getRawParameterValue("kickMode")->load();
    
    // Store distortion position for processing
    distortionPosition.store(distPosition);

    // Get reverb parameters
    float roomSize = apvts.getRawParameterValue("room")->load();
    float width = apvts.getRawParameterValue("width")->load();
    float damping = apvts.getRawParameterValue("damp")->load();

    // Set reverb parameters
    reverb.setParameters(roomSize, width, damping);

    // Update EQ parameters (8 bands)
    for (int band = 0; band < ParametricEQ::numBands; ++band)
    {
        juce::String freqId = "eq" + juce::String(band + 1) + "Freq";
        juce::String gainId = "eq" + juce::String(band + 1) + "Gain";
        juce::String qId = "eq" + juce::String(band + 1) + "Q";

        float freq = apvts.getRawParameterValue(freqId)->load();
        float gain = apvts.getRawParameterValue(gainId)->load();
        float q = apvts.getRawParameterValue(qId)->load();

        parametricEQ.setBandParameters(band, freq, gain, q);
    }

    // Get layer parameters
    float subVolDb = apvts.getRawParameterValue("subVol")->load();
    float subPitch = apvts.getRawParameterValue("subPitch")->load();
    float subDecay = apvts.getRawParameterValue("subDecay")->load();

    float bodyVolDb = apvts.getRawParameterValue("bodyVol")->load();
    float bodyPitch = apvts.getRawParameterValue("bodyPitch")->load();
    float bodyDecay = apvts.getRawParameterValue("bodyDecay")->load();

    float clickVolDb = apvts.getRawParameterValue("clickVol")->load();
    float clickPitch = apvts.getRawParameterValue("clickPitch")->load();
    float clickDecay = apvts.getRawParameterValue("clickDecay")->load();

    // Set layer parameters
    layerProcessor.setSubParameters(subVolDb, subPitch, subDecay);
    layerProcessor.setBodyParameters(bodyVolDb, bodyPitch, bodyDecay);
    layerProcessor.setClickParameters(clickVolDb, clickPitch, clickDecay);

    // Set smoothed targets
    driveSmoothed.setTargetValue(drive / 100.0f);
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
    mixSmoothed.setTargetValue(mix);

    // Mode-specific saturation adjustment
    float modeSaturate = saturate;
    switch (kickMode)
    {
        case 0: // Viper - more saturation
            modeSaturate = juce::jmin(100.0f, saturate + 20.0f);
            break;
        case 1: // Evil - heavy saturation
            modeSaturate = juce::jmin(100.0f, saturate + 40.0f);
            break;
        case 3: // Soft - less saturation
            modeSaturate = juce::jmax(0.0f, saturate - 20.0f);
            break;
        case 4: // Raw - no saturation
            modeSaturate = 0.0f;
            break;
        case 5: // Metal - more saturation
            modeSaturate = juce::jmin(100.0f, saturate + 30.0f);
            break;
    }

    // Process each sample
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float sampleL = channelDataL[i];
        float sampleR = channelDataR ? channelDataR[i] : sampleL;

        // Apply layer processing (multi-band split with volume/decay)
        float layerOutL, layerOutR;
        layerProcessor.processLayerMix(sampleL, sampleR, layerOutL, layerOutR);
        sampleL = layerOutL;
        sampleR = layerOutR;

        // Store dry signal for parallel processing
        float dryL = sampleL;
        float dryR = sampleR;

        // Apply Pre distortion (before EQ)
        float driveVal = driveSmoothed.getNextValue();
        float preDistL = sampleL;
        float preDistR = sampleR;
        if ((distPosition == 0 || distPosition == 2) && driveVal > 0.01f)
        {
            preDistL = distortion.processSample(sampleL, driveVal, distMode, bitCrush, modeSaturate);
            preDistR = distortion.processSample(sampleR, driveVal, distMode, bitCrush, modeSaturate);
        }
        
        // Use pre-distorted signal if in Pre or Both mode
        if (distPosition == 0) // Pre only
        {
            sampleL = preDistL;
            sampleR = preDistR;
        }
        else if (distPosition == 2) // Both - blend
        {
            sampleL = preDistL * 0.5f + dryL * 0.5f;
            sampleR = preDistR * 0.5f + dryR * 0.5f;
        }

        // Apply filter (track knob now affects pitch in renderVoices, not filter)
        float trackedHpf = hpf;
        float trackedLpf = lpf;

        int filterType = currentFilterType.load();
        if (filterType == 0) // HP
        {
            sampleL = filter.processSample(sampleL, trackedHpf, 20000.0f, resonance);
            sampleR = filter.processSample(sampleR, trackedHpf, 20000.0f, resonance);
        }
        else if (filterType == 1) // LP
        {
            sampleL = filter.processSample(sampleL, 20.0f, trackedLpf, resonance);
            sampleR = filter.processSample(sampleR, 20.0f, trackedLpf, resonance);
        }
        else // BP
        {
            sampleL = filter.processSample(sampleL, trackedHpf, trackedLpf, resonance);
            sampleR = filter.processSample(sampleR, trackedHpf, trackedLpf, resonance);
        }

        // Apply curve-based EQ (time-dependent)
        curveEQ.processStereo(sampleL, sampleR);
        
        // Also apply parametric EQ for fine-tuning (optional, can be disabled)
        // parametricEQ.processStereo(sampleL, sampleR);

        // Apply Post distortion (after EQ)
        if (distPosition == 1 || distPosition == 2) // Post or Both
        {
            if (driveVal > 0.01f)
            {
                float postDistL = distortion.processSample(sampleL, driveVal, distMode, bitCrush, modeSaturate);
                float postDistR = distortion.processSample(sampleR, driveVal, distMode, bitCrush, modeSaturate);
                
                if (distPosition == 1) // Post only
                {
                    sampleL = postDistL;
                    sampleR = postDistR;
                }
                else // Both - blend pre and post
                {
                    sampleL = preDistL * 0.3f + postDistL * 0.7f;
                    sampleR = preDistR * 0.3f + postDistR * 0.7f;
                }
            }
        }

        // Apply reverb
        if (roomSize > 0.01f)
        {
            reverb.processStereo(sampleL, sampleR);
        }

        // Apply output gain and mix
        float outGain = outputGainSmoothed.getNextValue();
        float mixVal = mixSmoothed.getNextValue();
        sampleL *= outGain * mixVal;
        sampleR *= outGain * mixVal;

        // Update levels for UI
        subLevel.store(layerProcessor.getSubLevel());
        bodyLevel.store(layerProcessor.getBodyLevel());
        clickLevel.store(layerProcessor.getClickLevel());

        // Write output
        channelDataL[i] = sampleL;
        if (channelDataR)
            channelDataR[i] = sampleR;
    }
}

//==============================================================================
void GabbermasterAudioProcessor::handleMidiEvent(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        Voice* voice = findFreeVoice();
        if (voice == nullptr)
            voice = &voices[0]; // Steal oldest voice

        voice->isActive = true;
        voice->noteNumber = message.getNoteNumber();
        voice->velocity = message.getFloatVelocity();
        voice->samplePosition = 0;
        
        // Reset oscillator phases
        voice->phase = 0.0f;
        voice->phase2 = 0.0f;
        voice->phase3 = 0.0f;
        voice->noiseState = 0.0f;

        // Get current mode and initialize mode parameters
        int kickMode = static_cast<int>(apvts.getRawParameterValue("kickMode")->load());
        float modeStartPitch, modeEndPitch, detune1, detune2, harmonicMix;
        getModeParameters(kickMode, modeStartPitch, modeEndPitch, detune1, detune2, harmonicMix);
        voice->mode = kickMode;
        voice->startPitch = modeStartPitch;
        voice->endPitch = modeEndPitch;
        voice->harmonicMix = harmonicMix;

        // Calculate pitch ratio based on note
        int baseNote = 60; // C4
        float semitones = static_cast<float>(voice->noteNumber - baseNote);
        voice->pitchRatio = std::pow(2.0f, semitones / 12.0f);

        // Setup envelope
        float attack = apvts.getRawParameterValue("attack")->load();
        float decay = apvts.getRawParameterValue("decay")->load();
        float sustain = apvts.getRawParameterValue("sustain")->load();
        float release = apvts.getRawParameterValue("release")->load();

        float timeScale = slowMode.load() ? 4.0f : 1.0f;

        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = (attack * timeScale) / 1000.0f;
        adsrParams.decay = (decay * timeScale) / 1000.0f;
        adsrParams.sustain = sustain;
        adsrParams.release = (release * timeScale) / 1000.0f;

        voice->envelope.setParameters(adsrParams);
        voice->envelope.noteOn();

        // Setup pitch envelope - more aggressive for classic gabber pitch drop
        float pitchEnvAmount = apvts.getRawParameterValue("pitchEnv")->load();
        voice->pitchEnvValue = pitchEnvAmount;

        juce::ADSR::Parameters pitchEnvParams;
        pitchEnvParams.attack = 0.0005f; // Faster attack for sharper drop
        pitchEnvParams.decay = juce::jmax(0.05f, (decay * timeScale) / 1000.0f * 0.5f); // Match decay time but faster
        pitchEnvParams.sustain = 0.0f;
        pitchEnvParams.release = 0.02f; // Quick release
        voice->pitchEnvelope.setParameters(pitchEnvParams);
        voice->pitchEnvelope.noteOn();

        // Trigger layer envelope
        layerProcessor.noteOn();
    }
    else if (message.isNoteOff())
    {
        Voice* voice = findVoiceForNote(message.getNoteNumber());
        if (voice)
        {
            voice->envelope.noteOff();
            voice->pitchEnvelope.noteOff();
            layerProcessor.noteOff();
        }
    }
}

void GabbermasterAudioProcessor::getModeParameters(int mode, float& startPitch, float& endPitch, float& detune1, float& detune2, float& harmonicMix)
{
    switch (mode)
    {
        case 0: // Viper - sawtooth-like harmonics, bright attack
            startPitch = 180.0f; // Hz
            endPitch = 50.0f;
            detune1 = 1.05f; // +5% detune
            detune2 = 0.97f; // -3% detune
            harmonicMix = 0.4f; // Strong harmonics
            break;
        case 1: // Evil - sine-sub with detuned layers
            startPitch = 50.0f;
            endPitch = 35.0f;
            detune1 = 1.02f;
            detune2 = 0.98f;
            harmonicMix = 0.2f; // Subtle harmonics
            break;
        case 2: // Hard - triangle base, tight
            startPitch = 120.0f;
            endPitch = 60.0f;
            detune1 = 1.01f;
            detune2 = 0.99f;
            harmonicMix = 0.3f;
            break;
        case 3: // Soft - clean, natural
            startPitch = 100.0f;
            endPitch = 55.0f;
            detune1 = 1.0f;
            detune2 = 1.0f;
            harmonicMix = 0.1f;
            break;
        case 4: // Raw - unprocessed character
            startPitch = 80.0f;
            endPitch = 45.0f;
            detune1 = 1.0f;
            detune2 = 1.0f;
            harmonicMix = 0.0f;
            break;
        case 5: // Metal - harsh, industrial
            startPitch = 200.0f;
            endPitch = 65.0f;
            detune1 = 1.08f;
            detune2 = 0.92f;
            harmonicMix = 0.5f;
            break;
        default:
            startPitch = 100.0f;
            endPitch = 50.0f;
            detune1 = 1.0f;
            detune2 = 1.0f;
            harmonicMix = 0.2f;
            break;
    }
}

float GabbermasterAudioProcessor::generateWaveformSample(Voice& voice, float time, float currentPitch, int mode)
{
    float sample = 0.0f;
    float phaseInc = currentPitch / static_cast<float>(currentSampleRate);
    
    switch (mode)
    {
        case 0: // Viper - sawtooth with harmonics for punch and click
        {
            // Main sawtooth - strong mid-range punch (200-400Hz)
            float saw = 2.0f * (voice.phase - 0.5f);
            sample = saw * 1.2f; // Boost for punch
            
            // Add detuned sub-oscillators for grit and body
            float saw2 = 2.0f * (voice.phase2 - 0.5f);
            float saw3 = 2.0f * (voice.phase3 - 0.5f);
            sample += saw2 * 0.4f * voice.harmonicMix;
            sample += saw3 * 0.3f * voice.harmonicMix;
            
            // Add high-frequency harmonics for click (>8kHz content)
            float saw4 = 2.0f * ((voice.phase * 8.0f) - 0.5f); // 8x harmonic
            float saw5 = 2.0f * ((voice.phase * 16.0f) - 0.5f); // 16x harmonic
            sample += saw4 * 0.15f * (1.0f - time * 10.0f); // Decay quickly
            sample += saw5 * 0.1f * (1.0f - time * 20.0f);
            
            // Add sharp click transient (noise burst at start)
            if (time < 0.003f)
            {
                voice.noiseState = voice.noiseState * 0.85f + (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.15f;
                float clickEnv = 1.0f - (time / 0.003f);
                sample += voice.noiseState * clickEnv * 0.6f; // Stronger click
            }
            break;
        }
        case 1: // Evil - sine-sub with detuned layers for deep body
        {
            // Strong sub-bass body (40-80Hz)
            float sine = std::sin(voice.phase * juce::MathConstants<float>::twoPi);
            float sine2 = std::sin(voice.phase2 * juce::MathConstants<float>::twoPi);
            float sine3 = std::sin(voice.phase3 * juce::MathConstants<float>::twoPi);
            sample = sine * 1.3f + sine2 * 0.6f + sine3 * 0.4f; // Boost for body
            
            // Add subtle punch in mids
            float sine4 = std::sin(voice.phase * juce::MathConstants<float>::twoPi * 4.0f);
            sample += sine4 * 0.2f * (1.0f - time * 2.0f);
            
            // Minimal click for dark character
            if (time < 0.002f)
            {
                voice.noiseState = voice.noiseState * 0.9f + (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.1f;
                sample += voice.noiseState * (1.0f - time / 0.002f) * 0.2f;
            }
            break;
        }
        case 2: // Hard - triangle with tight punch
        {
            float tri = 4.0f * std::abs(voice.phase - 0.5f) - 1.0f;
            sample = tri * 1.1f; // Boost for tight punch
            // Add harmonics for definition
            float tri2 = 4.0f * std::abs(voice.phase2 - 0.5f) - 1.0f;
            float tri3 = 4.0f * std::abs((voice.phase * 3.0f) - 0.5f) - 1.0f;
            sample += tri2 * 0.3f * voice.harmonicMix;
            sample += tri3 * 0.15f * (1.0f - time * 5.0f); // Quick decay for click
            break;
        }
        case 3: // Soft - clean sine
        {
            sample = std::sin(voice.phase * juce::MathConstants<float>::twoPi);
            break;
        }
        case 4: // Raw - basic sine
        {
            sample = std::sin(voice.phase * juce::MathConstants<float>::twoPi);
            break;
        }
        case 5: // Metal - square with PWM for harsh industrial sound
        {
            float square = (voice.phase < 0.5f) ? 1.0f : -1.0f;
            sample = square * 1.2f; // Aggressive
            // Add strong harmonics for harshness
            float square2 = (voice.phase2 < 0.5f) ? 1.0f : -1.0f;
            float square3 = (voice.phase3 < 0.5f) ? 1.0f : -1.0f;
            float square4 = ((voice.phase * 5.0f) < 0.5f) ? 1.0f : -1.0f; // High harmonic
            sample += square2 * 0.4f * voice.harmonicMix;
            sample += square3 * 0.3f * voice.harmonicMix;
            sample += square4 * 0.2f * (1.0f - time * 8.0f); // Sharp click
            break;
        }
        default:
            sample = std::sin(voice.phase * juce::MathConstants<float>::twoPi);
            break;
    }
    
    return sample;
}

void GabbermasterAudioProcessor::renderVoices(juce::AudioBuffer<float>& buffer, int numSamples)
{
    int kickMode = static_cast<int>(apvts.getRawParameterValue("kickMode")->load());
    float trackAmount = apvts.getRawParameterValue("track")->load(); // -12 to +12 semitones
    
    // Get mode parameters
    float modeStartPitch, modeEndPitch, detune1, detune2, harmonicMix;
    getModeParameters(kickMode, modeStartPitch, modeEndPitch, detune1, detune2, harmonicMix);

    for (auto& voice : voices)
    {
        if (!voice.isActive)
            continue;

        // Initialize mode parameters if not set
        if (voice.mode != kickMode)
        {
            voice.mode = kickMode;
            voice.harmonicMix = harmonicMix;
        }

        float timeSinceNoteOn = 0.0f;
        float timeStep = 1.0f / static_cast<float>(currentSampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // Get envelope
            float envValue = voice.envelope.getNextSample();
            float pitchEnvValue = voice.pitchEnvelope.getNextSample();

            if (!voice.envelope.isActive())
            {
                voice.isActive = false;
                break;
            }

            // Calculate pitch envelope (from start to end pitch)
            float pitchEnvProgress = 1.0f - pitchEnvValue; // 1.0 at start, 0.0 at end
            float currentPitchHz = modeStartPitch + (modeEndPitch - modeStartPitch) * (1.0f - pitchEnvProgress);
            
            // Apply Track knob - scales entire pitch envelope (STRONG EFFECT)
            if (std::abs(trackAmount) > 0.01f)
            {
                float trackRatio = std::pow(2.0f, trackAmount / 12.0f);
                currentPitchHz *= trackRatio;
                
                // Add micro-detune for chorusing effect (stronger)
                float microDetune = trackAmount * 0.01f; // 10 cents per semitone - MORE AUDIBLE
                currentPitchHz *= (1.0f + microDetune);
                
                // Also affect harmonic content - higher track = brighter
                if (trackAmount > 0.0f)
                {
                    float harmonicBoost = 1.0f + (trackAmount / 12.0f) * 0.5f; // Up to 50% more harmonics
                    // This will be applied in waveform generation
                }
            }
            
            // Convert to phase increments
            float phaseInc = currentPitchHz / static_cast<float>(currentSampleRate);
            float phaseInc2 = currentPitchHz * detune1 / static_cast<float>(currentSampleRate);
            float phaseInc3 = currentPitchHz * detune2 / static_cast<float>(currentSampleRate);

            // Generate waveform sample
            float sampleValue = generateWaveformSample(voice, timeSinceNoteOn, currentPitchHz, kickMode);

            // Apply envelope and velocity
            sampleValue *= envValue * voice.velocity;

            // Store envelope value for UI
            currentEnvelopeValue.store(envValue);

            // Add to buffer (both channels)
            buffer.addSample(0, i, sampleValue);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, i, sampleValue);

            // Advance phases
            voice.phase += phaseInc;
            if (voice.phase >= 1.0f) voice.phase -= 1.0f;
            
            voice.phase2 += phaseInc2;
            if (voice.phase2 >= 1.0f) voice.phase2 -= 1.0f;
            
            voice.phase3 += phaseInc3;
            if (voice.phase3 >= 1.0f) voice.phase3 -= 1.0f;

            timeSinceNoteOn += timeStep;
        }
    }
}

GabbermasterAudioProcessor::Voice* GabbermasterAudioProcessor::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice.isActive)
            return &voice;
    }
    return nullptr;
}

GabbermasterAudioProcessor::Voice* GabbermasterAudioProcessor::findVoiceForNote(int noteNumber)
{
    for (auto& voice : voices)
    {
        if (voice.isActive && voice.noteNumber == noteNumber)
            return &voice;
    }
    return nullptr;
}

//==============================================================================
bool GabbermasterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GabbermasterAudioProcessor::createEditor()
{
    return new GabbermasterAudioProcessorEditor (*this);
}

//==============================================================================
void GabbermasterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GabbermasterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void GabbermasterAudioProcessor::loadCustomSample(const juce::File& file)
{
    sampleManager.loadCustomSample(file, formatManager);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GabbermasterAudioProcessor();
}
