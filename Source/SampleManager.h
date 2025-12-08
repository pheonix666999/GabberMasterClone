#pragma once

#include <JuceHeader.h>

class SampleManager
{
public:
    SampleManager();
    ~SampleManager();
    
    void loadDefaultSamples();
    void loadCustomSample(const juce::File& file, juce::AudioFormatManager& formatManager);
    juce::AudioBuffer<float>* getSample(int index);
    
    int getNumSamples() const { return static_cast<int>(samples.size()); }
    
private:
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> samples;
    std::unique_ptr<juce::AudioBuffer<float>> customSample;
    
    void createSyntheticKick(juce::AudioBuffer<float>& buffer, double sampleRate,
                            float frequency, float decay, float distortion);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleManager)
};