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

    // Filter section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hpf", "High Pass", 20.0f, 2000.0f, 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lpf", "Low Pass", 200.0f, 20000.0f, 20000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "resonance", "Resonance", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterEnv", "Filter Envelope", 0.0f, 1.0f, 0.0f));

    // Track knob - keyboard tracking for filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "track", "Track", 0.0f, 100.0f, 0.0f));

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
    float hpf = apvts.getRawParameterValue("hpf")->load();
    float lpf = apvts.getRawParameterValue("lpf")->load();
    float resonance = apvts.getRawParameterValue("resonance")->load();
    float outputDb = apvts.getRawParameterValue("output")->load();
    float mix = apvts.getRawParameterValue("mix")->load() / 100.0f;
    float trackAmount = apvts.getRawParameterValue("track")->load() / 100.0f;
    int kickMode = (int)apvts.getRawParameterValue("kickMode")->load();

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

    // Apply kick mode character adjustments
    float modeBoost = 1.0f;
    float modeSaturate = saturate;
    switch (kickMode)
    {
        case 0: // Viper - punchy, aggressive
            modeBoost = 1.2f;
            modeSaturate = juce::jmin(100.0f, saturate + 20.0f);
            break;
        case 1: // Evil - dark, heavy distortion
            modeBoost = 1.4f;
            modeSaturate = juce::jmin(100.0f, saturate + 40.0f);
            break;
        case 2: // Hard - tight, compressed
            modeBoost = 1.1f;
            break;
        case 3: // Soft - clean, natural
            modeBoost = 0.9f;
            modeSaturate = juce::jmax(0.0f, saturate - 20.0f);
            break;
        case 4: // Raw - unprocessed character
            modeBoost = 1.0f;
            modeSaturate = 0.0f;
            break;
        case 5: // Metal - harsh, industrial
            modeBoost = 1.3f;
            modeSaturate = juce::jmin(100.0f, saturate + 30.0f);
            break;
    }

    // Get current note for tracking (use the most recent active voice)
    int currentNote = 60; // Default to middle C
    for (const auto& voice : voices)
    {
        if (voice.isActive)
        {
            currentNote = voice.noteNumber;
            break;
        }
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

        // Apply mode boost
        sampleL *= modeBoost;
        sampleR *= modeBoost;

        // Apply distortion (Pre)
        float driveVal = driveSmoothed.getNextValue();
        if (driveVal > 0.01f)
        {
            sampleL = distortion.processSample(sampleL, driveVal, distMode, bitCrush, modeSaturate);
            sampleR = distortion.processSample(sampleR, driveVal, distMode, bitCrush, modeSaturate);
        }

        // Apply filter with keyboard tracking
        float trackedHpf = hpf;
        float trackedLpf = lpf;

        if (trackAmount > 0.01f)
        {
            // Calculate pitch ratio from MIDI note (relative to C3 = 48)
            float noteOffset = (currentNote - 48) / 12.0f; // Octaves from C3
            float pitchRatio = std::pow(2.0f, noteOffset * trackAmount);

            // Apply tracking to filter frequencies
            trackedHpf = juce::jlimit(20.0f, 2000.0f, hpf * pitchRatio);
            trackedLpf = juce::jlimit(200.0f, 20000.0f, lpf * pitchRatio);
        }

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

        // Apply EQ
        parametricEQ.processStereo(sampleL, sampleR);

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

        // Setup pitch envelope
        float pitchEnvAmount = apvts.getRawParameterValue("pitchEnv")->load();
        voice->pitchEnvValue = pitchEnvAmount;

        juce::ADSR::Parameters pitchEnvParams;
        pitchEnvParams.attack = 0.001f;
        pitchEnvParams.decay = 0.1f * timeScale;
        pitchEnvParams.sustain = 0.0f;
        pitchEnvParams.release = 0.05f;
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

void GabbermasterAudioProcessor::renderVoices(juce::AudioBuffer<float>& buffer, int numSamples)
{
    int presetIndex = static_cast<int>(apvts.getRawParameterValue("preset")->load());
    auto* sample = sampleManager.getSample(presetIndex);

    if (sample == nullptr || sample->getNumSamples() == 0)
        return;

    const int sampleLength = sample->getNumSamples();
    const float* sampleData = sample->getReadPointer(0);

    for (auto& voice : voices)
    {
        if (!voice.isActive)
            continue;

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

            // Calculate current pitch with envelope
            float currentPitch = voice.pitchRatio;
            if (voice.pitchEnvValue != 0.0f)
            {
                float pitchMod = voice.pitchEnvValue * pitchEnvValue;
                currentPitch *= std::pow(2.0f, pitchMod / 12.0f);
            }

            // Get sample value with interpolation
            float sampleValue = 0.0f;
            if (voice.samplePosition < sampleLength - 1)
            {
                int pos = static_cast<int>(voice.samplePosition);
                float frac = voice.samplePosition - pos;
                sampleValue = sampleData[pos] * (1.0f - frac) + sampleData[pos + 1] * frac;
            }

            // Apply envelope and velocity
            sampleValue *= envValue * voice.velocity;

            // Store envelope value for UI
            currentEnvelopeValue.store(envValue);

            // Add to buffer (both channels)
            buffer.addSample(0, i, sampleValue);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, i, sampleValue);

            // Advance sample position
            voice.samplePosition += currentPitch;

            // Loop or stop at end
            if (voice.samplePosition >= sampleLength)
            {
                voice.samplePosition = 0;
            }
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
