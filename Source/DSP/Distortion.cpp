// ============================================================================
// DSP/Distortion.cpp
// ============================================================================
#include "Distortion.h"

Distortion::Distortion()
{
}

void Distortion::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
}

float Distortion::processSample(float input, float drive, int mode, float bitCrush, float saturate)
{
    float output = input;
    
    // Apply distortion mode
    switch (mode)
    {
        case 0: output = softClip(output, drive); break;
        case 1: output = hardClip(output, drive); break;
        case 2: output = fuzz(output, drive); break;
        case 3: output = decimator(output, drive); break;
        default: output = softClip(output, drive); break;
    }
    
    // Apply bit crusher
    if (bitCrush < 16.0f)
    {
        output = appliBitCrusher(output, bitCrush);
    }
    
    // Apply saturation
    if (saturate > 0.0f)
    {
        output = applySaturation(output, saturate / 100.0f);
    }
    
    return output;
}

float Distortion::softClip(float input, float drive)
{
    if (drive < 0.01f)
        return input;
    
    float gain = 1.0f + (drive * 10.0f);
    float x = input * gain;
    
    // Soft clipping using tanh
    return std::tanh(x) / std::tanh(gain);
}

float Distortion::hardClip(float input, float drive)
{
    if (drive < 0.01f)
        return input;
    
    float gain = 1.0f + (drive * 15.0f);
    float x = input * gain;
    
    // Hard clipping
    float threshold = 1.0f / gain;
    return juce::jlimit(-threshold, threshold, x) * gain;
}

float Distortion::fuzz(float input, float drive)
{
    if (drive < 0.01f)
        return input;
    
    float gain = 1.0f + (drive * 20.0f);
    float x = input * gain;
    
    // Fuzz distortion (asymmetric clipping with fold-back)
    if (x > 1.0f)
        x = 1.0f - (x - 1.0f);
    else if (x < -0.7f)
        x = -0.7f + (x + 0.7f) * 0.5f;
    
    return x / gain;
}

float Distortion::decimator(float input, float drive)
{
    if (drive < 0.01f)
        return input;
    
    // Sample rate reduction effect
    static float lastSample = 0.0f;
    static int sampleCounter = 0;
    
    int decimationFactor = 1 + static_cast<int>(drive * 32.0f);
    
    if (sampleCounter % decimationFactor == 0)
    {
        lastSample = input;
    }
    
    sampleCounter++;
    
    return lastSample;
}

float Distortion::appliBitCrusher(float input, float bits)
{
    if (bits >= 16.0f)
        return input;
    
    // Quantize to fewer bits
    float levels = std::pow(2.0f, bits);
    float quantized = std::floor(input * levels + 0.5f) / levels;
    
    return quantized;
}

float Distortion::applySaturation(float input, float amount)
{
    if (amount < 0.01f)
        return input;
    
    // Tape-style saturation
    float drive = 1.0f + (amount * 3.0f);
    float x = input * drive;
    
    // Soft saturation curve
    float saturated;
    if (std::abs(x) < 0.5f)
        saturated = x;
    else
        saturated = (std::abs(x) - 0.25f) / (1.0f + std::pow(std::abs(x) - 0.25f, 2.0f));
    
    return saturated * (x < 0.0f ? -1.0f : 1.0f) / drive;
}