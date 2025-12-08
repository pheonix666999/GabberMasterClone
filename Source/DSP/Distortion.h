#pragma once

#include <JuceHeader.h>

class Distortion
{
public:
    Distortion();
    
    void prepare(double sampleRate, int samplesPerBlock);
    float processSample(float input, float drive, int mode, float bitCrush, float saturate);
    
private:
    double currentSampleRate = 44100.0;
    
    // Distortion modes
    float softClip(float input, float drive);
    float hardClip(float input, float drive);
    float fuzz(float input, float drive);
    float decimator(float input, float drive);
    
    // Bit crusher
    float appliBitCrusher(float input, float bits);
    
    // Saturation
    float applySaturation(float input, float amount);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion)
};