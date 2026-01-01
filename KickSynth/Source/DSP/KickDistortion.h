#pragma once

#include "../JuceHeader.h"

/**
 * KickDistortion - Distortion module for kick synthesis
 * 
 * Types:
 * - 0: Tanh soft clip
 * - 1: Hard clip (simple threshold)
 * - 2: Asymmetric soft clip
 */
class KickDistortion
{
public:
    KickDistortion();
    ~KickDistortion() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Process single sample
    float processSample(float input);
    
    // Process buffer
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setDistortionType(int type) { distortionType = type; }
    void setDrive(float driveValue) { drive = driveValue; } // 0-1, maps to 1-10x
    void setAsymmetry(float asymmetryValue) { asymmetry = asymmetryValue; } // 0-1, maps to 0-0.5
    
private:
    int distortionType = 0; // 0=tanh, 1=hard, 2=asymmetric
    float drive = 0.5f; // 0-1
    float asymmetry = 0.0f; // 0-1
    
    float hardClip(float input);
    // Tanh soft clip
    float tanhSoftClip(float input);
    
    // Asymmetric soft clip
    float asymmetricSoftClip(float input);
};

