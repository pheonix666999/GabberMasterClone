#include "KickLimiter.h"
#include <cmath>

KickLimiter::KickLimiter()
{
}

void KickLimiter::prepare(double sampleRate, int samplesPerBlock)
{
    reset();
}

void KickLimiter::reset()
{
}

float KickLimiter::processSample(float input)
{
    // Apply gain
    float output = input * outputGainLinear;
    
    // Apply limiter if enabled
    if (limiterEnabled)
    {
        output = softClipLimit(output);
    }
    
    return output;
}

void KickLimiter::processBlock(juce::AudioBuffer<float>& buffer)
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

float KickLimiter::softClipLimit(float input)
{
    // Soft clipping to prevent >0 dBFS.
    // Monotonic, symmetric, unity-slope at 0.
    return std::tanh(input);
}



