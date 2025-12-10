#pragma once

#include <JuceHeader.h>
#include <array>

class ParametricEQ
{
public:
    static constexpr int numBands = 5;

    ParametricEQ() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = 2;

        for (auto& band : bands)
        {
            band.prepare(spec);
            band.reset();
        }
    }

    void setBandParameters(int bandIndex, float frequency, float gainDb, float q)
    {
        if (bandIndex < 0 || bandIndex >= numBands)
            return;

        frequencies[bandIndex] = frequency;
        gains[bandIndex] = gainDb;
        qValues[bandIndex] = q;

        updateBand(bandIndex);
    }

    void setFrequency(int bandIndex, float frequency)
    {
        if (bandIndex >= 0 && bandIndex < numBands)
        {
            frequencies[bandIndex] = frequency;
            updateBand(bandIndex);
        }
    }

    void setGain(int bandIndex, float gainDb)
    {
        if (bandIndex >= 0 && bandIndex < numBands)
        {
            gains[bandIndex] = gainDb;
            updateBand(bandIndex);
        }
    }

    void setQ(int bandIndex, float q)
    {
        if (bandIndex >= 0 && bandIndex < numBands)
        {
            qValues[bandIndex] = q;
            updateBand(bandIndex);
        }
    }

    float getFrequency(int bandIndex) const
    {
        return (bandIndex >= 0 && bandIndex < numBands) ? frequencies[bandIndex] : 1000.0f;
    }

    float getGain(int bandIndex) const
    {
        return (bandIndex >= 0 && bandIndex < numBands) ? gains[bandIndex] : 0.0f;
    }

    float getQ(int bandIndex) const
    {
        return (bandIndex >= 0 && bandIndex < numBands) ? qValues[bandIndex] : 1.0f;
    }

    void processStereo(float& left, float& right)
    {
        for (int i = 0; i < numBands; ++i)
        {
            if (std::abs(gains[i]) > 0.1f) // Only process if gain is significant
            {
                // Process left channel
                left = bands[i].processSample(0, left);
                // Process right channel
                right = bands[i].processSample(1, right);
            }
        }
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block(buffer);

        for (int i = 0; i < numBands; ++i)
        {
            if (std::abs(gains[i]) > 0.1f)
            {
                juce::dsp::ProcessContextReplacing<float> context(block);
                bands[i].process(context);
            }
        }
    }

    // Get magnitude response at a frequency (for drawing the curve)
    float getMagnitudeForFrequency(float frequency) const
    {
        float magnitude = 1.0f;

        for (int i = 0; i < numBands; ++i)
        {
            if (std::abs(gains[i]) > 0.1f)
            {
                // Calculate the parametric EQ magnitude response
                float f0 = frequencies[i];
                float G = std::pow(10.0f, gains[i] / 20.0f);
                float Q = qValues[i];

                float ratio = frequency / f0;
                float ratioSq = ratio * ratio;

                // Simplified magnitude calculation for peak/notch filter
                float numerator = (1.0f + (G - 1.0f) * ratioSq / (Q * Q)) + ratioSq;
                float denominator = (1.0f + ratioSq / (Q * Q)) + ratioSq;

                if (gains[i] > 0)
                    magnitude *= std::sqrt(numerator / denominator);
                else
                    magnitude *= std::sqrt(denominator / numerator);
            }
        }

        return magnitude;
    }

    // Default band frequencies
    static constexpr std::array<float, numBands> defaultFrequencies = {80.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f};

private:
    void updateBand(int bandIndex)
    {
        if (currentSampleRate <= 0)
            return;

        auto coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate,
            frequencies[bandIndex],
            qValues[bandIndex],
            juce::Decibels::decibelsToGain(gains[bandIndex])
        );

        *bands[bandIndex].state = *coefficients;
    }

    double currentSampleRate = 44100.0;

    std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
               juce::dsp::IIR::Coefficients<float>>, numBands> bands;

    std::array<float, numBands> frequencies = {80.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f};
    std::array<float, numBands> gains = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, numBands> qValues = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};
