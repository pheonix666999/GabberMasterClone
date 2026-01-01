#include "KickMetrics.h"
#include <iostream>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc < 2)
    {
        std::cout << "Usage: kick_analyze <wav_file> [--out metrics.json] [--verbose]\n";
        return 1;
    }

    juce::String inputPath = argv[1];
    juce::File inputFile(inputPath);
    if (!inputFile.existsAsFile())
    {
        std::cout << "Error: file not found: " << inputPath << "\n";
        return 1;
    }

    bool verbose = false;
    juce::File outFile;
    for (int i = 2; i < argc; ++i)
    {
        juce::String arg = argv[i];
        if (arg == "--verbose")
            verbose = true;
        else if (arg == "--out" && i + 1 < argc)
            outFile = juce::File(argv[++i]);
    }

    auto result = KickAnalyzer::analyzeFile(inputFile);
    auto json = result.metrics.toJson();

    if (verbose)
    {
        std::cerr << "trim_ms: " << result.trim_ms << "\n";
        std::cerr << "trimmed_samples: " << result.trimmed_samples << "\n";
        std::cerr << "sample_rate: " << result.sample_rate << "\n";
    }

    std::cout << json << std::endl;

    if (outFile != juce::File())
        outFile.replaceWithText(json);

    return 0;
}
