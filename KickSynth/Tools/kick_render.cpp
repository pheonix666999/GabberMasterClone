#include "KickRenderEngine.h"
#include <iostream>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc < 3)
    {
        std::cout << "Usage: kick_render <params.json> <output.wav> [--sr 48000] [--length-ms 500] [--velocity 1.0]\n";
        return 1;
    }

    juce::File paramsFile(argv[1]);
    juce::File outFile(argv[2]);

    if (!paramsFile.existsAsFile())
    {
        std::cout << "Error: params file not found: " << paramsFile.getFullPathName() << "\n";
        return 1;
    }

    double sampleRate = 48000.0;
    double lengthMs = 500.0;
    float velocity = 1.0f;

    for (int i = 3; i < argc; ++i)
    {
        juce::String arg = argv[i];
        if (arg == "--sr" && i + 1 < argc)
            sampleRate = juce::String(argv[++i]).getDoubleValue();
        else if (arg == "--length-ms" && i + 1 < argc)
            lengthMs = juce::String(argv[++i]).getDoubleValue();
        else if (arg == "--velocity" && i + 1 < argc)
            velocity = juce::String(argv[++i]).getFloatValue();
    }

    auto params = loadKickParamsJson(paramsFile);

    int numSamples = (int)std::round(sampleRate * (lengthMs / 1000.0));
    numSamples = std::max(1, numSamples);

    juce::AudioBuffer<float> buffer(1, numSamples);

    KickRenderEngine engine;
    engine.prepare(sampleRate);
    engine.render(params, velocity, buffer);

    juce::WavAudioFormat wav;
    if (outFile.existsAsFile())
        outFile.deleteFile();
    outFile.getParentDirectory().createDirectory();
    std::unique_ptr<juce::FileOutputStream> outStream(outFile.createOutputStream());
    if (!outStream)
    {
        std::cout << "Error: cannot open output file: " << outFile.getFullPathName() << "\n";
        return 1;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(outStream.get(), sampleRate, 1, 24, {}, 0));
    if (!writer)
    {
        std::cout << "Error: failed to create WAV writer\n";
        return 1;
    }

    outStream.release();
    writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());

    std::cout << "Wrote: " << outFile.getFullPathName() << std::endl;
    return 0;
}
