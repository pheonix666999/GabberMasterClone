#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

// Curve-based, time-dependent EQ system for Gabbermaster
class CurveEQ
{
public:
    enum class CurveTemplate
    {
        PunchBoost,    // +6dB 200-500Hz parabolic
        ClickSharp,    // Shelf +12dB >5kHz with resonance
        BodyCut,       // Low-shelf -3dB <100Hz
        GabberScoop,   // Mid-dip 1-3kHz
        Flat           // No curve
    };

    CurveEQ()
    {
        // Initialize curve points (frequency, gain in dB)
        curvePoints = {
            {20.0f, 0.0f},   // 20 Hz
            {50.0f, 0.0f},   // 50 Hz
            {100.0f, 0.0f},  // 100 Hz
            {200.0f, 0.0f},  // 200 Hz
            {500.0f, 0.0f},  // 500 Hz
            {1000.0f, 0.0f}, // 1 kHz
            {2000.0f, 0.0f}, // 2 kHz
            {5000.0f, 0.0f}, // 5 kHz
            {10000.0f, 0.0f}, // 10 kHz
            {20000.0f, 0.0f}  // 20 kHz
        };
        
        // Initialize time-dependent stages
        for (int i = 0; i < 4; ++i)
        {
            stageCurves[i] = curvePoints;
        }
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        
        // Prepare envelope follower for time-dependent shaping
        float attackTime = 0.001f; // 1ms
        float releaseTime = 0.1f;  // 100ms
        attackCoeff = std::exp(-1.0f / (attackTime * static_cast<float>(sampleRate)));
        releaseCoeff = std::exp(-1.0f / (releaseTime * static_cast<float>(sampleRate)));
        
        // Prepare filters for each curve point (for smooth interpolation)
        updateFilters();
    }

    void setCurveTemplate(CurveTemplate templateType)
    {
        currentTemplate = templateType;
        applyTemplate(templateType);
        updateFilters();
    }

    void setCurvePoint(int index, float frequency, float gainDb)
    {
        if (index >= 0 && index < curvePoints.size())
        {
            curvePoints[index].first = frequency;
            curvePoints[index].second = gainDb;
            updateFilters();
        }
    }

    void setTimeDependency(float amount)
    {
        timeDependency = juce::jlimit(0.0f, 1.0f, amount);
    }

    void setResonance(float resonance)
    {
        globalResonance = juce::jlimit(0.0f, 50.0f, resonance);
        updateFilters();
    }

    void processStereo(float& left, float& right)
    {
        // Update envelope follower
        float inputLevel = std::max(std::abs(left), std::abs(right));
        if (inputLevel > envelopeValue)
            envelopeValue = inputLevel + (envelopeValue - inputLevel) * attackCoeff;
        else
            envelopeValue = inputLevel + (envelopeValue - inputLevel) * releaseCoeff;
        float envValue = envelopeValue;
        
        // Determine current stage based on envelope
        int currentStage = 0;
        if (envValue > 0.9f)
            currentStage = 0; // Attack (0-50ms)
        else if (envValue > 0.5f)
            currentStage = 1; // Decay1 (50-200ms)
        else if (envValue > 0.1f)
            currentStage = 2; // Decay2 (200ms+)
        else
            currentStage = 3; // Release
        
        // Apply frequency response using multi-band filtering
        // Process through filter bank for accurate frequency response
        float leftProcessed = left;
        float rightProcessed = right;
        
        // Apply filters for each curve point
        for (size_t i = 0; i < curvePoints.size() && i < filtersL.size(); ++i)
        {
            if (std::abs(curvePoints[i].second) > 0.1f)
            {
                leftProcessed = filtersL[i].processSample(leftProcessed);
                rightProcessed = filtersR[i].processSample(rightProcessed);
            }
        }
        
        // Apply time-dependent modulation
        if (timeDependency > 0.01f)
        {
            // Blend between static curve and stage-specific curve
            float blend = timeDependency;
            float leftStage = left;
            float rightStage = right;
            
            // Process with stage curve
            for (size_t i = 0; i < stageCurves[currentStage].size() && i < filtersL.size(); ++i)
            {
                if (std::abs(stageCurves[currentStage][i].second) > 0.1f)
                {
                    // Temporarily update filters for stage curve
                    float freq = stageCurves[currentStage][i].first;
                    float gain = stageCurves[currentStage][i].second;
                    float q = 0.707f + (globalResonance / 100.0f) * 5.0f;
                    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                        currentSampleRate, freq, q, juce::Decibels::decibelsToGain(gain));
                    filtersL[i].coefficients = coeffs;
                    filtersR[i].coefficients = coeffs;
                    leftStage = filtersL[i].processSample(leftStage);
                    rightStage = filtersR[i].processSample(rightStage);
                }
            }
            
            leftProcessed = leftProcessed * (1.0f - blend) + leftStage * blend;
            rightProcessed = rightProcessed * (1.0f - blend) + rightStage * blend;
            
            // Note: Filters are restored on next prepare() call
        }
        
        left = leftProcessed;
        right = rightProcessed;
    }

    float getMagnitudeForFrequency(float frequency) const
    {
        return getGainAtFrequency(frequency, curvePoints);
    }

    // Get curve point for UI
    std::pair<float, float> getCurvePoint(int index) const
    {
        if (index >= 0 && index < curvePoints.size())
            return curvePoints[index];
        return {0.0f, 0.0f};
    }

private:
    using CurvePoint = std::pair<float, float>; // frequency, gainDb
    std::vector<CurvePoint> curvePoints;
    std::array<std::vector<CurvePoint>, 4> stageCurves; // Attack, Decay1, Decay2, Release
    
    double currentSampleRate = 44100.0;
    CurveTemplate currentTemplate = CurveTemplate::Flat;
    float timeDependency = 0.0f;
    float globalResonance = 0.0f;
    
    // Simple envelope follower
    float envelopeValue = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    
    // Filters for smooth curve application
    std::array<juce::dsp::IIR::Filter<float>, 10> filtersL;
    std::array<juce::dsp::IIR::Filter<float>, 10> filtersR;

    void applyTemplate(CurveTemplate templateType)
    {
        switch (templateType)
        {
            case CurveTemplate::PunchBoost:
                // +12dB 200-500Hz parabolic for strong punch
                setCurvePoint(3, 200.0f, 3.0f);   // 200 Hz
                setCurvePoint(4, 350.0f, 12.0f); // 350 Hz peak - STRONG
                setCurvePoint(5, 500.0f, 3.0f);  // 500 Hz
                // Cut mud below
                setCurvePoint(2, 100.0f, -3.0f); // 100 Hz
                break;
                
            case CurveTemplate::ClickSharp:
                // Shelf +18dB >5kHz with resonance for sharp click
                setCurvePoint(6, 2000.0f, 3.0f);   // 2 kHz
                setCurvePoint(7, 5000.0f, 12.0f);  // 5 kHz
                setCurvePoint(8, 10000.0f, 18.0f); // 10 kHz - VERY STRONG
                setCurvePoint(9, 20000.0f, 18.0f); // 20 kHz
                break;
                
            case CurveTemplate::BodyCut:
                // Low-shelf -3dB <100Hz
                setCurvePoint(0, 20.0f, -3.0f);   // 20 Hz
                setCurvePoint(1, 50.0f, -3.0f);  // 50 Hz
                setCurvePoint(2, 100.0f, 0.0f);  // 100 Hz
                break;
                
            case CurveTemplate::GabberScoop:
                // Mid-dip 1-3kHz
                setCurvePoint(5, 1000.0f, 0.0f);  // 1 kHz
                setCurvePoint(6, 2000.0f, -6.0f); // 2 kHz dip
                setCurvePoint(7, 3000.0f, 0.0f);  // 3 kHz
                break;
                
            case CurveTemplate::Flat:
            default:
                // Reset all to 0
                for (auto& point : curvePoints)
                    point.second = 0.0f;
                break;
        }
        
        // Copy to all stages
        for (int i = 0; i < 4; ++i)
            stageCurves[i] = curvePoints;
    }


    float getGainAtFrequency(float frequency, const std::vector<CurvePoint>& curve) const
    {
        if (curve.empty())
            return 0.0f;
        
        // Find surrounding points
        int lowerIdx = 0;
        int upperIdx = static_cast<int>(curve.size() - 1);
        
        for (size_t i = 0; i < curve.size() - 1; ++i)
        {
            if (frequency >= curve[i].first && frequency <= curve[i + 1].first)
            {
                lowerIdx = static_cast<int>(i);
                upperIdx = static_cast<int>(i + 1);
                break;
            }
        }
        
        // Linear interpolation
        if (lowerIdx == upperIdx)
            return curve[lowerIdx].second;
        
        float freq1 = curve[lowerIdx].first;
        float freq2 = curve[upperIdx].first;
        float gain1 = curve[lowerIdx].second;
        float gain2 = curve[upperIdx].second;
        
        float t = (frequency - freq1) / (freq2 - freq1);
        return gain1 + (gain2 - gain1) * t;
    }

    void updateFilters()
    {
        // Update filter coefficients for smooth curve application
        // This is a simplified implementation - in production, use a proper filter bank
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        
        for (size_t i = 0; i < curvePoints.size() && i < filtersL.size(); ++i)
        {
            float freq = curvePoints[i].first;
            float gain = curvePoints[i].second;
            float q = 0.707f + (globalResonance / 100.0f) * 5.0f;
            
            if (std::abs(gain) > 0.1f)
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                    currentSampleRate, freq, q, juce::Decibels::decibelsToGain(gain));
                filtersL[i].coefficients = coeffs;
                filtersR[i].coefficients = coeffs;
                filtersL[i].prepare(spec);
                filtersR[i].prepare(spec);
            }
        }
    }
};

