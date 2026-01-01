#include "KickMetrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

juce::String KickMetrics::toJson() const
{
    auto obj = juce::DynamicObject::Ptr(new juce::DynamicObject());
    obj->setProperty("peak_dbfs", peak_dbfs);
    obj->setProperty("true_peak_dbfs", true_peak_dbfs);
    obj->setProperty("rms_0_100ms_dbfs", rms_0_100ms_dbfs);
    obj->setProperty("crest_db", crest_db);
    obj->setProperty("attack_ms", attack_ms);
    obj->setProperty("t12_ms", t12_ms);
    obj->setProperty("t24_ms", t24_ms);
    obj->setProperty("tail_ms_to_-60db", tail_ms_to_minus60db);
    obj->setProperty("pitch_start_hz", pitch_start_hz);
    obj->setProperty("pitch_end_hz", pitch_end_hz);
    obj->setProperty("pitch_tau_ms", pitch_tau_ms);
    obj->setProperty("sub_20_60_over_body_60_200", sub_20_60_over_body_60_200);
    obj->setProperty("click_2k_10k_over_body_60_200", click_2k_10k_over_body_60_200);
    return juce::JSON::toString(juce::var(obj.get()), true);
}

KickMetrics KickMetrics::fromJson(const juce::var& obj)
{
    KickMetrics m;
    if (auto* o = obj.getDynamicObject())
    {
        juce::ignoreUnused(o);
        m.peak_dbfs = obj.getProperty("peak_dbfs", m.peak_dbfs);
        m.true_peak_dbfs = obj.getProperty("true_peak_dbfs", m.true_peak_dbfs);
        m.rms_0_100ms_dbfs = obj.getProperty("rms_0_100ms_dbfs", m.rms_0_100ms_dbfs);
        m.crest_db = obj.getProperty("crest_db", m.crest_db);
        m.attack_ms = obj.getProperty("attack_ms", m.attack_ms);
        m.t12_ms = obj.getProperty("t12_ms", m.t12_ms);
        m.t24_ms = obj.getProperty("t24_ms", m.t24_ms);
        m.tail_ms_to_minus60db = obj.getProperty("tail_ms_to_-60db", m.tail_ms_to_minus60db);
        m.pitch_start_hz = obj.getProperty("pitch_start_hz", m.pitch_start_hz);
        m.pitch_end_hz = obj.getProperty("pitch_end_hz", m.pitch_end_hz);
        m.pitch_tau_ms = obj.getProperty("pitch_tau_ms", m.pitch_tau_ms);
        m.sub_20_60_over_body_60_200 = obj.getProperty("sub_20_60_over_body_60_200", m.sub_20_60_over_body_60_200);
        m.click_2k_10k_over_body_60_200 = obj.getProperty("click_2k_10k_over_body_60_200", m.click_2k_10k_over_body_60_200);
    }
    return m;
}

KickAnalyzeResult KickAnalyzer::analyzeFile(const juce::File& file)
{
    KickAnalyzeResult result;
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (!reader)
        return result;

    juce::AudioBuffer<float> buffer((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    return analyzeBuffer(buffer, reader->sampleRate);
}

KickAnalyzeResult KickAnalyzer::analyzeBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    KickAnalyzeResult result;
    result.sample_rate = sampleRate;

    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return result;

    auto mono = toMono(buffer);
    const float* samples = mono.getReadPointer(0);
    int numSamples = mono.getNumSamples();

    int onset = findOnsetSample(samples, numSamples, sampleRate);
    onset = juce::jlimit(0, numSamples, onset);

    result.onset_sample = onset;
    result.trim_ms = (onset / sampleRate) * 1000.0;

    int trimmedSamples = numSamples - onset;
    result.trimmed_samples = trimmedSamples;
    if (trimmedSamples <= 0)
        return result;

    juce::AudioBuffer<float> trimmed(1, trimmedSamples);
    trimmed.copyFrom(0, 0, mono, 0, onset, trimmedSamples);

    const float* t = trimmed.getReadPointer(0);

    // Sample peak and true peak (dBFS).
    // Note: peak_dbfs may be > 0 when the input contains inter-sample or explicit sample clipping.
    double peak = 0.0;
    int peakIndex = 0;
    for (int i = 0; i < trimmedSamples; ++i)
    {
        double v = std::abs(t[i]);
        if (v > peak)
        {
            peak = v;
            peakIndex = i;
        }
    }

    result.metrics.peak_dbfs = juce::Decibels::gainToDecibels((float)peak, -120.0f);
    double truePeak = computeTruePeak(t, trimmedSamples);
    result.metrics.true_peak_dbfs = juce::Decibels::gainToDecibels((float)truePeak, -120.0f);

    // RMS 0-100ms
    int samples100ms = (int)std::round(sampleRate * 0.1);
    int rmsSamples = std::min(samples100ms, trimmedSamples);
    result.metrics.rms_0_100ms_dbfs = computeRmsDb(t, rmsSamples);
    result.metrics.crest_db = result.metrics.peak_dbfs - result.metrics.rms_0_100ms_dbfs;

    // Attack time (time from onset to maximum absolute sample peak).
    result.metrics.attack_ms = (peakIndex / sampleRate) * 1000.0;

    // Envelope timing metrics, measured on a peak-envelope to avoid zero-crossing artifacts.
    // Definition: threshold = peak * 10^(dB/20), where peak is the sample-peak of the trimmed audio.
    auto envelope = buildPeakEnvelope(t, trimmedSamples);

    double t12Thresh = peak * std::pow(10.0, -12.0 / 20.0);
    double t24Thresh = peak * std::pow(10.0, -24.0 / 20.0);
    double t60Thresh = peak * std::pow(10.0, -60.0 / 20.0);

    bool t12Found = false;
    bool t24Found = false;
    for (int i = peakIndex; i < trimmedSamples; ++i)
    {
        double v = envelope.empty() ? std::abs(t[i]) : envelope[(size_t)i];
        if (!t12Found && v <= t12Thresh)
        {
            result.metrics.t12_ms = ((i - peakIndex) / sampleRate) * 1000.0;
            t12Found = true;
        }
        if (!t24Found && v <= t24Thresh)
        {
            result.metrics.t24_ms = ((i - peakIndex) / sampleRate) * 1000.0;
            t24Found = true;
        }
        if (t12Found && t24Found)
            break;
    }

    // Tail to -60dB from onset
    result.metrics.tail_ms_to_minus60db = (trimmedSamples / sampleRate) * 1000.0;
    for (int i = peakIndex; i < trimmedSamples; ++i)
    {
        double v = envelope.empty() ? std::abs(t[i]) : envelope[(size_t)i];
        if (v <= t60Thresh)
        {
            result.metrics.tail_ms_to_minus60db = (i / sampleRate) * 1000.0;
            break;
        }
    }

    // Pitch tracking using YIN (band-limited + downsampled to avoid STFT-bin artifacts).
    const double totalMs = (trimmedSamples / sampleRate) * 1000.0;
    const double activeMs = juce::jlimit(0.0, totalMs,
                                         (result.metrics.tail_ms_to_minus60db > 0.0)
                                             ? result.metrics.tail_ms_to_minus60db
                                             : totalMs);

    // Use two band-limited pitch tracks:
    // - "high": tracks the early, higher-frequency part of a typical kick sweep.
    // - "low": suppresses upper harmonics (helps avoid octave errors later in the tail).
    auto pitchBandHigh = bandpass(t, trimmedSamples, sampleRate, 80.0, 300.0);
    auto pitchBandLow = bandpass(t, trimmedSamples, sampleRate, 20.0, 80.0);
    int dsFactor = 1;
    if (sampleRate >= 40000.0)
        dsFactor = 8;
    else if (sampleRate >= 20000.0)
        dsFactor = 4;
    else
        dsFactor = 2;

    auto downsample = [&](const std::vector<float>& src) -> std::vector<float>
    {
        std::vector<float> down;
        down.reserve(src.size() / (size_t)dsFactor + 1);
        for (int i = 0; i < (int)src.size(); i += dsFactor)
            down.push_back(src[(size_t)i]);
        return down;
    };

    const auto pitchDownHigh = downsample(pitchBandHigh);
    const auto pitchDownLow = downsample(pitchBandLow);
    const double pitchSampleRate = sampleRate / (double)dsFactor;

    auto analyzePitchTrack = [&](const std::vector<float>& signal,
                                 double windowMs,
                                 double hopMs,
                                 double minHz,
                                 double maxHz,
                                 std::vector<double>& outTimesMs,
                                 std::vector<double>& outPitchesHz)
    {
        const int windowSamples = std::max(64, (int)std::round(pitchSampleRate * (windowMs / 1000.0)));
        const int hopSamples = std::max(16, (int)std::round(pitchSampleRate * (hopMs / 1000.0)));
        if ((int)signal.size() < windowSamples)
            return;

        double peakAbs = 0.0;
        for (float v : signal)
            peakAbs = std::max(peakAbs, (double)std::abs(v));
        const double windowPeakGate = std::max(1e-7, peakAbs * 0.005); // about -46 dB relative

        const int maxStart = (int)signal.size() - windowSamples;
        outTimesMs.reserve(outTimesMs.size() + (size_t)maxStart / (size_t)hopSamples + 1);
        outPitchesHz.reserve(outTimesMs.capacity());

        for (int start = 0; start <= maxStart; start += hopSamples)
        {
            const double timeMs = (start / pitchSampleRate) * 1000.0; // window start time
            if (timeMs > activeMs)
                break;

            double wPeak = 0.0;
            for (int i = 0; i < windowSamples; ++i)
                wPeak = std::max(wPeak, (double)std::abs(signal[(size_t)(start + i)]));
            if (wPeak < windowPeakGate)
                continue;

            const double pitch = estimatePitchYin(signal.data() + start, windowSamples, pitchSampleRate, minHz, maxHz);
            if (pitch > 0.0)
            {
                outTimesMs.push_back(timeMs);
                outPitchesHz.push_back(pitch);
            }
        }
    };

    std::vector<double> timesHiMs, pitchesHiHz;
    std::vector<double> timesLoMs, pitchesLoHz;
    analyzePitchTrack(pitchDownHigh, 15.0, 3.0, 80.0, 300.0, timesHiMs, pitchesHiHz);
    analyzePitchTrack(pitchDownLow, 60.0, 10.0, 20.0, 80.0, timesLoMs, pitchesLoHz);

    auto pitchMedian = [&](const std::vector<double>& times,
                           const std::vector<double>& pitches,
                           double fromMs,
                           double toMs) -> double
    {
        std::vector<double> values;
        for (size_t i = 0; i < times.size(); ++i)
        {
            if (times[i] >= fromMs && times[i] <= toMs)
                values.push_back(pitches[i]);
        }
        if (values.empty())
            return 0.0;
        return median(values);
    };

    const double startWindowTo = std::min(30.0, activeMs);
    result.metrics.pitch_start_hz = pitchMedian(timesHiMs, pitchesHiHz, 0.0, startWindowTo);
    if (result.metrics.pitch_start_hz <= 0.0)
        result.metrics.pitch_start_hz = pitchMedian(timesLoMs, pitchesLoHz, 0.0, startWindowTo);

    const double endFromMs = (activeMs >= 180.0) ? 120.0 : std::max(0.0, activeMs - 30.0);
    const double endToMs = (activeMs >= 180.0) ? 180.0 : activeMs;
    result.metrics.pitch_end_hz = pitchMedian(timesLoMs, pitchesLoHz, endFromMs, endToMs);
    if (result.metrics.pitch_end_hz <= 0.0)
        result.metrics.pitch_end_hz = pitchMedian(timesHiMs, pitchesHiHz, endFromMs, endToMs);

    if (result.metrics.pitch_start_hz <= 0.0)
        result.metrics.pitch_start_hz = pitchMedian(timesHiMs, pitchesHiHz, 0.0, activeMs);
    if (result.metrics.pitch_start_hz <= 0.0)
        result.metrics.pitch_start_hz = pitchMedian(timesLoMs, pitchesLoHz, 0.0, activeMs);
    if (result.metrics.pitch_end_hz <= 0.0)
        result.metrics.pitch_end_hz = result.metrics.pitch_start_hz;

    struct FitPoint
    {
        double timeMs = 0.0;
        double hz = 0.0;
    };

    std::vector<FitPoint> fitPoints;
    fitPoints.reserve(timesHiMs.size() + timesLoMs.size());
    for (size_t i = 0; i < timesHiMs.size(); ++i)
        fitPoints.push_back({ timesHiMs[i], pitchesHiHz[i] });
    for (size_t i = 0; i < timesLoMs.size(); ++i)
        fitPoints.push_back({ timesLoMs[i], pitchesLoHz[i] });

    std::sort(fitPoints.begin(), fitPoints.end(), [](const FitPoint& a, const FitPoint& b) {
        return a.timeMs < b.timeMs;
    });

    std::vector<double> timesFitMs;
    std::vector<double> pitchesFitHz;
    timesFitMs.reserve(fitPoints.size());
    pitchesFitHz.reserve(fitPoints.size());
    for (const auto& p : fitPoints)
    {
        if (p.timeMs <= activeMs && p.hz > 0.0)
        {
            timesFitMs.push_back(p.timeMs);
            pitchesFitHz.push_back(p.hz);
        }
    }

    result.metrics.pitch_tau_ms = estimatePitchTau(timesFitMs, pitchesFitHz,
                                                   result.metrics.pitch_start_hz,
                                                   result.metrics.pitch_end_hz);

    // Spectral ratios
    double subRms = bandRms(t, trimmedSamples, sampleRate, 20.0, 60.0);
    double bodyRms = bandRms(t, trimmedSamples, sampleRate, 60.0, 200.0);
    double clickRms = bandRms(t, trimmedSamples, sampleRate, 2000.0, 10000.0);
    if (bodyRms > 0.0)
    {
        result.metrics.sub_20_60_over_body_60_200 = subRms / bodyRms;
        result.metrics.click_2k_10k_over_body_60_200 = clickRms / bodyRms;
    }

    return result;
}

juce::AudioBuffer<float> KickAnalyzer::toMono(const juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> mono(1, buffer.getNumSamples());
    if (buffer.getNumChannels() == 1)
    {
        mono.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    }
    else
    {
        mono.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            mono.addFrom(0, 0, buffer, ch, 0, buffer.getNumSamples(), 1.0f / buffer.getNumChannels());
    }
    return mono;
}

int KickAnalyzer::findOnsetSample(const float* samples, int numSamples, double sampleRate)
{
    if (numSamples <= 0 || sampleRate <= 0.0)
        return 0;

    // Short-term average absolute amplitude envelope.
    const int windowSamples = std::max(1, (int)std::round(sampleRate * 0.002)); // 2ms
    std::vector<double> prefix((size_t)numSamples + 1, 0.0);
    double globalPeak = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        double a = std::abs(samples[i]);
        prefix[(size_t)i + 1] = prefix[(size_t)i] + a;
        globalPeak = std::max(globalPeak, a);
    }

    if (globalPeak <= 0.0)
        return 0;

    auto avgAbsAt = [&](int center) -> double
    {
        int start = std::max(0, center - windowSamples / 2);
        int end = std::min(numSamples, center + windowSamples / 2 + 1);
        double sum = prefix[(size_t)end] - prefix[(size_t)start];
        return sum / std::max(1, end - start);
    };

    std::vector<double> env((size_t)numSamples, 0.0);
    for (int i = 0; i < numSamples; ++i)
        env[(size_t)i] = avgAbsAt(i);

    const int noiseSamples = std::min(numSamples, (int)std::round(sampleRate * 0.10)); // 100ms
    std::vector<double> noiseVals;
    noiseVals.reserve((size_t)noiseSamples);
    for (int i = 0; i < noiseSamples; ++i)
        noiseVals.push_back(env[(size_t)i]);

    const double noiseMedian = median(noiseVals);

    std::vector<double> deviations;
    deviations.reserve(noiseVals.size());
    for (double v : noiseVals)
        deviations.push_back(std::abs(v - noiseMedian));

    const double noiseMad = deviations.empty() ? 0.0 : median(deviations);

    // Threshold combines noise-floor estimate and a relative threshold to handle very clean silences.
    const double threshold = std::max({ noiseMedian + 6.0 * noiseMad, globalPeak * 0.01, 1e-6 });
    const double releaseThreshold = threshold * 0.5;

    const int holdSamples = std::max(1, (int)std::round(sampleRate * 0.003)); // 3ms

    for (int i = 0; i + holdSamples < numSamples; ++i)
    {
        if (env[(size_t)i] <= threshold)
            continue;

        bool sustained = true;
        for (int j = 1; j < holdSamples; ++j)
        {
            if (env[(size_t)(i + j)] < releaseThreshold)
            {
                sustained = false;
                break;
            }
        }

        if (!sustained)
            continue;

        // Refine onset by searching backwards for the last sample below the release threshold.
        const int lookback = std::max(0, i - (int)std::round(sampleRate * 0.01)); // 10ms
        for (int k = i; k > lookback; --k)
        {
            if (env[(size_t)k] < releaseThreshold)
                return k + 1;
        }

        return i;
    }

    return 0;
}

double KickAnalyzer::computeTruePeak(const float* samples, int numSamples)
{
    if (numSamples <= 0)
        return 0.0;

    juce::AudioBuffer<float> input(1, numSamples);
    input.copyFrom(0, 0, samples, numSamples);

    juce::dsp::Oversampling<float> oversampling(1, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
                                                true, false); // 4x, FIR, max quality
    oversampling.initProcessing((size_t)numSamples);
    oversampling.reset();

    const juce::AudioBuffer<float>& inputConst = input;
    juce::dsp::AudioBlock<const float> inputBlock(inputConst);
    auto upBlock = oversampling.processSamplesUp(inputBlock);

    double maxValue = 0.0;
    const float* up = upBlock.getChannelPointer(0);
    for (size_t i = 0; i < upBlock.getNumSamples(); ++i)
        maxValue = std::max(maxValue, (double)std::abs(up[i]));

    return maxValue;
}

double KickAnalyzer::computeRmsDb(const float* samples, int numSamples)
{
    if (numSamples <= 0)
        return -120.0;
    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sum += samples[i] * samples[i];
    double rms = std::sqrt(sum / numSamples);
    return juce::Decibels::gainToDecibels((float)rms, -120.0f);
}

std::vector<double> KickAnalyzer::buildPeakEnvelope(const float* samples, int numSamples)
{
    if (numSamples <= 0)
        return {};

    std::vector<double> absSamples((size_t)numSamples, 0.0);
    for (int i = 0; i < numSamples; ++i)
        absSamples[(size_t)i] = std::abs(samples[i]);

    std::vector<int> peaks;
    peaks.reserve((size_t)numSamples / 4);

    peaks.push_back(0);
    for (int i = 1; i < numSamples - 1; ++i)
    {
        double v = absSamples[(size_t)i];
        if (v >= absSamples[(size_t)(i - 1)] && v > absSamples[(size_t)(i + 1)])
            peaks.push_back(i);
    }
    if (numSamples > 1)
        peaks.push_back(numSamples - 1);

    // Remove duplicates (can occur for very short buffers).
    peaks.erase(std::unique(peaks.begin(), peaks.end()), peaks.end());

    std::vector<double> env((size_t)numSamples, 0.0);
    if (peaks.size() < 2)
    {
        env = absSamples;
        return env;
    }

    for (size_t p = 0; p + 1 < peaks.size(); ++p)
    {
        int i0 = peaks[p];
        int i1 = peaks[p + 1];
        double v0 = absSamples[(size_t)i0];
        double v1 = absSamples[(size_t)i1];
        int len = i1 - i0;

        if (len <= 0)
        {
            env[(size_t)i0] = std::max(env[(size_t)i0], v0);
            continue;
        }

        for (int i = i0; i <= i1; ++i)
        {
            double t = (double)(i - i0) / (double)len;
            env[(size_t)i] = v0 + (v1 - v0) * t;
        }
    }

    // Ensure the envelope never drops below the absolute value (robust against flat-top peaks).
    for (int i = 0; i < numSamples; ++i)
        env[(size_t)i] = std::max(env[(size_t)i], absSamples[(size_t)i]);

    return env;
}

std::vector<float> KickAnalyzer::bandpass(const float* samples, int numSamples, double sampleRate,
                                         double lowHz, double highHz)
{
    if (numSamples <= 0 || sampleRate <= 0.0)
        return {};

    std::vector<float> out((size_t)numSamples, 0.0f);

    juce::dsp::IIR::Filter<float> hp;
    juce::dsp::IIR::Filter<float> lp;
    const bool useHp = lowHz > 0.0;
    const bool useLp = highHz > 0.0 && highHz < (sampleRate * 0.5);

    if (useHp)
    {
        hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, lowHz);
        hp.reset();
    }

    if (useLp)
    {
        lp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, highHz);
        lp.reset();
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float v = samples[i];
        if (useHp)
            v = hp.processSample(v);
        if (useLp)
            v = lp.processSample(v);
        out[(size_t)i] = v;
    }

    return out;
}

double KickAnalyzer::estimatePitchYin(const float* samples, int numSamples, double sampleRate,
                                      double minHz, double maxHz)
{
    if (numSamples <= 0)
        return 0.0;

    double mean = 0.0;
    for (int i = 0; i < numSamples; ++i)
        mean += samples[i];
    mean /= numSamples;

    std::vector<double> x(numSamples);
    for (int i = 0; i < numSamples; ++i)
        x[i] = samples[i] - mean;

    int minTau = (int)std::floor(sampleRate / maxHz);
    int maxTau = (int)std::ceil(sampleRate / minHz);
    maxTau = std::min(maxTau, numSamples - 2);
    if (minTau < 2 || maxTau <= minTau)
        return 0.0;

    std::vector<double> diff(maxTau + 1, 0.0);
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        double sum = 0.0;
        for (int i = 0; i < numSamples - tau; ++i)
        {
            double d = x[i] - x[i + tau];
            sum += d * d;
        }
        diff[tau] = sum;
    }

    std::vector<double> cmndf(maxTau + 1, 1.0);
    double runningSum = 0.0;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        runningSum += diff[tau];
        cmndf[tau] = diff[tau] * tau / (runningSum + 1e-12);
    }

    // NOTE: Kick drums are strongly non-stationary; a more relaxed threshold + a cost that penalises
    // very long lags helps prevent the estimator from collapsing to the minimum frequency.
    const double threshold = 0.2;
    int tau = -1;
    for (int t = minTau; t <= maxTau; ++t)
    {
        if (cmndf[t] < threshold)
        {
            tau = t;
            while (tau + 1 <= maxTau && cmndf[tau + 1] < cmndf[tau])
                ++tau;
            break;
        }
    }

    if (tau == -1)
    {
        tau = minTau;
        double bestCost = cmndf[minTau] * (double)minTau;
        for (int t = minTau + 1; t <= maxTau; ++t)
        {
            double cost = cmndf[t] * (double)t;
            if (cost < bestCost)
            {
                bestCost = cost;
                tau = t;
            }
        }
    }

    double betterTau = tau;
    if (tau > 1 && tau < maxTau)
    {
        double c0 = cmndf[tau - 1];
        double c1 = cmndf[tau];
        double c2 = cmndf[tau + 1];
        double denom = (2.0 * c1 - c0 - c2);
        if (std::abs(denom) > 1e-9)
            betterTau = tau + (c0 - c2) / (2.0 * denom);
    }

    if (betterTau <= 0.0)
        return 0.0;
    return sampleRate / betterTau;
}

double KickAnalyzer::estimatePitchTau(const std::vector<double>& timesMs,
                                      const std::vector<double>& pitchesHz,
                                      double startHz, double endHz)
{
    if (timesMs.size() < 3 || startHz <= 0.0 || endHz <= 0.0 || startHz == endHz)
        return 0.0;

    std::vector<double> x;
    std::vector<double> y;
    x.reserve(timesMs.size());
    y.reserve(timesMs.size());

    for (size_t i = 0; i < timesMs.size(); ++i)
    {
        double p = pitchesHz[i];
        double denom = (startHz - endHz);
        double numer = (p - endHz);
        if (numer <= 0.0 || denom <= 0.0)
            continue;
        double val = std::log(numer / denom);
        if (std::isfinite(val))
        {
            x.push_back(timesMs[i]);
            y.push_back(val);
        }
    }

    if (x.size() < 3)
        return 0.0;

    double meanX = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
    double meanY = std::accumulate(y.begin(), y.end(), 0.0) / y.size();

    double num = 0.0;
    double den = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        num += (x[i] - meanX) * (y[i] - meanY);
        den += (x[i] - meanX) * (x[i] - meanX);
    }

    if (den <= 0.0)
        return 0.0;

    double slope = num / den;
    if (slope >= 0.0)
        return 0.0;

    double tauMs = -1.0 / slope;
    if (!std::isfinite(tauMs) || tauMs <= 0.0)
        return 0.0;

    // Prevent pathological fits from producing absurd time constants when the pitch trajectory
    // is nearly flat or dominated by noisy frames.
    const double maxTauMs = 2000.0;
    return std::min(tauMs, maxTauMs);
}

double KickAnalyzer::bandRms(const float* samples, int numSamples, double sampleRate,
                             double lowHz, double highHz)
{
    if (numSamples <= 0 || sampleRate <= 0.0)
        return 0.0;

    const bool useHp = lowHz > 0.0;
    const bool useLp = highHz > 0.0 && highHz < (sampleRate * 0.5);
    if (useHp && useLp && lowHz >= highHz)
        return 0.0;

    juce::dsp::IIR::Filter<float> hp;
    juce::dsp::IIR::Filter<float> lp;

    if (useHp)
    {
        hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, lowHz);
        hp.reset();
    }

    if (useLp)
    {
        lp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, highHz);
        lp.reset();
    }

    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float y = samples[i];
        if (useHp)
            y = hp.processSample(y);
        if (useLp)
            y = lp.processSample(y);
        sum += y * y;
    }
    return std::sqrt(sum / numSamples);
}

double KickAnalyzer::median(std::vector<double>& values)
{
    if (values.empty())
        return 0.0;
    size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    double m = values[mid];
    if (values.size() % 2 == 0)
    {
        std::nth_element(values.begin(), values.begin() + mid - 1, values.end());
        m = 0.5 * (m + values[mid - 1]);
    }
    return m;
}

KickMetricScore computeScore(const KickMetrics& target, const KickMetrics& actual)
{
    struct Term { double w; double scale; double a; double t; };
    std::vector<Term> terms = {
        {1.0, 6.0, actual.peak_dbfs, target.peak_dbfs},
        {1.0, 6.0, actual.rms_0_100ms_dbfs, target.rms_0_100ms_dbfs},
        {1.0, 6.0, actual.crest_db, target.crest_db},
        {1.0, 20.0, actual.attack_ms, target.attack_ms},
        {3.0, 10.0, actual.t12_ms, target.t12_ms},
        {3.0, 20.0, actual.t24_ms, target.t24_ms},
        {2.0, 100.0, actual.tail_ms_to_minus60db, target.tail_ms_to_minus60db},
        {4.0, 50.0, actual.pitch_start_hz, target.pitch_start_hz},
        {4.0, 50.0, actual.pitch_end_hz, target.pitch_end_hz},
        {4.0, 30.0, actual.pitch_tau_ms, target.pitch_tau_ms},
        {2.0, 20.0, actual.sub_20_60_over_body_60_200, target.sub_20_60_over_body_60_200},
        {2.0, 10.0, actual.click_2k_10k_over_body_60_200, target.click_2k_10k_over_body_60_200}
    };

    KickMetricScore score;
    for (const auto& term : terms)
    {
        double scale = term.scale > 0.0 ? term.scale : 1.0;
        double diff = (term.a - term.t) / scale;
        score.score += term.w * std::abs(diff);
    }
    return score;
}
