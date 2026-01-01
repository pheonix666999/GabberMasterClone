#pragma once

#include "../JuceHeader.h"

/**
 * KickLimiter - Output limiter/soft clip to prevent >0 dBFS
 */
class KickLimiter
{
public:
    KickLimiter();
    ~KickLimiter() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process single sample
    float processSample(float input);
    
    // Process buffer
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setLimiterEnabled(bool enabled) { limiterEnabled = enabled; }
    void setOutputGain(float gainDb) { outputGainLinear = juce::Decibels::decibelsToGain(gainDb); }
    
private:
    bool limiterEnabled = true;
    float outputGainLinear = 1.0f;
    
    // Soft clip limiter
    float softClipLimit(float input);
};

