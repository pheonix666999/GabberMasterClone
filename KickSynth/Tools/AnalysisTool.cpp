#include "../Source/JuceHeader.h"
#include "KickMetrics.h"
#include "MatchHelper.h"
#include <iostream>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;
    
    if (argc < 2)
    {
        std::cout << "Usage: KickAnalysisTool <wav_file> [target_metrics_json]" << std::endl;
        std::cout << "  If target_metrics_json is provided, computes diff and score" << std::endl;
        return 1;
    }
    
    juce::String inputFile = juce::String(argv[1]);
    juce::File file(inputFile);
    
    if (!file.existsAsFile())
    {
        std::cout << "Error: File not found: " << inputFile << std::endl;
        return 1;
    }
    
    // Load audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader)
    {
        std::cout << "Error: Could not read audio file" << std::endl;
        return 1;
    }
    
    // Read audio data
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    
    // Compute metrics
    KickMetrics metrics = KickMetrics::computeFromAudio(buffer, reader->sampleRate);
    
    // Print metrics as JSON
    std::cout << "Metrics for: " << inputFile << std::endl;
    std::cout << metrics.toJson() << std::endl;
    
    // If target metrics provided, compute diff and score
    if (argc >= 3)
    {
        juce::String targetFile = juce::String(argv[2]);
        juce::File targetMetricsFile(targetFile);
        
        if (targetMetricsFile.existsAsFile())
        {
            // Load target metrics from JSON file
            juce::String jsonText = targetMetricsFile.loadFileAsString();
            // Parse JSON (simplified - in production use proper JSON parser)
            // For now, assume it's a simple format or use the provided target values directly
            
            // Create target metrics from provided values (hardcoded for now)
            KickMetrics target;
            target.peak_dbfs = 2.0415353626f;
            target.rms_0_100ms_dbfs = -0.8082205101f;
            target.crest_db = 2.8497558727f;
            target.attack_ms = 891.4166666667f;
            target.t12_ms = 4.8125f;
            target.t24_ms = 16.8958333333f;
            target.tail_ms_to_minus60db = 70.0f;
            target.pitch_start_hz = 23.4375f;
            target.pitch_end_hz = 23.4375f;
            target.pitch_tau_ms = 20.5776819191f;
            target.sub_20_60_over_body_60_200 = 106.7212189154f;
            target.click_2k_10k_over_body_60_200 = 25.7267841740f;
            
            // Compute diff
            KickMetrics diff = KickMetrics::computeDiff(target, metrics);
            
            // Compute score
            float score = KickMetrics::computeScore(target, metrics);
            
            std::cout << "\n--- Comparison with Target ---\n" << std::endl;
            std::cout << "Difference:\n" << diff.toJson() << std::endl;
            std::cout << "\nWeighted Error Score: " << score << " (lower is better)" << std::endl;
            
            // Suggest parameters
            MatchHelper::ParameterSuggestions suggestions = MatchHelper::suggestParameters(target);
            std::cout << "\n--- Suggested Parameters ---\n" << std::endl;
            std::cout << "bodyLevel: " << suggestions.bodyLevel << std::endl;
            std::cout << "clickLevel: " << suggestions.clickLevel << std::endl;
            std::cout << "pitchStartHz: " << suggestions.pitchStartHz << std::endl;
            std::cout << "pitchEndHz: " << suggestions.pitchEndHz << std::endl;
            std::cout << "pitchTauMs: " << suggestions.pitchTauMs << std::endl;
            std::cout << "attackMs: " << suggestions.attackMs << std::endl;
            std::cout << "t12Ms: " << suggestions.t12Ms << std::endl;
            std::cout << "t24Ms: " << suggestions.t24Ms << std::endl;
            std::cout << "tailMsToMinus60Db: " << suggestions.tailMsToMinus60Db << std::endl;
            std::cout << "clickDecayMs: " << suggestions.clickDecayMs << std::endl;
            std::cout << "clickHPFHz: " << suggestions.clickHPFHz << std::endl;
            std::cout << "bodyOscType: " << (suggestions.bodyOscType == 0 ? "Sine" : "Triangle") << std::endl;
            std::cout << "drive: " << suggestions.drive << std::endl;
            std::cout << "asymmetry: " << suggestions.asymmetry << std::endl;
            std::cout << "outputGain: " << suggestions.outputGain << std::endl;
        }
    }
    
    return 0;
}

