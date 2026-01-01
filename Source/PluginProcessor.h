#pragma once

#include <JuceHeader.h>

//==============================================================================
class GabbermasterAudioProcessor : public juce::AudioProcessor
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

    // CRITICAL: These MUST return correct values for synth/MIDI
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
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

    // Trigger a kick from GUI (GO button)
    void triggerKick(int noteNumber = 48, float velocity = 1.0f);

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Voice structure for kick synthesis
    struct Voice
    {
        bool isActive = false;
        int noteNumber = -1;
        float velocity = 0.0f;
        float phase = 0.0f;
        float clickPhase = 0.0f;
        float currentFreq = 50.0f;
        float targetFreq = 23.2f;
        float startFreq = 164.0f;

        // Envelope state
        float envLevel = 0.0f;
        float clickEnvLevel = 0.0f;
        int samplesSinceNoteOn = 0;

        // Per-voice timing (calculated at note-on based on MIDI note)
        float attackTau = 0.008f;       // Attack time constant
        float decayTau = 0.200f;        // Decay time constant
        float pitchDecayTau = 0.00676f; // Pitch sweep time constant
        float clickDecayTau = 0.001f;   // Click transient decay

        // Sample-based pitch ratio (for scaling click freq, etc.)
        float pitchRatio = 1.0f;

        // Max duration in samples (to prevent infinite voices)
        int maxDurationSamples = 88200; // 2 seconds at 44.1kHz

        void reset()
        {
            isActive = false;
            noteNumber = -1;
            velocity = 0.0f;
            phase = 0.0f;
            clickPhase = 0.0f;
            currentFreq = 50.0f;
            targetFreq = 23.2f;
            startFreq = 164.0f;
            envLevel = 0.0f;
            clickEnvLevel = 0.0f;
            samplesSinceNoteOn = 0;
            attackTau = 0.008f;
            decayTau = 0.200f;
            pitchDecayTau = 0.00676f;
            clickDecayTau = 0.001f;
            pitchRatio = 1.0f;
            maxDurationSamples = 88200;
        }
    };

    static constexpr int maxVoices = 8;
    std::array<Voice, maxVoices> voices;

    // Processing state
    double currentSampleRate = 44100.0;

    // Filter state (stereo)
    float filterState1L = 0.0f;
    float filterState2L = 0.0f;
    float filterState1R = 0.0f;
    float filterState2R = 0.0f;

    // Post-synth LPF state
    float postLpfStateL = 0.0f;
    float postLpfStateR = 0.0f;

    // Debug timing
    int64_t lastNoMidiLogTime = 0;

    // Reference note for pitch calculations
    static constexpr int referenceNote = 48; // C3

    // Neutral track position
    static constexpr float neutralTrack = 0.0625f;

    // Helper methods
    void startVoice(int noteNumber, float velocity);
    void stopVoice(int noteNumber);
    void renderVoices(juce::AudioBuffer<float>& buffer);
    float processFilter(float input, float cutoffNorm, int filterType, float Q,
                        float envAmount, float envLevel, bool bypass, bool isLeft);
    float processPostLPF(float input, float cutoffHz, bool isLeft);

    // Internal engine constants for each kick mode (NOT parameters)
    void getKickModeParams(int mode, float& startPitchHz, float& endPitchHz,
                           float& pitchDecayMs, float& clickFreq, float& clickAmp);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GabbermasterAudioProcessor)
};
