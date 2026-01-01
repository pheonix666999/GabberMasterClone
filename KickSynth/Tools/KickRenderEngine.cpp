#include "KickRenderEngine.h"

void KickRenderEngine::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    distortion.prepare(sampleRate, 512);
    limiter.prepare(sampleRate, 512);
    outputHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    outputHPF.coefficients = outputHPFCoeffs;
    outputHPF.reset();
}

void KickRenderEngine::render(const KickParams& params, float velocity, juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() == 0)
        return;

    // Setup voice parameters
    voice.setBodyLevel(params.bodyLevel);
    voice.setClickLevel(params.clickLevel);
    voice.setPitchStartHz(params.pitchStartHz);
    voice.setPitchEndHz(params.pitchEndHz);
    voice.setPitchTauMs(params.pitchTauMs);
    voice.setAttackMs(params.attackMs);
    voice.setT12Ms(params.t12Ms);
    voice.setT24Ms(params.t24Ms);
    voice.setTailMsToMinus60Db(params.tailMsToMinus60Db);
    voice.setClickDecayMs(params.clickDecayMs);
    voice.setClickHPFHz(params.clickHPFHz);
    voice.setBodyOscillatorType(params.bodyOscType);
    voice.setKeyTracking(params.keyTracking);
    voice.setRetriggerMode(params.retriggerMode);
    voice.setVelocitySensitivity(params.velocitySensitivity);

    distortion.setDistortionType(params.distortionType);
    distortion.setDrive(params.drive);
    distortion.setAsymmetry(params.asymmetry);

    limiter.setLimiterEnabled(params.limiterEnabled);
    limiter.setOutputGain(params.outputGainDb);

    outputHPFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, params.outputHPFHz);
    outputHPF.coefficients = outputHPFCoeffs;
    outputHPF.reset();

    // Trigger voice
    voice.noteOn(60, velocity, currentSampleRate);

    buffer.clear();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float sample = voice.renderSample();
        buffer.setSample(0, i, sample);
    }

    // Apply output HPF
    auto* data = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        data[i] = outputHPF.processSample(data[i]);

    // Distortion & limiter
    distortion.processBlock(buffer);
    limiter.processBlock(buffer);
}
