#pragma once

#include <JuceHeader.h>

/**
 * KickVoice - Synthesizes a kick drum with measurable parameters
 * 
 * Architecture:
 * - BODY oscillator (sine/triangle) with pitch envelope
 * - CLICK layer (noise burst with 2k-10k emphasis)
 * - AMP envelope (attack + exponential decay)
 * - Distortion applied externally
 */
class KickVoice
{
public:
    KickVoice();
    ~KickVoice() = default;

    // Voice management
    void noteOn(int noteNumber, float velocity, double sampleRate);
    void noteOff();
    bool isActive() const { return active; }
    
    // Parameter setters (all values normalized 0-1 except where noted)
    void setBodyLevel(float level) { bodyLevel = level; }
    void setClickLevel(float level) { clickLevel = level; }
    
    // Pitch envelope parameters (Hz)
    void setPitchStartHz(float hz) { pitchStartHz = hz; }
    void setPitchEndHz(float hz) { pitchEndHz = hz; }
    void setPitchTauMs(float tauMs) { pitchTauMs = tauMs; }
    
    // Amplitude envelope parameters (ms)
    void setAttackMs(float attackMs) { attackMsValue = attackMs; }
    void setT12Ms(float t12Ms) { t12MsValue = t12Ms; }
    void setT24Ms(float t24Ms) { t24MsValue = t24Ms; }
    void setTailMsToMinus60Db(float tailMs) { tailMsToMinus60Db = tailMs; }
    
    // Click layer parameters
    void setClickDecayMs(float decayMs) { clickDecayMs = decayMs; }
    void setClickHPFHz(float hpfHz) { clickHPFHz = hpfHz; }
    
    // Body oscillator type
    void setBodyOscillatorType(int type) { bodyOscType = type; } // 0=sine, 1=triangle
    
    // Key tracking
    void setKeyTracking(float semitones) { keyTrackingSemitones = semitones; }
    
    // Retrigger mode
    void setRetriggerMode(bool enabled) { retriggerMode = enabled; }
    
    // Render one sample
    float renderSample();
    
    // Get current pitch for analysis
    float getCurrentPitchHz() const { return currentPitchHz; }
    
    // Get envelope value for visualization
    float getAmpEnvelopeValue() const { return ampEnvValue; }
    float getPitchEnvelopeValue() const { return pitchEnvValue; }

private:
    // State
    bool active = false;
    double sampleRate = 44100.0;
    double timeSinceNoteOn = 0.0;
    int noteNumber = 60; // C4
    
    // Body oscillator
    float bodyPhase = 0.0f;
    int bodyOscType = 0; // 0=sine, 1=triangle
    
    // Click layer
    float clickPhase = 0.0f;
    float clickNoiseState = 0.0f;
    juce::dsp::IIR::Filter<float> clickHPF;
    juce::dsp::IIR::Coefficients<float>::Ptr clickHPFCoeffs;
    
    // Pitch envelope (exponential sweep)
    float pitchStartHz = 200.0f;
    float pitchEndHz = 50.0f;
    float pitchTauMs = 20.0f;
    float currentPitchHz = 200.0f;
    float pitchEnvValue = 1.0f;
    
    // Amplitude envelope (attack + exponential decay)
    float attackMsValue = 0.0f;
    float t12MsValue = 5.0f;  // Time to -12dB
    float t24MsValue = 20.0f; // Time to -24dB
    float tailMsToMinus60Db = 70.0f;
    float ampEnvValue = 0.0f;
    
    // Click layer
    float clickDecayMs = 3.0f;
    float clickHPFHz = 2000.0f;
    
    // Levels
    float bodyLevel = 1.0f;
    float clickLevel = 0.5f;
    
    // Key tracking
    float keyTrackingSemitones = 0.0f;
    
    // Retrigger mode
    bool retriggerMode = false;
    
    // Helper functions
    float generateBodySample();
    float generateClickSample();
    void updatePitchEnvelope();
    void updateAmpEnvelope();
    float exponentialDecay(float timeMs, float tauMs);
    float computeDecayTauFromT12(float t12Ms);
    float computeDecayTauFromT24(float t24Ms);
    float computeDecayTauFromTail(float tailMs);
};

