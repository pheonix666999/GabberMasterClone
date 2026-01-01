#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
GabbermasterAudioProcessor::GabbermasterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    for (auto& voice : voices)
        voice.reset();

    setCurrentProgram(0);
}

GabbermasterAudioProcessor::~GabbermasterAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GabbermasterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ============ CANONICAL PARAMETERS ONLY ============

    // Distortion
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DistPostEQ", "Dist Post EQ",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DistPreEQ", "Dist Pre EQ",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 1.0f));

    // EQ Band 1-4
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band1-Freq", "EQ Band 1 Freq",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.61638f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band1-Gain", "EQ Band 1 Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.5625f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band1-Q", "EQ Band 1 Q",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.30862f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band2-Freq", "EQ Band 2 Freq",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.84483f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band2-Gain", "EQ Band 2 Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band2-Q", "EQ Band 2 Q",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.11034f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band3-Freq", "EQ Band 3 Freq",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.40948f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band3-Gain", "EQ Band 3 Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band3-Q", "EQ Band 3 Q",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.51552f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band4-Freq", "EQ Band 4 Freq",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.00431f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band4-Gain", "EQ Band 4 Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "EQ-Band4-Q", "EQ Band 4 Q",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 0.36034f));

    // EQ Mode and On/Off
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "EQ_EQMode", "EQ Mode",
        juce::StringArray{"Fast", "Slow"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "EQ_EQON", "EQ On", true));

    // Envelope ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Envelope-ADSRAttack", "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Envelope-ADSRDecay", "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.7625f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Envelope-ADSRSustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.1375f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Envelope-ADSRRelease", "Release",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Envelope-ADSRVolume", "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f), 1.0f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "Filter-FilterType", "Filter Type",
        juce::StringArray{"LP", "HP", "BP"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Filter-FilterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Filter-FilterEnvelope", "Filter Envelope",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Filter-FilterQ", "Filter Q",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Filter-FilterTrack", "Filter Track",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0625f));

    // KickSelector - 8 modes
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "KickSelector", "Kick Mode",
        juce::StringArray{"Viper", "Noise", "Bounce", "Rotterdam", "Mutha", "Stompin", "Merrik", "Massive"}, 0));

    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Reverb-ReverbDamp", "Reverb Damp",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.0375f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Reverb-ReverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.175f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "Reverb-ReverbOn", "Reverb On", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Reverb-ReverbRoom", "Reverb Room",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 0.6375f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Reverb-ReverbWidth", "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0125f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String GabbermasterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

double GabbermasterAudioProcessor::getTailLengthSeconds() const
{
    return 3.0;
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

    // Load "Fag Tag" preset - EXACT values as specified
    if (auto* p = apvts.getParameter("DistPostEQ")) p->setValueNotifyingHost(1.0f);
    if (auto* p = apvts.getParameter("DistPreEQ")) p->setValueNotifyingHost(1.0f);

    if (auto* p = apvts.getParameter("EQ-Band1-Freq")) p->setValueNotifyingHost(0.61638f);
    if (auto* p = apvts.getParameter("EQ-Band1-Gain")) p->setValueNotifyingHost(0.5625f);
    if (auto* p = apvts.getParameter("EQ-Band1-Q")) p->setValueNotifyingHost(0.30862f);

    if (auto* p = apvts.getParameter("EQ-Band2-Freq")) p->setValueNotifyingHost(0.84483f);
    if (auto* p = apvts.getParameter("EQ-Band2-Gain")) p->setValueNotifyingHost(0.2f);
    if (auto* p = apvts.getParameter("EQ-Band2-Q")) p->setValueNotifyingHost(0.11034f);

    if (auto* p = apvts.getParameter("EQ-Band3-Freq")) p->setValueNotifyingHost(0.40948f);
    if (auto* p = apvts.getParameter("EQ-Band3-Gain")) p->setValueNotifyingHost(1.0f);
    if (auto* p = apvts.getParameter("EQ-Band3-Q")) p->setValueNotifyingHost(0.51552f);

    if (auto* p = apvts.getParameter("EQ-Band4-Freq")) p->setValueNotifyingHost(0.00431f);
    if (auto* p = apvts.getParameter("EQ-Band4-Gain")) p->setValueNotifyingHost(1.0f);
    if (auto* p = apvts.getParameter("EQ-Band4-Q")) p->setValueNotifyingHost(0.36034f);

    if (auto* p = apvts.getParameter("EQ_EQMode")) p->setValueNotifyingHost(1.0f); // Slow
    if (auto* p = apvts.getParameter("EQ_EQON")) p->setValueNotifyingHost(1.0f);

    if (auto* p = apvts.getParameter("Envelope-ADSRAttack")) p->setValueNotifyingHost(0.0f);
    if (auto* p = apvts.getParameter("Envelope-ADSRDecay")) p->setValueNotifyingHost(0.7625f);
    if (auto* p = apvts.getParameter("Envelope-ADSRSustain")) p->setValueNotifyingHost(0.1375f);
    if (auto* p = apvts.getParameter("Envelope-ADSRRelease")) p->setValueNotifyingHost(0.0f);
    if (auto* p = apvts.getParameter("Envelope-ADSRVolume")) p->setValueNotifyingHost(1.0f);

    if (auto* p = apvts.getParameter("Filter-FilterType")) p->setValueNotifyingHost(1.0f); // BP
    if (auto* p = apvts.getParameter("Filter-FilterCutoff")) p->setValueNotifyingHost(0.0f);
    if (auto* p = apvts.getParameter("Filter-FilterEnvelope")) p->setValueNotifyingHost(0.0f);
    if (auto* p = apvts.getParameter("Filter-FilterQ")) p->setValueNotifyingHost(0.0f);
    if (auto* p = apvts.getParameter("Filter-FilterTrack")) p->setValueNotifyingHost(0.0625f);

    if (auto* p = apvts.getParameter("KickSelector")) p->setValueNotifyingHost(0.0f); // Viper

    if (auto* p = apvts.getParameter("Reverb-ReverbDamp")) p->setValueNotifyingHost(0.0375f);
    if (auto* p = apvts.getParameter("Reverb-ReverbMix")) p->setValueNotifyingHost(0.175f);
    if (auto* p = apvts.getParameter("Reverb-ReverbOn")) p->setValueNotifyingHost(1.0f);
    if (auto* p = apvts.getParameter("Reverb-ReverbRoom")) p->setValueNotifyingHost(0.6375f);
    if (auto* p = apvts.getParameter("Reverb-ReverbWidth")) p->setValueNotifyingHost(1.0f);
}

const juce::String GabbermasterAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return "Fag Tag";
}

void GabbermasterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void GabbermasterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    for (auto& voice : voices)
    {
        voice.reset();
        voice.maxDurationSamples = static_cast<int>(sampleRate * 2.5); // 2.5 seconds max
    }

    filterState1L = filterState2L = 0.0f;
    filterState1R = filterState2R = 0.0f;
    postLpfStateL = postLpfStateR = 0.0f;
}

void GabbermasterAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GabbermasterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

//==============================================================================
void GabbermasterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // Process MIDI
    bool hadMidi = false;
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            hadMidi = true;
            int note = msg.getNoteNumber();
            float vel = msg.getFloatVelocity();
            startVoice(note, vel);

            int activeCount = 0;
            for (const auto& v : voices) if (v.isActive) activeCount++;
            DBG(">>> MIDI NoteOn: note=" << note << " vel=" << vel << " active=" << activeCount);
        }
        else if (msg.isNoteOff())
        {
            hadMidi = true;
            stopVoice(msg.getNoteNumber());
            DBG(">>> MIDI NoteOff: note=" << msg.getNoteNumber());
        }
    }

    // Debug: log if no MIDI (once per second)
    if (!hadMidi)
    {
        bool anyActive = false;
        for (const auto& v : voices) if (v.isActive) { anyActive = true; break; }

        if (!anyActive)
        {
            int64_t now = juce::Time::currentTimeMillis();
            if (now - lastNoMidiLogTime > 1000)
            {
                lastNoMidiLogTime = now;
                DBG("NO MIDI RECEIVED");
            }
        }
    }

    renderVoices(buffer);
}

//==============================================================================
void GabbermasterAudioProcessor::startVoice(int noteNumber, float velocity)
{
    Voice* voiceToUse = nullptr;

    for (auto& voice : voices)
    {
        if (!voice.isActive)
        {
            voiceToUse = &voice;
            break;
        }
    }

    if (voiceToUse == nullptr)
        voiceToUse = &voices[0]; // Steal oldest

    // Get mode parameters (internal engine constants)
    int kickMode = static_cast<int>(apvts.getRawParameterValue("KickSelector")->load());
    float modeStartPitch, modeEndPitch, pitchDecayMs, clickFreq, clickAmp;
    getKickModeParams(kickMode, modeStartPitch, modeEndPitch, pitchDecayMs, clickFreq, clickAmp);

    // ============================================================
    // SAMPLE-BASED PITCH: MIDI note DIRECTLY controls playback pitch
    // Like resampling - higher notes play the "sample" faster
    // Reference note 48 (C3) produces the mode's base frequencies
    // ============================================================
    float pitchRatio = std::pow(2.0f, static_cast<float>(noteNumber - referenceNote) / 12.0f);

    // Apply pitch ratio to oscillator frequencies
    // This is the core sample-based behavior
    float startFreq = modeStartPitch * pitchRatio;
    float endFreq = modeEndPitch * pitchRatio;

    // ============================================================
    // ATTACK: Scales inversely with pitch (sample-based resampling effect)
    // When you play a sample faster, the attack is proportionally shorter
    // Target: 16.4ms at low pitch, 1.7ms at high pitch
    // Pitch ratio ~10x should give attack ratio ~10x (linear scaling)
    // ============================================================
    // Base attack tau at reference note: ~7ms gives 10-90% rise of ~16ms
    float baseAttackTau = 0.007f;
    float attackTau = baseAttackTau / pitchRatio;
    attackTau = juce::jlimit(0.0003f, 0.05f, attackTau);

    // ============================================================
    // DECAY/T60: Scales with pitch but not linearly
    // Target: T60 = 1.254s at low pitch, 0.794s at high pitch
    // Ratio: 1.254/0.794 = 1.58 for pitch ratio of ~10
    // So decay scales as: decayTau = base / (pitchRatio ^ 0.22)
    // Base T60 at reference = 1.254s -> tau = 1.254/6.9 = 0.182s
    // ============================================================
    float baseDecayTau = 0.182f; // T60 ≈ 1.25s at reference
    float decayTau = baseDecayTau / std::pow(std::max(pitchRatio, 0.1f), 0.22f);
    decayTau = juce::jlimit(0.05f, 0.5f, decayTau);

    // Pitch decay (sweep time) also scales with pitch ratio
    // Faster playback = faster sweep
    float pitchDecayTau = (pitchDecayMs / 1000.0f) / pitchRatio;
    pitchDecayTau = juce::jlimit(0.001f, 0.05f, pitchDecayTau);

    // Click transient decay - scales with pitch
    float clickDecayTau = 0.001f / pitchRatio;
    clickDecayTau = juce::jlimit(0.0001f, 0.005f, clickDecayTau);

    // Initialize voice
    voiceToUse->isActive = true;
    voiceToUse->noteNumber = noteNumber;
    voiceToUse->velocity = velocity;
    voiceToUse->phase = 0.0f;
    voiceToUse->clickPhase = 0.0f;
    voiceToUse->startFreq = startFreq;
    voiceToUse->currentFreq = startFreq;
    voiceToUse->targetFreq = endFreq;
    voiceToUse->envLevel = 0.0f;
    voiceToUse->clickEnvLevel = 1.0f;
    voiceToUse->samplesSinceNoteOn = 0;
    voiceToUse->attackTau = attackTau;
    voiceToUse->decayTau = decayTau;
    voiceToUse->pitchDecayTau = pitchDecayTau;
    voiceToUse->clickDecayTau = clickDecayTau;
    voiceToUse->pitchRatio = pitchRatio;
    voiceToUse->maxDurationSamples = static_cast<int>(currentSampleRate * 2.5);
}

void GabbermasterAudioProcessor::stopVoice(int noteNumber)
{
    // For kicks, note-off doesn't immediately kill - let decay continue
    // Just mark for slightly faster release if needed
    for (auto& voice : voices)
    {
        if (voice.isActive && voice.noteNumber == noteNumber)
        {
            // Optional: slightly speed up decay on note-off
            // voice.decayTau *= 0.8f;
        }
    }
}

void GabbermasterAudioProcessor::triggerKick(int noteNumber, float velocity)
{
    // Called from GUI GO button - triggers a kick without MIDI
    startVoice(noteNumber, velocity);
    DBG(">>> GUI Trigger: note=" << noteNumber << " vel=" << velocity);
}

void GabbermasterAudioProcessor::getKickModeParams(int mode, float& startPitchHz, float& endPitchHz,
                                                    float& pitchDecayMs, float& clickFreq, float& clickAmp)
{
    // Internal engine constants - NOT exposed as parameters
    // These are the characteristic values for each kick mode
    switch (mode)
    {
        case 0: // Viper - classic gabber
            startPitchHz = 164.0f;
            endPitchHz = 23.2f;
            pitchDecayMs = 6.76f;
            clickFreq = 2500.0f;
            clickAmp = 0.3f;
            break;
        case 1: // Noise
            startPitchHz = 180.0f;
            endPitchHz = 25.0f;
            pitchDecayMs = 8.0f;
            clickFreq = 3000.0f;
            clickAmp = 0.4f;
            break;
        case 2: // Bounce
            startPitchHz = 200.0f;
            endPitchHz = 28.0f;
            pitchDecayMs = 10.0f;
            clickFreq = 2000.0f;
            clickAmp = 0.25f;
            break;
        case 3: // Rotterdam
            startPitchHz = 170.0f;
            endPitchHz = 24.0f;
            pitchDecayMs = 7.0f;
            clickFreq = 2800.0f;
            clickAmp = 0.35f;
            break;
        case 4: // Mutha
            startPitchHz = 150.0f;
            endPitchHz = 21.0f;
            pitchDecayMs = 12.0f;
            clickFreq = 1800.0f;
            clickAmp = 0.2f;
            break;
        case 5: // Stompin
            startPitchHz = 190.0f;
            endPitchHz = 26.0f;
            pitchDecayMs = 5.0f;
            clickFreq = 3500.0f;
            clickAmp = 0.45f;
            break;
        case 6: // Merrik
            startPitchHz = 220.0f;
            endPitchHz = 30.0f;
            pitchDecayMs = 6.0f;
            clickFreq = 3200.0f;
            clickAmp = 0.4f;
            break;
        case 7: // Massive
        default:
            startPitchHz = 160.0f;
            endPitchHz = 22.0f;
            pitchDecayMs = 15.0f;
            clickFreq = 1500.0f;
            clickAmp = 0.15f;
            break;
    }
}

void GabbermasterAudioProcessor::renderVoices(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float sampleRate = static_cast<float>(currentSampleRate);
    const float dt = 1.0f / sampleRate;

    // Get parameters
    float volumeNorm = apvts.getRawParameterValue("Envelope-ADSRVolume")->load();
    float filterCutoffNorm = apvts.getRawParameterValue("Filter-FilterCutoff")->load();
    int filterType = static_cast<int>(apvts.getRawParameterValue("Filter-FilterType")->load());
    float filterQ = apvts.getRawParameterValue("Filter-FilterQ")->load();
    float filterEnvelope = apvts.getRawParameterValue("Filter-FilterEnvelope")->load();

    // Get click parameters (base values - will be scaled by voice pitchRatio)
    int kickMode = static_cast<int>(apvts.getRawParameterValue("KickSelector")->load());
    float modeStartPitch, modeEndPitch, pitchDecayMs, baseClickFreq, clickAmp;
    getKickModeParams(kickMode, modeStartPitch, modeEndPitch, pitchDecayMs, baseClickFreq, clickAmp);

    // Determine if filter should be bypassed
    // CRITICAL: When cutoff=0, envelope=0, Q=0, the original still outputs full bass
    // So we bypass/open the filter in this case
    bool filterBypass = (filterCutoffNorm < 0.01f && filterEnvelope < 0.01f && filterQ < 0.01f);

    // Master output gain - target peak of -0.09 dBFS (0.99)
    // The output chain: oscillator (max ~1.16) * envelope * velocity * volume * distortion * gain
    // Need final output to peak at ~0.99
    const float masterGain = 4.5f;

    for (auto& voice : voices)
    {
        if (!voice.isActive)
            continue;

        // Pre-calculate exponential coefficients
        float attackCoef = 1.0f - std::exp(-dt / voice.attackTau);
        float decayCoef = 1.0f - std::exp(-dt / voice.decayTau);
        float pitchCoef = 1.0f - std::exp(-dt / voice.pitchDecayTau);
        float clickCoef = 1.0f - std::exp(-dt / voice.clickDecayTau);

        for (int i = 0; i < numSamples; ++i)
        {
            // === ENVELOPE ===
            // Fast exponential attack, then exponential decay
            if (voice.envLevel < 0.999f && voice.samplesSinceNoteOn < static_cast<int>(sampleRate * 0.05f))
            {
                // Attack phase
                voice.envLevel += (1.0f - voice.envLevel) * attackCoef;
            }
            else
            {
                // Decay phase - exponential decay toward 0
                voice.envLevel *= (1.0f - decayCoef);
            }

            // Click envelope: fast exponential decay
            voice.clickEnvLevel *= (1.0f - clickCoef);

            // === PITCH SWEEP ===
            voice.currentFreq += (voice.targetFreq - voice.currentFreq) * pitchCoef;

            // === MAIN OSCILLATOR ===
            // Use SINE as fundamental with controlled odd harmonics
            // Target ratios from original_left analysis:
            // H3 ≈ -17.4 dB (0.135), H5 ≈ -33.5 dB (0.021), H7 ≈ -42.4 dB (0.0076)
            float phase2pi = voice.phase * juce::MathConstants<float>::twoPi;

            float osc = std::sin(phase2pi);                       // Fundamental
            osc += 0.135f * std::sin(3.0f * phase2pi);           // H3
            osc += 0.021f * std::sin(5.0f * phase2pi);           // H5
            osc += 0.0076f * std::sin(7.0f * phase2pi);          // H7

            // === CLICK TRANSIENT ===
            // Click frequency scales with pitch ratio (sample-based behavior)
            float scaledClickFreq = baseClickFreq * voice.pitchRatio;
            float click = std::sin(voice.clickPhase * juce::MathConstants<float>::twoPi);
            click *= clickAmp * voice.clickEnvLevel;

            // === MIX ===
            float sample = osc * voice.envLevel + click;

            // Apply velocity and volume
            sample *= voice.velocity * volumeNorm;

            // === DISTORTION (Gabber character) ===
            // Soft saturation for warmth
            float drive = 1.3f;
            sample = std::tanh(sample * drive);

            // === POST LPF ===
            // Cutoff scales with pitch for natural high-frequency rolloff
            float postLpfHz = 1500.0f + 20.0f * voice.currentFreq;
            postLpfHz = juce::jlimit(600.0f, 18000.0f, postLpfHz);
            sample = processPostLPF(sample, postLpfHz, true);

            // === USER FILTER ===
            sample = processFilter(sample, filterCutoffNorm, filterType, filterQ,
                                   filterEnvelope, voice.envLevel, filterBypass, true);

            // === OUTPUT GAIN ===
            sample *= masterGain;

            // === FINAL LIMITER ===
            // Target peak of -0.09 dBFS (0.99)
            // Use soft limiting to prevent harsh clipping
            if (sample > 0.85f)
                sample = 0.85f + 0.14f * std::tanh((sample - 0.85f) / 0.14f);
            else if (sample < -0.85f)
                sample = -0.85f + 0.14f * std::tanh((sample + 0.85f) / 0.14f);
            sample = juce::jlimit(-0.99f, 0.99f, sample);

            // Add to buffer
            buffer.addSample(0, i, sample);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, i, sample);

            // Advance phases
            voice.phase += voice.currentFreq / sampleRate;
            while (voice.phase >= 1.0f) voice.phase -= 1.0f;

            voice.clickPhase += scaledClickFreq / sampleRate;
            while (voice.clickPhase >= 1.0f) voice.clickPhase -= 1.0f;

            voice.samplesSinceNoteOn++;
        }

        // Kill voice when envelope is very low OR max duration exceeded
        // Use very low threshold to allow full T60 decay
        if (voice.envLevel < 0.00001f || voice.samplesSinceNoteOn > voice.maxDurationSamples)
        {
            voice.isActive = false;
        }
    }
}

float GabbermasterAudioProcessor::processPostLPF(float input, float cutoffHz, bool isLeft)
{
    float nyquist = static_cast<float>(currentSampleRate) * 0.49f;
    cutoffHz = std::min(cutoffHz, nyquist);

    float omega = juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float>(currentSampleRate);
    float alpha = omega / (omega + 1.0f);

    float& state = isLeft ? postLpfStateL : postLpfStateR;
    state = state + alpha * (input - state);
    return state;
}

float GabbermasterAudioProcessor::processFilter(float input, float cutoffNorm, int filterType, float Q,
                                                 float envAmount, float envLevel, bool bypass, bool isLeft)
{
    // CRITICAL: When bypass is true (cutoff=0, env=0, Q=0), pass through unchanged
    // This matches the original behaviour where FilterCutoff=0 doesn't mute the sound
    if (bypass)
        return input;

    // Base cutoff calculation
    float baseCutoff = 400.0f; // Base center for BP

    // Apply envelope modulation to cutoff
    float envMod = envAmount * envLevel * 10000.0f;

    float freqHz;
    switch (filterType)
    {
        case 0: // LP
            freqHz = 80.0f + cutoffNorm * 19920.0f + envMod;
            break;
        case 1: // HP
            freqHz = 20.0f + cutoffNorm * 7980.0f + envMod;
            break;
        case 2: // BP
        default:
            // For BP, even at cutoff=0, we want to pass audio
            // Use a scale factor that keeps audio audible
            freqHz = baseCutoff + cutoffNorm * 14600.0f + envMod;
            freqHz = juce::jlimit(80.0f, 15000.0f, freqHz);
            break;
    }

    float nyquist = static_cast<float>(currentSampleRate) * 0.45f;
    freqHz = std::min(freqHz, nyquist);
    freqHz = std::max(freqHz, 20.0f);

    float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float>(currentSampleRate);
    float alpha = omega / (omega + 1.0f);
    float resonance = 1.0f + Q * 2.0f;

    float& state1 = isLeft ? filterState1L : filterState1R;
    float& state2 = isLeft ? filterState2L : filterState2R;

    float output;
    switch (filterType)
    {
        case 0: // LP
            state1 += alpha * (input - state1);
            output = state1;
            break;
        case 1: // HP
            state1 += alpha * (input - state1);
            output = input - state1;
            break;
        case 2: // BP
        default:
            state1 += alpha * (input - state1);
            {
                float hp = input - state1;
                state2 += alpha * (hp - state2);
                output = state2 * resonance;
            }
            break;
    }

    return output;
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

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GabbermasterAudioProcessor();
}
