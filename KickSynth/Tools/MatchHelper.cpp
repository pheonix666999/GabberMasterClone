#include "MatchHelper.h"
#include "../Source/JuceHeader.h"
#include <algorithm>

MatchHelper::ParameterSuggestions MatchHelper::suggestParameters(const KickMetrics& target)
{
    ParameterSuggestions suggestions;
    
    // Direct mappings for timing parameters
    suggestions.pitchStartHz = mapPitchStartHz(target.pitch_start_hz);
    suggestions.pitchEndHz = mapPitchEndHz(target.pitch_end_hz);
    suggestions.pitchTauMs = mapPitchTauMs(target.pitch_tau_ms);
    suggestions.attackMs = mapAttackMs(target.attack_ms);
    suggestions.t12Ms = mapT12Ms(target.t12_ms);
    suggestions.t24Ms = mapT24Ms(target.t24_ms);
    suggestions.tailMsToMinus60Db = mapTailMs(target.tail_ms_to_minus60db);
    
    // Map spectral ratios to levels
    suggestions.clickLevel = mapClickLevel(target.click_2k_10k_over_body_60_200);
    suggestions.bodyLevel = mapBodyLevel(target.sub_20_60_over_body_60_200);
    
    // Set click HPF based on click ratio (higher ratio = higher HPF)
    if (target.click_2k_10k_over_body_60_200 > 20.0f)
        suggestions.clickHPFHz = 5000.0f;
    else if (target.click_2k_10k_over_body_60_200 > 10.0f)
        suggestions.clickHPFHz = 3000.0f;
    else
        suggestions.clickHPFHz = 2000.0f;
    
    // Click decay based on attack time
    suggestions.clickDecayMs = juce::jmax(1.0f, target.attack_ms * 0.1f);
    
    // Body oscillator type based on sub ratio (higher sub = sine, lower = triangle)
    if (target.sub_20_60_over_body_60_200 > 50.0f)
        suggestions.bodyOscType = 0; // Sine
    else
        suggestions.bodyOscType = 1; // Triangle
    
    // Drive based on crest factor
    if (target.crest_db > 10.0f)
        suggestions.drive = 0.7f;
    else if (target.crest_db > 5.0f)
        suggestions.drive = 0.5f;
    else
        suggestions.drive = 0.3f;
    
    // Output gain to match peak
    if (target.peak_dbfs < 0.0f)
        suggestions.outputGain = -target.peak_dbfs;
    else
        suggestions.outputGain = 0.0f;
    
    return suggestions;
}

float MatchHelper::computeParameterScore(const ParameterSuggestions& params, const KickMetrics& target)
{
    // This would require rendering the kick with these parameters and analyzing it
    // For now, return a placeholder score
    // In practice, this would be called by the user after rendering
    return 0.0f;
}

float MatchHelper::mapPitchStartHz(float targetPitchStart)
{
    // Clamp to valid range
    return juce::jlimit(20.0f, 500.0f, targetPitchStart);
}

float MatchHelper::mapPitchEndHz(float targetPitchEnd)
{
    // Clamp to valid range
    return juce::jlimit(20.0f, 200.0f, targetPitchEnd);
}

float MatchHelper::mapPitchTauMs(float targetPitchTau)
{
    // Clamp to valid range
    return juce::jlimit(1.0f, 100.0f, targetPitchTau);
}

float MatchHelper::mapAttackMs(float targetAttack)
{
    // Clamp to valid range
    return juce::jlimit(0.0f, 50.0f, targetAttack);
}

float MatchHelper::mapT12Ms(float targetT12)
{
    // Clamp to valid range
    return juce::jlimit(1.0f, 50.0f, targetT12);
}

float MatchHelper::mapT24Ms(float targetT24)
{
    // Clamp to valid range
    return juce::jlimit(5.0f, 200.0f, targetT24);
}

float MatchHelper::mapTailMs(float targetTail)
{
    // Clamp to valid range
    return juce::jlimit(10.0f, 500.0f, targetTail);
}

float MatchHelper::mapClickLevel(float targetClickRatio)
{
    // Map click ratio (typically 0-50) to level (0-1)
    // Higher ratio = higher level
    float normalized = juce::jlimit(0.0f, 50.0f, targetClickRatio) / 50.0f;
    return normalized;
}

float MatchHelper::mapBodyLevel(float targetSubRatio)
{
    // Map sub ratio (typically 0-200) to body level (0-1)
    // Higher ratio = higher body level
    float normalized = juce::jlimit(0.0f, 200.0f, targetSubRatio) / 200.0f;
    return juce::jmax(0.1f, normalized); // Ensure minimum body level
}

