#pragma once

#include "KickMetrics.h"
#include <map>

/**
 * MatchHelper - Provides parameter mapping and scoring for matching target metrics
 */
class MatchHelper
{
public:
    // Parameter suggestions based on target metrics
    struct ParameterSuggestions
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
        float drive = 0.5f;
        float asymmetry = 0.0f;
        float outputGain = 0.0f;
    };
    
    // Generate parameter suggestions from target metrics
    static ParameterSuggestions suggestParameters(const KickMetrics& target);
    
    // Compute score for given parameters (requires rendering kick and analyzing)
    // This would be called iteratively by the user
    static float computeParameterScore(const ParameterSuggestions& params, const KickMetrics& target);
    
private:
    // Mapping rules from metrics to parameters
    static float mapPitchStartHz(float targetPitchStart);
    static float mapPitchEndHz(float targetPitchEnd);
    static float mapPitchTauMs(float targetPitchTau);
    static float mapAttackMs(float targetAttack);
    static float mapT12Ms(float targetT12);
    static float mapT24Ms(float targetT24);
    static float mapTailMs(float targetTail);
    static float mapClickLevel(float targetClickRatio);
    static float mapBodyLevel(float targetSubRatio);
};



