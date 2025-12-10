#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    // Preset selector
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "preset", "Preset", 0, 19, 0));

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

    // EQ Band parameters (5 bands)
    // Band 1 - Sub (80 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Freq", "EQ1 Frequency",
        juce::NormalisableRange<float>(20.0f, 200.0f, 1.0f, 0.5f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Gain", "EQ1 Gain", -18.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq1Q", "EQ1 Q", 0.1f, 10.0f, 1.0f));

    // Band 2 - Low-mid (250 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Freq", "EQ2 Frequency",
        juce::NormalisableRange<float>(100.0f, 500.0f, 1.0f, 0.5f), 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Gain", "EQ2 Gain", -18.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq2Q", "EQ2 Q", 0.1f, 10.0f, 1.0f));

    // Band 3 - Mid (1000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Freq", "EQ3 Frequency",
        juce::NormalisableRange<float>(400.0f, 2000.0f, 1.0f, 0.5f), 1000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Gain", "EQ3 Gain", -18.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq3Q", "EQ3 Q", 0.1f, 10.0f, 1.0f));

    // Band 4 - High-mid (4000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Freq", "EQ4 Frequency",
        juce::NormalisableRange<float>(2000.0f, 8000.0f, 1.0f, 0.5f), 4000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Gain", "EQ4 Gain", -18.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq4Q", "EQ4 Q", 0.1f, 10.0f, 1.0f));

    // Band 5 - High (12000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Freq", "EQ5 Frequency",
        juce::NormalisableRange<float>(8000.0f, 20000.0f, 1.0f, 0.5f), 12000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Gain", "EQ5 Gain", -18.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eq5Q", "EQ5 Q", 0.1f, 10.0f, 1.0f));

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

    // Get reverb parameters
    float roomSize = apvts.getRawParameterValue("room")->load();
    float width = apvts.getRawParameterValue("width")->load();
    float damping = apvts.getRawParameterValue("damp")->load();

    // Set reverb parameters
    reverb.setParameters(roomSize, width, damping);

    // Update EQ parameters
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

    // Set smoothed targets
    driveSmoothed.setTargetValue(drive / 100.0f);
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
    mixSmoothed.setTargetValue(mix);

    // Get envelope time scale (slow mode multiplies times by 4)
    float timeScale = slowMode.load() ? 4.0f : 1.0f;

    // Process each sample
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float sampleL = channelDataL[i];
        float sampleR = channelDataR ? channelDataR[i] : sampleL;

        // Apply distortion (Pre)
        float driveVal = driveSmoothed.getNextValue();
        sampleL = distortion.processSample(sampleL, driveVal, distMode, bitCrush, saturate);
        sampleR = distortion.processSample(sampleR, driveVal, distMode, bitCrush, saturate);

        // Apply filters based on filter type
        int filterType = currentFilterType.load();
        if (filterType == 0) // HI (High Pass)
        {
            sampleL = filter.processSample(sampleL, hpf, 20000.0f, resonance);
            sampleR = filter.processSample(sampleR, hpf, 20000.0f, resonance);
        }
        else if (filterType == 1) // LO (Low Pass)
        {
            sampleL = filter.processSample(sampleL, 20.0f, lpf, resonance);
            sampleR = filter.processSample(sampleR, 20.0f, lpf, resonance);
        }
        else // BP (Band Pass) - apply both
        {
            sampleL = filter.processSample(sampleL, hpf, lpf, resonance);
            sampleR = filter.processSample(sampleR, hpf, lpf, resonance);
        }

        // Apply parametric EQ
        parametricEQ.processStereo(sampleL, sampleR);

        // Apply reverb
        if (roomSize > 0.01f)
        {
            reverb.processStereo(sampleL, sampleR);
        }

        // Apply output gain
        float gain = outputGainSmoothed.getNextValue();
        sampleL *= gain;
        sampleR *= gain;

        // Mix wet/dry
        float mixVal = mixSmoothed.getNextValue();
        channelDataL[i] = sampleL * mixVal + dryBuffer.getSample(0, i) * (1.0f - mixVal);
        if (channelDataR)
            channelDataR[i] = sampleR * mixVal + (dryBuffer.getNumChannels() > 1 ? dryBuffer.getSample(1, i) : dryBuffer.getSample(0, i)) * (1.0f - mixVal);
    }
}

void GabbermasterAudioProcessor::handleMidiEvent(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        // Find free voice
        if (auto* voice = findFreeVoice())
        {
            voice->isActive = true;
            voice->noteNumber = message.getNoteNumber();
            voice->velocity = message.getVelocity() / 127.0f;
            voice->samplePosition = 0;

            // Get envelope time scale (slow mode multiplies times by 4)
            float timeScale = slowMode.load() ? 4.0f : 1.0f;

            // Set ADSR parameters
            float attack = apvts.getRawParameterValue("attack")->load() / 1000.0f * timeScale;
            float decay = apvts.getRawParameterValue("decay")->load() / 1000.0f * timeScale;
            float sustain = apvts.getRawParameterValue("sustain")->load();
            float release = apvts.getRawParameterValue("release")->load() / 1000.0f * timeScale;

            juce::ADSR::Parameters params;
            params.attack = attack;
            params.decay = decay;
            params.sustain = sustain;
            params.release = release;

            voice->envelope.setParameters(params);
            voice->envelope.noteOn();

            // Set pitch envelope (fast attack, no sustain for pitch drop effect)
            float pitchEnvAmount = apvts.getRawParameterValue("pitchEnv")->load();
            voice->pitchEnvValue = pitchEnvAmount;

            juce::ADSR::Parameters pitchParams;
            pitchParams.attack = 0.001f;
            pitchParams.decay = 0.1f * timeScale;
            pitchParams.sustain = 0.0f;
            pitchParams.release = 0.01f;

            voice->pitchEnvelope.setParameters(pitchParams);
            voice->pitchEnvelope.noteOn();

            // Calculate pitch ratio based on MIDI note (middle C = 60 = original pitch)
            voice->pitchRatio = std::pow(2.0f, (voice->noteNumber - 60) / 12.0f);
        }
    }
    else if (message.isNoteOff())
    {
        if (auto* voice = findVoiceForNote(message.getNoteNumber()))
        {
            voice->envelope.noteOff();
            voice->pitchEnvelope.noteOff();
        }
    }
}

void GabbermasterAudioProcessor::renderVoices(juce::AudioBuffer<float>& buffer, int numSamples)
{
    int presetIndex = (int)apvts.getRawParameterValue("preset")->load();
    auto* sample = sampleManager.getSample(presetIndex);

    if (!sample || sample->getNumSamples() == 0)
        return;

    float maxEnvValue = 0.0f;

    for (auto& voice : voices)
    {
        if (!voice.isActive)
            continue;

        for (int i = 0; i < numSamples; ++i)
        {
            float envelopeValue = voice.envelope.getNextSample();
            float pitchEnvValue = voice.pitchEnvelope.getNextSample();

            // Track envelope for UI
            if (envelopeValue > maxEnvValue)
                maxEnvValue = envelopeValue;

            if (!voice.envelope.isActive())
            {
                voice.reset();
                break;
            }

            // Calculate current pitch with envelope
            float pitchMod = 1.0f + (voice.pitchEnvValue / 12.0f) * pitchEnvValue;
            float currentPitch = voice.pitchRatio * pitchMod;

            // Get sample position (with pitch shifting)
            float floatPos = voice.samplePosition * currentPitch;
            int intPos = static_cast<int>(floatPos);

            if (intPos >= sample->getNumSamples() - 1)
            {
                voice.reset();
                break;
            }

            // Linear interpolation for smoother pitch shifting
            float frac = floatPos - intPos;
            float sample1 = sample->getSample(0, intPos);
            float sample2 = sample->getSample(0, juce::jmin(intPos + 1, sample->getNumSamples() - 1));
            float sampleValue = sample1 + frac * (sample2 - sample1);

            sampleValue *= voice.velocity * envelopeValue;

            // Add to output buffer
            buffer.addSample(0, i, sampleValue);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, i, sampleValue);

            voice.samplePosition++;
        }
    }

    // Update envelope value for UI
    currentEnvelopeValue.store(maxEnvValue);
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

void GabbermasterAudioProcessor::loadCustomSample(const juce::File& file)
{
    sampleManager.loadCustomSample(file, formatManager);
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
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GabbermasterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GabbermasterAudioProcessor();
}
