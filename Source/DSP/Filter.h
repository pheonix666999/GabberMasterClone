// ============================================================================
// DSP/Filter.h
// ============================================================================
#pragma once

#include <JuceHeader.h>

class Filter
{
public:
    enum class FilterType
    {
        HighPass = 0,
        LowPass = 1,
        BandPass = 2
    };

    Filter();

    void prepare(double sampleRate, int samplesPerBlock);
    float processSample(float input, float cutoffFreq, float resonance, FilterType type);
    float processSampleWithTracking(float input, float cutoffFreq, float resonance,
                                     FilterType type, int midiNote, float trackAmount);
    void reset();

private:
    double currentSampleRate = 44100.0;

    // Filter state variables
    float z1 = 0.0f, z2 = 0.0f;

    // Last parameters for optimization
    float lastCutoffFreq = 1000.0f;
    float lastResonance = 0.0f;
    FilterType lastType = FilterType::LowPass;

    // Filter coefficients
    struct FilterCoeffs
    {
        float b0, b1, b2, a1, a2;
    };

    FilterCoeffs coeffs;

    void calculateCoeffs(float freq, float q, FilterType type);
    float processFilter(float input);

    // Legacy HPF/LPF for backwards compatibility
    float hpf_z1 = 0.0f, hpf_z2 = 0.0f;
    float lpf_z1 = 0.0f, lpf_z2 = 0.0f;
    float lastHpfFreq = 20.0f;
    float lastLpfFreq = 20000.0f;
    FilterCoeffs hpfCoeffs;
    FilterCoeffs lpfCoeffs;

    void calculateHighPassCoeffs(float freq, float q);
    void calculateLowPassCoeffs(float freq, float q);
    float processFilterOld(float input, const FilterCoeffs& c, float& z1_ref, float& z2_ref);

public:
    // Legacy method for backwards compatibility
    float processSample(float input, float hpfFreq, float lpfFreq, float resonance);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Filter)
};
