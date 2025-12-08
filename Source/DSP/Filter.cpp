// ============================================================================
// DSP/Filter.cpp
// ============================================================================
#include "Filter.h"

Filter::Filter()
{
    reset();
}

void Filter::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    reset();
}

void Filter::reset()
{
    hpf_z1 = hpf_z2 = 0.0f;
    lpf_z1 = lpf_z2 = 0.0f;
    z1 = z2 = 0.0f;
}

// New filter processing with type selection
float Filter::processSample(float input, float cutoffFreq, float resonance, FilterType type)
{
    // Clamp frequency to valid range
    cutoffFreq = juce::jlimit(20.0f, 20000.0f, cutoffFreq);

    // Update coefficients if parameters changed
    if (std::abs(cutoffFreq - lastCutoffFreq) > 0.1f ||
        std::abs(resonance - lastResonance) > 0.01f ||
        type != lastType)
    {
        float q = 0.707f + (resonance * 10.0f);
        calculateCoeffs(cutoffFreq, q, type);
        lastCutoffFreq = cutoffFreq;
        lastResonance = resonance;
        lastType = type;
    }

    return processFilter(input);
}

// Filter with keyboard tracking
float Filter::processSampleWithTracking(float input, float cutoffFreq, float resonance,
                                         FilterType type, int midiNote, float trackAmount)
{
    // Calculate frequency offset based on MIDI note (middle C = 60)
    float noteOffset = (midiNote - 60) / 12.0f; // Octaves from middle C
    float trackingMultiplier = std::pow(2.0f, noteOffset * trackAmount);

    float trackedCutoff = cutoffFreq * trackingMultiplier;
    trackedCutoff = juce::jlimit(20.0f, 20000.0f, trackedCutoff);

    return processSample(input, trackedCutoff, resonance, type);
}

void Filter::calculateCoeffs(float freq, float q, FilterType type)
{
    float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(currentSampleRate);
    float sn = std::sin(omega);
    float cs = std::cos(omega);
    float alpha = sn / (2.0f * q);

    float a0 = 1.0f + alpha;

    switch (type)
    {
        case FilterType::HighPass:
            coeffs.b0 = (1.0f + cs) / (2.0f * a0);
            coeffs.b1 = -(1.0f + cs) / a0;
            coeffs.b2 = (1.0f + cs) / (2.0f * a0);
            break;

        case FilterType::LowPass:
            coeffs.b0 = (1.0f - cs) / (2.0f * a0);
            coeffs.b1 = (1.0f - cs) / a0;
            coeffs.b2 = (1.0f - cs) / (2.0f * a0);
            break;

        case FilterType::BandPass:
            coeffs.b0 = alpha / a0;
            coeffs.b1 = 0.0f;
            coeffs.b2 = -alpha / a0;
            break;
    }

    coeffs.a1 = (-2.0f * cs) / a0;
    coeffs.a2 = (1.0f - alpha) / a0;
}

float Filter::processFilter(float input)
{
    // Direct Form II Transposed biquad filter
    float output = coeffs.b0 * input + z1;
    z1 = coeffs.b1 * input - coeffs.a1 * output + z2;
    z2 = coeffs.b2 * input - coeffs.a2 * output;

    return output;
}

// Legacy method for backwards compatibility
float Filter::processSample(float input, float hpfFreq, float lpfFreq, float resonance)
{
    float lastRes = lastResonance;

    // Update coefficients if parameters changed
    if (std::abs(hpfFreq - lastHpfFreq) > 0.1f || std::abs(resonance - lastRes) > 0.01f)
    {
        float q = 0.707f + (resonance * 5.0f);
        calculateHighPassCoeffs(hpfFreq, q);
        lastHpfFreq = hpfFreq;
    }

    if (std::abs(lpfFreq - lastLpfFreq) > 0.1f || std::abs(resonance - lastRes) > 0.01f)
    {
        float q = 0.707f + (resonance * 5.0f);
        calculateLowPassCoeffs(lpfFreq, q);
        lastLpfFreq = lpfFreq;
        lastResonance = resonance;
    }

    float output = input;

    // Apply high-pass filter
    if (hpfFreq > 20.0f)
    {
        output = processFilterOld(output, hpfCoeffs, hpf_z1, hpf_z2);
    }

    // Apply low-pass filter
    if (lpfFreq < 20000.0f)
    {
        output = processFilterOld(output, lpfCoeffs, lpf_z1, lpf_z2);
    }

    return output;
}

void Filter::calculateHighPassCoeffs(float freq, float q)
{
    float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(currentSampleRate);
    float sn = std::sin(omega);
    float cs = std::cos(omega);
    float alpha = sn / (2.0f * q);

    float a0 = 1.0f + alpha;

    hpfCoeffs.b0 = (1.0f + cs) / (2.0f * a0);
    hpfCoeffs.b1 = -(1.0f + cs) / a0;
    hpfCoeffs.b2 = (1.0f + cs) / (2.0f * a0);
    hpfCoeffs.a1 = (-2.0f * cs) / a0;
    hpfCoeffs.a2 = (1.0f - alpha) / a0;
}

void Filter::calculateLowPassCoeffs(float freq, float q)
{
    float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(currentSampleRate);
    float sn = std::sin(omega);
    float cs = std::cos(omega);
    float alpha = sn / (2.0f * q);

    float a0 = 1.0f + alpha;

    lpfCoeffs.b0 = (1.0f - cs) / (2.0f * a0);
    lpfCoeffs.b1 = (1.0f - cs) / a0;
    lpfCoeffs.b2 = (1.0f - cs) / (2.0f * a0);
    lpfCoeffs.a1 = (-2.0f * cs) / a0;
    lpfCoeffs.a2 = (1.0f - alpha) / a0;
}

float Filter::processFilterOld(float input, const FilterCoeffs& c, float& z1_ref, float& z2_ref)
{
    float output = c.b0 * input + z1_ref;
    z1_ref = c.b1 * input - c.a1 * output + z2_ref;
    z2_ref = c.b2 * input - c.a2 * output;

    return output;
}
