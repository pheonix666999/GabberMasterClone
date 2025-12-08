// ============================================================================
// DSP/Reverb.cpp
// ============================================================================
#include "Reverb.h"

Reverb::Reverb()
{
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.33f;
    reverbParams.dryLevel = 0.4f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
}

void Reverb::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juceReverb.setSampleRate(sampleRate);
    reset();
}

void Reverb::reset()
{
    juceReverb.reset();
}

void Reverb::setParameters(float roomSize, float width, float damping)
{
    // Only update if parameters changed (optimization)
    if (std::abs(roomSize - lastRoomSize) > 0.001f ||
        std::abs(width - lastWidth) > 0.001f ||
        std::abs(damping - lastDamping) > 0.001f)
    {
        reverbParams.roomSize = roomSize;
        reverbParams.width = width;
        reverbParams.damping = damping;

        // Adjust wet/dry based on room size
        reverbParams.wetLevel = 0.2f + (roomSize * 0.4f);
        reverbParams.dryLevel = 1.0f - (roomSize * 0.3f);

        juceReverb.setParameters(reverbParams);

        lastRoomSize = roomSize;
        lastWidth = width;
        lastDamping = damping;
    }
}

void Reverb::processStereo(float& leftSample, float& rightSample)
{
    if (bypassed)
        return;

    juceReverb.processStereo(&leftSample, &rightSample, 1);
}
