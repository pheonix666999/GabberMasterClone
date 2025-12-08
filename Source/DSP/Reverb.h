// ============================================================================
// DSP/Reverb.h
// Simple stereo reverb for GabberMaster
// ============================================================================
#pragma once

#include <JuceHeader.h>

class Reverb
{
public:
    Reverb();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setParameters(float roomSize, float width, float damping);
    void processStereo(float& leftSample, float& rightSample);

    bool isBypassed() const { return bypassed; }
    void setBypassed(bool shouldBypass) { bypassed = shouldBypass; }

private:
    juce::Reverb juceReverb;
    juce::Reverb::Parameters reverbParams;

    double currentSampleRate = 44100.0;
    bool bypassed = false;

    float lastRoomSize = -1.0f;
    float lastWidth = -1.0f;
    float lastDamping = -1.0f;
};
