#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
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
        spec.numChannels = 1;  // Each filter handles one channel

        for (int i = 0; i < numBands; ++i)
        {
            bandsL[i].prepare(spec);
            bandsL[i].reset();
            bandsR[i].prepare(spec);
            bandsR[i].reset();
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
                left = bandsL[i].processSample(left);
                right = bandsR[i].processSample(right);
            }
        }
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 1)
            return;

        auto* leftChannel = buffer.getWritePointer(0);
        auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float left = leftChannel[sample];
            float right = rightChannel ? rightChannel[sample] : left;

            processStereo(left, right);

            leftChannel[sample] = left;
            if (rightChannel)
                rightChannel[sample] = right;
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

        *bandsL[bandIndex].coefficients = *coefficients;
        *bandsR[bandIndex].coefficients = *coefficients;
    }

    double currentSampleRate = 44100.0;

    // Separate filters for left and right channels
    std::array<juce::dsp::IIR::Filter<float>, numBands> bandsL;
    std::array<juce::dsp::IIR::Filter<float>, numBands> bandsR;

    std::array<float, numBands> frequencies = {80.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f};
    std::array<float, numBands> gains = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, numBands> qValues = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};
