#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

class LayerProcessor
{
public:
    LayerProcessor() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = 1;  // Each filter handles one channel

        // Sub layer: Low-pass at 200 Hz
        subLowPassL.prepare(spec);
        subLowPassR.prepare(spec);
        subLowPassL.reset();
        subLowPassR.reset();
        updateSubFilter();

        // Body layer: Band-pass 200 Hz - 2000 Hz
        bodyHighPassL.prepare(spec);
        bodyHighPassR.prepare(spec);
        bodyLowPassL.prepare(spec);
        bodyLowPassR.prepare(spec);
        bodyHighPassL.reset();
        bodyHighPassR.reset();
        bodyLowPassL.reset();
        bodyLowPassR.reset();
        updateBodyFilter();

        // Click layer: High-pass at 2000 Hz
        clickHighPassL.prepare(spec);
        clickHighPassR.prepare(spec);
        clickHighPassL.reset();
        clickHighPassR.reset();
        updateClickFilter();

        // Layer envelopes
        subEnvelope.setSampleRate(sampleRate);
        bodyEnvelope.setSampleRate(sampleRate);
        clickEnvelope.setSampleRate(sampleRate);

        // Smoothed values
        subVolSmoothed.reset(sampleRate, 0.02);
        bodyVolSmoothed.reset(sampleRate, 0.02);
        clickVolSmoothed.reset(sampleRate, 0.02);

        subPitchSmoothed.reset(sampleRate, 0.02);
        bodyPitchSmoothed.reset(sampleRate, 0.02);
        clickPitchSmoothed.reset(sampleRate, 0.02);
    }

    void setSubParameters(float volumeDb, float pitchSemitones, float decayMs)
    {
        subVolSmoothed.setTargetValue(juce::Decibels::decibelsToGain(volumeDb));
        subPitchSmoothed.setTargetValue(pitchSemitones);
        subDecayMs = decayMs;
    }

    void setBodyParameters(float volumeDb, float pitchSemitones, float decayMs)
    {
        bodyVolSmoothed.setTargetValue(juce::Decibels::decibelsToGain(volumeDb));
        bodyPitchSmoothed.setTargetValue(pitchSemitones);
        bodyDecayMs = decayMs;
    }

    void setClickParameters(float volumeDb, float pitchSemitones, float decayMs)
    {
        clickVolSmoothed.setTargetValue(juce::Decibels::decibelsToGain(volumeDb));
        clickPitchSmoothed.setTargetValue(pitchSemitones);
        clickDecayMs = decayMs;
    }

    void noteOn()
    {
        // Set up envelopes with current decay times
        juce::ADSR::Parameters subParams;
        subParams.attack = 0.001f;
        subParams.decay = subDecayMs / 1000.0f;
        subParams.sustain = 0.0f;
        subParams.release = 0.05f;
        subEnvelope.setParameters(subParams);
        subEnvelope.noteOn();

        juce::ADSR::Parameters bodyParams;
        bodyParams.attack = 0.001f;
        bodyParams.decay = bodyDecayMs / 1000.0f;
        bodyParams.sustain = 0.0f;
        bodyParams.release = 0.05f;
        bodyEnvelope.setParameters(bodyParams);
        bodyEnvelope.noteOn();

        juce::ADSR::Parameters clickParams;
        clickParams.attack = 0.0f;
        clickParams.decay = clickDecayMs / 1000.0f;
        clickParams.sustain = 0.0f;
        clickParams.release = 0.01f;
        clickEnvelope.setParameters(clickParams);
        clickEnvelope.noteOn();

        isActive = true;
    }

    void noteOff()
    {
        subEnvelope.noteOff();
        bodyEnvelope.noteOff();
        clickEnvelope.noteOff();
    }

    // Process a single sample and split into layers
    void processSample(float inputL, float inputR, float& outputL, float& outputR)
    {
        if (!isActive)
        {
            outputL = inputL;
            outputR = inputR;
            return;
        }

        // Get envelope values for each layer
        float subEnvVal = subEnvelope.getNextSample();
        float bodyEnvVal = bodyEnvelope.getNextSample();
        float clickEnvVal = clickEnvelope.getNextSample();

        // Check if all envelopes are done
        if (!subEnvelope.isActive() && !bodyEnvelope.isActive() && !clickEnvelope.isActive())
        {
            isActive = false;
        }

        // Get volume values
        float subVol = subVolSmoothed.getNextValue();
        float bodyVol = bodyVolSmoothed.getNextValue();
        float clickVol = clickVolSmoothed.getNextValue();

        // Split input into frequency bands
        // Sub layer (low frequencies < 200 Hz)
        float subL = subLowPassL.processSample(inputL);
        float subR = subLowPassR.processSample(inputR);

        // Click layer (high frequencies > 2000 Hz)
        float clickL = clickHighPassL.processSample(inputL);
        float clickR = clickHighPassR.processSample(inputR);

        // Body layer (mid frequencies 200-2000 Hz)
        // First high-pass to remove sub, then low-pass to remove click
        float bodyL = bodyHighPassL.processSample(inputL);
        bodyL = bodyLowPassL.processSample(bodyL);
        float bodyR = bodyHighPassR.processSample(inputR);
        bodyR = bodyLowPassR.processSample(bodyR);

        // Apply volume and envelope to each layer
        subL *= subVol * subEnvVal;
        subR *= subVol * subEnvVal;

        bodyL *= bodyVol * bodyEnvVal;
        bodyR *= bodyVol * bodyEnvVal;

        clickL *= clickVol * clickEnvVal;
        clickR *= clickVol * clickEnvVal;

        // Store levels for UI metering
        currentSubLevel = std::max(std::abs(subL), std::abs(subR));
        currentBodyLevel = std::max(std::abs(bodyL), std::abs(bodyR));
        currentClickLevel = std::max(std::abs(clickL), std::abs(clickR));

        // Mix all layers together
        outputL = subL + bodyL + clickL;
        outputR = subR + bodyR + clickR;
    }

    // Process without layer envelopes (for continuous processing)
    void processLayerMix(float inputL, float inputR, float& outputL, float& outputR)
    {
        // Get volume values
        float subVol = subVolSmoothed.getNextValue();
        float bodyVol = bodyVolSmoothed.getNextValue();
        float clickVol = clickVolSmoothed.getNextValue();

        // Split input into frequency bands
        float subL = subLowPassL.processSample(inputL);
        float subR = subLowPassR.processSample(inputR);

        float clickL = clickHighPassL.processSample(inputL);
        float clickR = clickHighPassR.processSample(inputR);

        float bodyL = bodyHighPassL.processSample(inputL);
        bodyL = bodyLowPassL.processSample(bodyL);
        float bodyR = bodyHighPassR.processSample(inputR);
        bodyR = bodyLowPassR.processSample(bodyR);

        // Apply volume to each layer
        subL *= subVol;
        subR *= subVol;
        bodyL *= bodyVol;
        bodyR *= bodyVol;
        clickL *= clickVol;
        clickR *= clickVol;

        // Store levels for UI
        currentSubLevel = std::max(std::abs(subL), std::abs(subR));
        currentBodyLevel = std::max(std::abs(bodyL), std::abs(bodyR));
        currentClickLevel = std::max(std::abs(clickL), std::abs(clickR));

        // Mix
        outputL = subL + bodyL + clickL;
        outputR = subR + bodyR + clickR;
    }

    float getSubLevel() const { return currentSubLevel; }
    float getBodyLevel() const { return currentBodyLevel; }
    float getClickLevel() const { return currentClickLevel; }

private:
    void updateSubFilter()
    {
        if (currentSampleRate > 0)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 200.0f, 0.707f);
            *subLowPassL.coefficients = *coeffs;
            *subLowPassR.coefficients = *coeffs;
        }
    }

    void updateBodyFilter()
    {
        if (currentSampleRate > 0)
        {
            auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 200.0f, 0.707f);
            auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 2000.0f, 0.707f);
            *bodyHighPassL.coefficients = *hpCoeffs;
            *bodyHighPassR.coefficients = *hpCoeffs;
            *bodyLowPassL.coefficients = *lpCoeffs;
            *bodyLowPassR.coefficients = *lpCoeffs;
        }
    }

    void updateClickFilter()
    {
        if (currentSampleRate > 0)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 2000.0f, 0.707f);
            *clickHighPassL.coefficients = *coeffs;
            *clickHighPassR.coefficients = *coeffs;
        }
    }

    double currentSampleRate = 44100.0;
    bool isActive = false;

    // Separate filters for left and right channels
    juce::dsp::IIR::Filter<float> subLowPassL, subLowPassR;
    juce::dsp::IIR::Filter<float> bodyHighPassL, bodyHighPassR;
    juce::dsp::IIR::Filter<float> bodyLowPassL, bodyLowPassR;
    juce::dsp::IIR::Filter<float> clickHighPassL, clickHighPassR;

    // Per-layer envelopes
    juce::ADSR subEnvelope;
    juce::ADSR bodyEnvelope;
    juce::ADSR clickEnvelope;

    // Decay times
    float subDecayMs = 500.0f;
    float bodyDecayMs = 300.0f;
    float clickDecayMs = 50.0f;

    // Smoothed values
    juce::SmoothedValue<float> subVolSmoothed{1.0f};
    juce::SmoothedValue<float> bodyVolSmoothed{1.0f};
    juce::SmoothedValue<float> clickVolSmoothed{1.0f};

    juce::SmoothedValue<float> subPitchSmoothed{0.0f};
    juce::SmoothedValue<float> bodyPitchSmoothed{0.0f};
    juce::SmoothedValue<float> clickPitchSmoothed{0.0f};

    // Level metering
    float currentSubLevel = 0.0f;
    float currentBodyLevel = 0.0f;
    float currentClickLevel = 0.0f;
};
