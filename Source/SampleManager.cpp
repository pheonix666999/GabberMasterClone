// ============================================================================
// SampleManager.cpp
// ============================================================================
#include "SampleManager.h"

SampleManager::SampleManager()
{
}

SampleManager::~SampleManager()
{
}

void SampleManager::loadDefaultSamples()
{
    // Create 48 synthetic hardcore kick samples with varying characteristics
    double sampleRate = 44100.0;
    
    struct KickPreset
    {
        float frequency;
        float decay;
        float distortion;
    };
    
    // Base kick presets with different characteristics
    std::vector<KickPreset> basePresets = {
        // Classic gabber kicks
        {60.0f, 0.3f, 0.8f},   // Deep, punchy
        {70.0f, 0.25f, 0.9f},  // Mid, distorted
        {50.0f, 0.4f, 0.7f},   // Sub, long decay
        {80.0f, 0.2f, 0.95f},  // High, aggressive
        {65.0f, 0.35f, 0.85f}, // Balanced
        
        // Speedcore/Terror kicks
        {90.0f, 0.15f, 1.0f},   // Fast attack, brutal
        {100.0f, 0.12f, 0.98f}, // Ultra fast
        {85.0f, 0.18f, 0.92f},  // Speed gabber
        
        // Hardstyle kicks
        {55.0f, 0.5f, 0.6f},   // Reverse bass
        {60.0f, 0.45f, 0.65f}, // Punchy hardstyle
        {65.0f, 0.4f, 0.7f},   // Classic hardstyle
        
        // Industrial kicks
        {75.0f, 0.22f, 0.88f}, // Metallic
        {70.0f, 0.28f, 0.82f}, // Industrial mid
        {68.0f, 0.3f, 0.75f},  // Grinding
        
        // Experimental
        {95.0f, 0.1f, 0.95f},  // Click attack
        {45.0f, 0.6f, 0.5f},   // Sub bomb
        {110.0f, 0.08f, 1.0f}, // Laser kick
        {58.0f, 0.38f, 0.78f}, // Warm kick
        {72.0f, 0.26f, 0.86f}, // Crunchy
        {63.0f, 0.32f, 0.73f}  // Balanced soft
    };
    
    // Expand to 48 presets by lightly varying the base set
    std::vector<KickPreset> presets;
    presets.reserve(48);
    
    auto addVariant = [&](const KickPreset& src, int variantIdx)
    {
        float freqScale = 1.0f + 0.03f * static_cast<float>((variantIdx % 4) - 1);  // -0.03, 0, 0.03, 0.06
        float decayScale = 1.0f + 0.05f * static_cast<float>((variantIdx % 3) - 1); // -0.05, 0, 0.05
        float distOffset = 0.05f * static_cast<float>((variantIdx % 5) - 2);        // -0.1 .. +0.1
        
        KickPreset p;
        p.frequency = juce::jlimit(35.0f, 140.0f, src.frequency * freqScale);
        p.decay = juce::jlimit(0.08f, 0.8f, src.decay * decayScale);
        p.distortion = juce::jlimit(0.4f, 1.1f, src.distortion + distOffset);
        
        presets.push_back(p);
    };
    
    int variant = 0;
    while (presets.size() < 48)
    {
        const auto& src = basePresets[variant % basePresets.size()];
        addVariant(src, variant);
        variant++;
    }
    
    for (const auto& preset : presets)
    {
        auto sample = std::make_unique<juce::AudioBuffer<float>>(1, (int)(sampleRate * 1.0)); // 1 second
        createSyntheticKick(*sample, sampleRate, preset.frequency, preset.decay, preset.distortion);
        samples.push_back(std::move(sample));
    }
}

void SampleManager::createSyntheticKick(juce::AudioBuffer<float>& buffer, double sampleRate,
                                       float frequency, float decay, float distortion)
{
    auto* data = buffer.getWritePointer(0);
    int numSamples = buffer.getNumSamples();
    
    // Phase and pitch envelope
    float phase = 0.0f;
    float pitchEnvAmount = frequency * 2.5f; // Pitch drop amount
    
    for (int i = 0; i < numSamples; ++i)
    {
        float time = (float)i / (float)sampleRate;
        
        // Amplitude envelope (exponential decay)
        float ampEnv = std::exp(-time / decay);
        
        // Pitch envelope (fast exponential drop)
        float pitchEnv = frequency + pitchEnvAmount * std::exp(-time / 0.03f);
        
        // Generate sine wave with pitch envelope
        float sample = std::sin(phase * juce::MathConstants<float>::twoPi);
        
        // Add harmonics for body
        sample += 0.3f * std::sin(phase * juce::MathConstants<float>::twoPi * 2.0f);
        sample += 0.1f * std::sin(phase * juce::MathConstants<float>::twoPi * 3.0f);
        
        // Apply distortion
        if (distortion > 0.0f)
        {
            float dist = 1.0f + (distortion * 10.0f);
            sample = std::tanh(sample * dist) / std::tanh(dist);
        }
        
        // Apply amplitude envelope
        sample *= ampEnv;
        
        // Add click attack
        if (time < 0.005f)
        {
            float clickEnv = 1.0f - (time / 0.005f);
            sample += clickEnv * 0.3f * (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
        }
        
        // Clip
        sample = juce::jlimit(-1.0f, 1.0f, sample);
        
        data[i] = sample * 0.8f; // Normalize
        
        // Update phase
        phase += pitchEnv / (float)sampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }
}

void SampleManager::loadCustomSample(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader != nullptr)
    {
        customSample = std::make_unique<juce::AudioBuffer<float>>(
            reader->numChannels,
            static_cast<int>(reader->lengthInSamples));
        
        reader->read(customSample.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        
        // Convert to mono if stereo
        if (customSample->getNumChannels() > 1)
        {
            auto monoBuffer = std::make_unique<juce::AudioBuffer<float>>(1, customSample->getNumSamples());
            monoBuffer->clear();
            
            for (int channel = 0; channel < customSample->getNumChannels(); ++channel)
            {
                monoBuffer->addFrom(0, 0, *customSample, channel, 0,
                                   customSample->getNumSamples(), 1.0f / customSample->getNumChannels());
            }
            
            customSample = std::move(monoBuffer);
        }
    }
}

juce::AudioBuffer<float>* SampleManager::getSample(int index)
{
    // Return custom sample if index is -1 or out of range
    if (customSample != nullptr && (index < 0 || index >= samples.size()))
        return customSample.get();
    
    if (index >= 0 && index < samples.size())
        return samples[index].get();
    
    return nullptr;
}
