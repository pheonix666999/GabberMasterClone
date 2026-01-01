#include "KickVoice.h"
#include <JuceHeader.h>
#include <cmath>
#include <cstdint>
#include <random>

KickVoice::KickVoice()
{
    // Initialize click HPF
    clickHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, 2000.0f);
    if (clickHPFCoeffs)
        clickHPF.coefficients = clickHPFCoeffs;
}

void KickVoice::noteOn(int noteNum, float vel, double sr)
{
    noteNumber = noteNum;
    sampleRate = sr;
    timeSinceNoteOn = 0.0;
    active = true;
    
    // Reset oscillators
    bodyPhase = 0.0f;
    clickPhase = 0.0f;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    clickNoiseState = dis(gen);
    
    // Update click HPF sample rate
    clickHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, clickHPFHz);
    if (clickHPFCoeffs)
    {
        clickHPF.coefficients = clickHPFCoeffs;
        clickHPF.reset();
    }
    
    // Initialize envelopes - kicks need instant attack, start at full amplitude
    currentPitchHz = pitchStartHz;
    ampEnvValue = 1.0f;  // Start at full amplitude for punchy kick attack
    pitchEnvValue = 1.0f;
}

void KickVoice::noteOff()
{
    active = false;
}

float KickVoice::renderSample()
{
    if (!active)
        return 0.0f;
    
    // Update time
    timeSinceNoteOn += 1.0 / sampleRate;
    double timeMs = timeSinceNoteOn * 1000.0;
    
    // Update envelopes
    updatePitchEnvelope();
    updateAmpEnvelope();
    
    // Generate body
    float bodySample = generateBodySample() * bodyLevel * ampEnvValue;
    
    // Generate click
    float clickSample = generateClickSample() * clickLevel;
    
    // Combine
    float output = bodySample + clickSample;
    
    // Check if voice should stop (tail reached -60dB or max duration)
    if (ampEnvValue < 0.001f || timeMs > tailMsToMinus60Db * 2.0)
    {
        if (!retriggerMode)
            active = false;
    }
    
    return output;
}

float KickVoice::generateBodySample()
{
    // Calculate phase increment based on current pitch
    float freqHz = currentPitchHz;
    
    // Apply key tracking
    if (keyTrackingSemitones != 0.0f)
    {
        float noteOffset = (noteNumber - 60.0f) + keyTrackingSemitones;
        freqHz *= std::pow(2.0f, noteOffset / 12.0f);
    }
    
    float phaseInc = freqHz / static_cast<float>(sampleRate);
    
    // Generate waveform
    float sample = 0.0f;
    if (bodyOscType == 0) // Sine
    {
        sample = std::sin(bodyPhase * juce::MathConstants<float>::twoPi);
    }
    else // Triangle
    {
        float triPhase = std::fmod(bodyPhase, 1.0f);
        if (triPhase < 0.5f)
            sample = 4.0f * triPhase - 1.0f;
        else
            sample = 3.0f - 4.0f * triPhase;
    }
    
    // Update phase
    bodyPhase += phaseInc;
    if (bodyPhase >= 1.0f)
        bodyPhase -= 1.0f;
    
    return sample;
}

float KickVoice::generateClickSample()
{
    // Generate noise burst with exponential decay
    double timeMs = timeSinceNoteOn * 1000.0;
    
    if (timeMs > clickDecayMs * 5.0) // Click has decayed
        return 0.0f;
    
    // Generate white noise using LCG
    uint32_t state = static_cast<uint32_t>(clickNoiseState * 2147483647.0f);
    state = state * 1103515245u + 12345u;
    clickNoiseState = static_cast<float>(state & 0x7FFFFFFFu) / 2147483647.0f;
    float noise = clickNoiseState * 2.0f - 1.0f;
    
    // Apply exponential decay
    float decayEnv = exponentialDecay(static_cast<float>(timeMs), clickDecayMs);
    
    // Apply HPF for 2k-10k emphasis
    float filtered = noise;
    if (this->clickHPFCoeffs.get() != nullptr)
    {
        filtered = this->clickHPF.processSample(noise);
    }
    
    return filtered * decayEnv;
}

void KickVoice::updatePitchEnvelope()
{
    // Exponential sweep: pitch(t) = pitchEnd + (pitchStart - pitchEnd) * exp(-t/tau)
    double timeMs = timeSinceNoteOn * 1000.0;
    float tauSamples = pitchTauMs / 1000.0f * static_cast<float>(sampleRate);
    
    if (tauSamples > 0.0f)
    {
        float expValue = std::exp(-static_cast<float>(timeSinceNoteOn) / (tauSamples / static_cast<float>(sampleRate)));
        currentPitchHz = pitchEndHz + (pitchStartHz - pitchEndHz) * expValue;
        pitchEnvValue = expValue;
    }
    else
    {
        currentPitchHz = pitchEndHz;
        pitchEnvValue = 0.0f;
    }
}

void KickVoice::updateAmpEnvelope()
{
    double timeMs = timeSinceNoteOn * 1000.0;

    // For kicks, skip attack phase if attack is very short (< 1ms) - instant attack
    // Attack phase only used for slow/pad sounds
    if (attackMsValue >= 1.0f && timeMs < attackMsValue)
    {
        ampEnvValue = static_cast<float>(timeMs / attackMsValue);
        return;
    }

    // Decay phase - exponential decay
    // We need to match t12_ms, t24_ms, and tail_ms_to_-60db
    // Use the most restrictive constraint

    float decayTauMs = computeDecayTauFromTail(tailMsToMinus60Db);

    // Verify t12 and t24 constraints
    float t12Tau = computeDecayTauFromT12(t12MsValue);
    float t24Tau = computeDecayTauFromT24(t24MsValue);

    // Use the smallest tau (most restrictive)
    decayTauMs = juce::jmin(decayTauMs, t12Tau, t24Tau);

    // Decay starts from note onset (or after attack if attack > 1ms)
    float decayStartMs = (attackMsValue >= 1.0f) ? attackMsValue : 0.0f;
    float decayTimeMs = static_cast<float>(timeMs) - decayStartMs;
    ampEnvValue = exponentialDecay(decayTimeMs, decayTauMs);
}

float KickVoice::exponentialDecay(float timeMs, float tauMs)
{
    if (tauMs <= 0.0f)
        return 0.0f;
    
    return std::exp(-timeMs / tauMs);
}

float KickVoice::computeDecayTauFromT12(float t12Ms)
{
    // -12dB = 0.25 amplitude
    // exp(-t12/tau) = 0.25
    // -t12/tau = ln(0.25)
    // tau = -t12 / ln(0.25)
    if (t12Ms <= 0.0f)
        return 1.0f;
    return -t12Ms / std::log(0.25f);
}

float KickVoice::computeDecayTauFromT24(float t24Ms)
{
    // -24dB = 0.063 amplitude
    // exp(-t24/tau) = 0.063
    // tau = -t24 / ln(0.063)
    if (t24Ms <= 0.0f)
        return 1.0f;
    return -t24Ms / std::log(0.063f);
}

float KickVoice::computeDecayTauFromTail(float tailMs)
{
    // -60dB = 0.001 amplitude
    // exp(-tail/tau) = 0.001
    // tau = -tail / ln(0.001)
    if (tailMs <= 0.0f)
        return 1.0f;
    return -tailMs / std::log(0.001f);
}

