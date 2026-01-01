#pragma once

#include <JuceHeader.h>

struct KickParams
{
    float bodyLevel = 1.0f;
    float clickLevel = 0.5f;
    float pitchStartHz = 200.0f;
    float pitchEndHz = 50.0f;
    float pitchTauMs = 20.0f;
    float attackMs = 0.0f;
    float t12Ms = 5.0f;
    float t24Ms = 20.0f;
    float tailMsToMinus60Db = 70.0f;
    float clickDecayMs = 3.0f;
    float clickHPFHz = 2000.0f;
    int bodyOscType = 0; // 0=sine, 1=triangle
    float keyTracking = 0.0f;
    bool retriggerMode = false;
    int distortionType = 0; // 0=tanh,1=hard,2=asym
    float drive = 0.5f;
    float asymmetry = 0.0f;
    float outputGainDb = 0.0f;
    float outputHPFHz = 20.0f;
    bool limiterEnabled = true;
    float velocitySensitivity = 1.0f;

    juce::String toJson() const;
    static KickParams fromJson(const juce::var& obj);
};

KickParams loadKickParamsJson(const juce::File& file);
void saveKickParamsJson(const juce::File& file, const KickParams& params);
