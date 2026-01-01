#include "KickVoice.h"
#include <algorithm>
#include <cmath>

KickVoice::KickVoice()
{
    // Initialize click HPF
    clickHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, 2000.0f);
    clickHPF.coefficients = clickHPFCoeffs;
}

void KickVoice::noteOn(int noteNum, float vel, double sr)
{
    noteNumber = noteNum;
    sampleRate = sr;
    this->timeSinceNoteOn = 0.0;
    this->active = true;
    velocityScale = 1.0f + velocitySensitivity * (juce::jlimit(0.0f, 1.0f, vel) - 1.0f);
    velocityScale = juce::jlimit(0.25f, 3.0f, velocityScale);
    
    // Reset oscillators
    bodyPhase = 0.0f;
    clickPhase = 0.0f;
    clickNoiseState = juce::Random::getSystemRandom().nextFloat();
    
    // Update click HPF sample rate
    clickHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, clickHPFHz);
    clickHPF.coefficients = clickHPFCoeffs;
    clickHPF.reset();
    
    // Initialize envelopes
    currentPitchHz = pitchStartHz;
    ampEnvValue = 0.0f;
    pitchEnvValue = 1.0f;
}

void KickVoice::noteOff()
{
    if (retriggerMode)
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
    float output = (bodySample + clickSample) * velocityScale;
    
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
    float filtered = clickHPF.processSample(noise);
    
    return filtered * decayEnv;
}

void KickVoice::updatePitchEnvelope()
{
    // Exponential sweep: pitch(t) = pitchEnd + (pitchStart - pitchEnd) * exp(-t/tau)
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
    const double timeMs = timeSinceNoteOn * 1000.0;

    // Attack phase (linear ramp to full scale).
    if (timeMs < attackMsValue)
    {
        if (attackMsValue > 0.0f)
            ampEnvValue = static_cast<float>(timeMs / attackMsValue);
        else
            ampEnvValue = 1.0f;
        return;
    }

    // Decay phase: a piecewise log-linear envelope that hits the measurable timing points exactly.
    //
    // Definitions (as measured by the analyzer):
    // - t12_ms: time from peak to -12 dB (A = 10^(-12/20))
    // - t24_ms: time from peak to -24 dB (A = 10^(-24/20))
    // - tail_ms_to_-60db: time from onset (note-on) to -60 dB (A = 10^(-60/20) = 0.001)
    //
    // The synth's decay starts after the attack ramp has reached its peak, so the -60 dB time from
    // peak is (tail - attack). We clamp/repair invalid orderings to keep the envelope stable.
    const double decayMs = timeMs - (double)attackMsValue;
    const double t12 = std::max(0.0, (double)t12MsValue);
    const double t24 = std::max(t12 + 1e-3, (double)t24MsValue);
    const double t60FromPeak = std::max(t24 + 1e-3, (double)tailMsToMinus60Db - (double)attackMsValue);

    constexpr double ln10 = 2.302585092994046;
    constexpr double logA12 = -0.6 * ln10;  // ln(10^(-12/20))
    constexpr double logA24 = -1.2 * ln10;  // ln(10^(-24/20))
    constexpr double logA60 = -3.0 * ln10;  // ln(10^(-60/20)) = ln(0.001)

    double logEnv = 0.0;
    if (decayMs <= 0.0)
    {
        logEnv = 0.0;
    }
    else if (t12 > 0.0 && decayMs < t12)
    {
        const double u = decayMs / t12;
        logEnv = u * logA12;
    }
    else if (decayMs < t24)
    {
        const double u = (decayMs - t12) / (t24 - t12);
        logEnv = logA12 + u * (logA24 - logA12);
    }
    else
    {
        const double u = (decayMs - t24) / (t60FromPeak - t24);
        logEnv = logA24 + u * (logA60 - logA24);
    }

    ampEnvValue = (float)std::exp(logEnv);
    ampEnvValue = juce::jlimit(0.0f, 1.0f, ampEnvValue);
}

float KickVoice::exponentialDecay(float timeMs, float tauMs)
{
    if (tauMs <= 0.0f)
        return 0.0f;
    
    return std::exp(-timeMs / tauMs);
}

