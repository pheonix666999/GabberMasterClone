#pragma once

#include "JuceHeader.h"
#include "DSP/KickVoice.h"
#include "DSP/KickDistortion.h"
#include "DSP/KickLimiter.h"
#include <atomic>

//==============================================================================
class KickSynthAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    KickSynthAudioProcessor();
    ~KickSynthAudioProcessor() override;
    
    void triggerGoHit(float velocity = 1.0f);

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Get voice for visualization
    KickVoice* getVoice() { return voices[0].get(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Voice management (monophonic for kick)
    static constexpr int maxVoices = 1;
    std::array<std::unique_ptr<KickVoice>, maxVoices> voices;
    
    // DSP modules
    KickDistortion distortion;
    KickLimiter limiter;
    
    // Sample rate
    double currentSampleRate = 44100.0;
    
    // Parameter smoothing
    juce::SmoothedValue<float> bodyLevelSmoothed;
    juce::SmoothedValue<float> clickLevelSmoothed;
    juce::SmoothedValue<float> pitchStartSmoothed;
    juce::SmoothedValue<float> pitchEndSmoothed;
    juce::SmoothedValue<float> pitchTauSmoothed;
    juce::SmoothedValue<float> attackMsSmoothed;
    juce::SmoothedValue<float> t12MsSmoothed;
    juce::SmoothedValue<float> t24MsSmoothed;
    juce::SmoothedValue<float> tailMsSmoothed;
    juce::SmoothedValue<float> clickDecayMsSmoothed;
    juce::SmoothedValue<float> clickHPFHzSmoothed;
    juce::SmoothedValue<float> driveSmoothed;
    juce::SmoothedValue<float> asymmetrySmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> outputHPFHzSmoothed;
    juce::SmoothedValue<float> velocitySensitivitySmoothed;
    
    juce::dsp::IIR::Filter<float> outputHPF;
    juce::dsp::IIR::Coefficients<float>::Ptr outputHPFCoeffs;
    
    void renderVoices(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void updateVoiceParameters();
    
    std::atomic<bool> goRequest{false};
    std::atomic<float> goVelocity{1.0f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickSynthAudioProcessor)
};

