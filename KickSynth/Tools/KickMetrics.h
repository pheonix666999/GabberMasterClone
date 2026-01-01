#pragma once

#include <JuceHeader.h>
#include <vector>

struct KickMetrics
{
    double peak_dbfs = -120.0;
    double true_peak_dbfs = -120.0;
    double rms_0_100ms_dbfs = -120.0;
    double crest_db = 0.0;
    double attack_ms = 0.0;
    double t12_ms = 0.0;
    double t24_ms = 0.0;
    double tail_ms_to_minus60db = 0.0;
    double pitch_start_hz = 0.0;
    double pitch_end_hz = 0.0;
    double pitch_tau_ms = 0.0;
    double sub_20_60_over_body_60_200 = 0.0;
    double click_2k_10k_over_body_60_200 = 0.0;

    juce::String toJson() const;
    static KickMetrics fromJson(const juce::var& obj);
};

struct KickAnalyzeResult
{
    KickMetrics metrics;
    int onset_sample = 0;
    double trim_ms = 0.0;
    double sample_rate = 0.0;
    int trimmed_samples = 0;
};

class KickAnalyzer
{
public:
    static KickAnalyzeResult analyzeFile(const juce::File& file);
    static KickAnalyzeResult analyzeBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    static juce::AudioBuffer<float> toMono(const juce::AudioBuffer<float>& buffer);
    static int findOnsetSample(const float* samples, int numSamples, double sampleRate);
    static double computeTruePeak(const float* samples, int numSamples);
    static double computeRmsDb(const float* samples, int numSamples);
    static std::vector<double> buildPeakEnvelope(const float* samples, int numSamples);
    static std::vector<float> bandpass(const float* samples, int numSamples, double sampleRate,
                                       double lowHz, double highHz);
    static double estimatePitchYin(const float* samples, int numSamples, double sampleRate,
                                   double minHz, double maxHz);
    static double estimatePitchTau(const std::vector<double>& timesMs,
                                   const std::vector<double>& pitchesHz,
                                   double startHz, double endHz);
    static double bandRms(const float* samples, int numSamples, double sampleRate,
                          double lowHz, double highHz);
    static double median(std::vector<double>& values);
};

struct KickMetricScore
{
    double score = 0.0;
};

KickMetricScore computeScore(const KickMetrics& target, const KickMetrics& actual);
