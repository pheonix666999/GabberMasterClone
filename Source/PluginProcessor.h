#pragma once

#include <JuceHeader.h>
#include "SampleManager.h"
#include "DSP/Distortion.h"
#include "DSP/Filter.h"
#include "DSP/Reverb.h"
#include "DSP/EQ.h"
#include "DSP/CurveEQ.h"
#include "DSP/LayerProcessor.h"

//==============================================================================
class GabbermasterAudioProcessor : public juce::AudioProcessor,
                                   public juce::ValueTree::Listener
{
public:
    //==============================================================================
    GabbermasterAudioProcessor();
    ~GabbermasterAudioProcessor() override;

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
    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    SampleManager& getSampleManager() { return sampleManager; }

    // Load custom sample
    void loadCustomSample(const juce::File& file);

    // For UI access
    float getEnvelopeValue() const { return currentEnvelopeValue.load(); }
    int getCurrentFilterType() const { return currentFilterType.load(); }
    bool isSlowMode() const { return slowMode.load(); }
    bool isEffectsBypassed() const { return effectsBypassed.load(); }

    void setFilterType(int type) { currentFilterType.store(type); }
    void setSlowMode(bool slow) { slowMode.store(slow); }
    void setEffectsBypassed(bool bypassed) { effectsBypassed.store(bypassed); }

    // EQ access for UI drawing
    ParametricEQ& getEQ() { return parametricEQ; }
    const ParametricEQ& getEQ() const { return parametricEQ; }
    CurveEQ& getCurveEQ() { return curveEQ; }
    const CurveEQ& getCurveEQ() const { return curveEQ; }

    // Layer levels for UI
    float getSubLevel() const { return subLevel.load(); }
    float getBodyLevel() const { return bodyLevel.load(); }
    float getClickLevel() const { return clickLevel.load(); }

private:
    //==============================================================================
    // Audio Processing State Tree
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Sample playback
    SampleManager sampleManager;
    juce::AudioFormatManager formatManager;

    // DSP Effects
    Distortion distortion;
    Filter filter;
    Reverb reverb;
    ParametricEQ parametricEQ; // Keep for backward compatibility
    CurveEQ curveEQ; // New curve-based EQ
    LayerProcessor layerProcessor;

    // ADSR Envelope
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    // Voice management
    struct Voice
    {
        bool isActive = false;
        int noteNumber = -1;
        float velocity = 0.0f;
        int samplePosition = 0;
        float pitchRatio = 1.0f;
        float pitchEnvValue = 0.0f;
        juce::ADSR envelope;
        juce::ADSR pitchEnvelope;
        
        // Oscillator state for algorithmic generation
        float phase = 0.0f;
        float phase2 = 0.0f; // For detuned sub-oscillators
        float phase3 = 0.0f;
        float noiseState = 0.0f;
        float startPitch = 0.0f;
        float endPitch = 0.0f;
        int mode = 0;
        float harmonicMix = 0.0f;

        void reset()
        {
            isActive = false;
            noteNumber = -1;
            velocity = 0.0f;
            samplePosition = 0;
            pitchRatio = 1.0f;
            pitchEnvValue = 0.0f;
            envelope.reset();
            pitchEnvelope.reset();
            phase = 0.0f;
            phase2 = 0.0f;
            phase3 = 0.0f;
            noiseState = 0.0f;
            startPitch = 0.0f;
            endPitch = 0.0f;
            mode = 0;
            harmonicMix = 0.0f;
        }
    };

    static constexpr int maxVoices = 16;
    std::array<Voice, maxVoices> voices;

    // Processing
    double currentSampleRate = 44100.0;

    // Parameter smoothing
    juce::SmoothedValue<float> driveSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    // State for UI
    std::atomic<float> currentEnvelopeValue{0.0f};
    std::atomic<int> currentFilterType{1}; // 0=HP, 1=LP, 2=BP
    std::atomic<bool> slowMode{false};
    std::atomic<bool> effectsBypassed{false};
    std::atomic<int> distortionPosition{1}; // 0=Pre, 1=Post, 2=Both

    // Layer level metering (for UI visualization)
    std::atomic<float> subLevel{0.0f};
    std::atomic<float> bodyLevel{0.0f};
    std::atomic<float> clickLevel{0.0f};

    // Helper methods
    void handleMidiEvent(const juce::MidiMessage& message);
    void renderVoices(juce::AudioBuffer<float>& buffer, int numSamples);
    Voice* findFreeVoice();
    Voice* findVoiceForNote(int noteNumber);
    
    // Mode-based waveform generation
    float generateWaveformSample(Voice& voice, float time, float currentPitch, int mode);
    void getModeParameters(int mode, float& startPitch, float& endPitch, float& detune1, float& detune2, float& harmonicMix);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GabbermasterAudioProcessor)
};
