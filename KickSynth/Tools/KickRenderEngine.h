#pragma once

#include <JuceHeader.h>
#include "KickParams.h"
#include "../Source/DSP/KickVoice.h"
#include "../Source/DSP/KickDistortion.h"
#include "../Source/DSP/KickLimiter.h"

class KickRenderEngine
{
public:
    void prepare(double sampleRate);
    void render(const KickParams& params, float velocity, juce::AudioBuffer<float>& buffer);

private:
    double currentSampleRate = 48000.0;
    KickVoice voice;
    KickDistortion distortion;
    KickLimiter limiter;
    juce::dsp::IIR::Filter<float> outputHPF;
    juce::dsp::IIR::Coefficients<float>::Ptr outputHPFCoeffs;
};
