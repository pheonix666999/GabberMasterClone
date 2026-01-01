#include "KickDistortion.h"
#include <cmath>

KickDistortion::KickDistortion()
{
}

void KickDistortion::prepare(double sampleRate, int samplesPerBlock)
{
    reset();
}

void KickDistortion::reset()
{
}

float KickDistortion::processSample(float input)
{
    if (drive <= 0.000001f)
        return input;

    // Map drive: 0-1 -> 1-10x
    const float driveAmount = 1.0f + drive * 9.0f;
    const float driven = input * driveAmount;
    
    // Apply distortion
    float output = 0.0f;
    if (distortionType == 0) // Tanh soft clip
    {
        output = tanhSoftClip(driven);
    }
    else if (distortionType == 1) // Hard clip
    {
        output = hardClip(driven);
    }
    else // Asymmetric soft clip
    {
        output = asymmetricSoftClip(driven);
    }
    
    return output;
}

void KickDistortion::processBlock(juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = processSample(channelData[sample]);
        }
    }
}

float KickDistortion::tanhSoftClip(float input)
{
    // Soft clipping using tanh
    return std::tanh(input);
}

float KickDistortion::asymmetricSoftClip(float input)
{
    // Asymmetric soft clipping
    // Map asymmetry: 0-1 -> 0-0.5
    float asymAmount = asymmetry * 0.5f;
    
    if (input > 0.0f)
    {
        // Positive: tanh with slight asymmetry
        return std::tanh(input * (1.0f + asymAmount));
    }
    else
    {
        // Negative: more aggressive clipping
        return std::tanh(input * (1.0f - asymAmount * 2.0f));
    }
}

float KickDistortion::hardClip(float input)
{
    return juce::jlimit(-1.0f, 1.0f, input);
}



