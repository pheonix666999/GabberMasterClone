#include "KickMetrics.h"
#include "KickRenderEngine.h"
#include "KickParams.h"
#include <iostream>
#include <array>
#include <random>
#include <algorithm>
#include <cmath>
#include <iomanip>

struct Candidate
{
    KickParams params;
    KickMetrics metrics;
    double score = 0.0;
};

static float clampFloat(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}

static KickParams clampParams(KickParams p)
{
    p.bodyLevel = clampFloat(p.bodyLevel, 0.0f, 1.0f);
    p.clickLevel = clampFloat(p.clickLevel, 0.0f, 1.0f);
    p.pitchStartHz = clampFloat(p.pitchStartHz, 20.0f, 500.0f);
    p.pitchEndHz = clampFloat(p.pitchEndHz, 20.0f, 200.0f);
    p.pitchTauMs = clampFloat(p.pitchTauMs, 1.0f, 100.0f);
    p.attackMs = clampFloat(p.attackMs, 0.0f, 50.0f);
    p.t12Ms = clampFloat(p.t12Ms, 1.0f, 50.0f);
    p.t24Ms = clampFloat(p.t24Ms, 5.0f, 200.0f);
    p.tailMsToMinus60Db = clampFloat(p.tailMsToMinus60Db, 10.0f, 500.0f);
    p.clickDecayMs = clampFloat(p.clickDecayMs, 1.0f, 20.0f);
    p.clickHPFHz = clampFloat(p.clickHPFHz, 500.0f, 10000.0f);
    p.bodyOscType = std::max(0, std::min(p.bodyOscType, 1));
    p.keyTracking = clampFloat(p.keyTracking, -12.0f, 12.0f);
    p.distortionType = std::max(0, std::min(p.distortionType, 2));
    p.drive = clampFloat(p.drive, 0.0f, 1.0f);
    p.asymmetry = clampFloat(p.asymmetry, 0.0f, 1.0f);
    p.outputGainDb = clampFloat(p.outputGainDb, -12.0f, 12.0f);
    p.outputHPFHz = clampFloat(p.outputHPFHz, 20.0f, 200.0f);
    p.velocitySensitivity = clampFloat(p.velocitySensitivity, 0.0f, 1.0f);
    return p;
}

static KickParams baseFromTarget(const KickMetrics& target)
{
    KickParams p;
    p.pitchStartHz = clampFloat((float)target.pitch_start_hz, 20.0f, 500.0f);
    p.pitchEndHz = clampFloat((float)target.pitch_end_hz, 20.0f, 200.0f);
    p.pitchTauMs = clampFloat((float)target.pitch_tau_ms, 1.0f, 100.0f);
    p.attackMs = clampFloat((float)target.attack_ms, 0.0f, 50.0f);
    p.t12Ms = clampFloat((float)target.t12_ms, 1.0f, 50.0f);
    p.t24Ms = clampFloat((float)target.t24_ms, 5.0f, 200.0f);
    p.tailMsToMinus60Db = clampFloat((float)target.tail_ms_to_minus60db, 10.0f, 500.0f);
    p.clickLevel = clampFloat((float)(target.click_2k_10k_over_body_60_200 / 10.0), 0.0f, 1.0f);
    p.bodyLevel = clampFloat((float)(target.sub_20_60_over_body_60_200 / 50.0), 0.1f, 1.0f);
    p.drive = clampFloat((float)(target.crest_db / 20.0), 0.0f, 1.0f);
    p.outputGainDb = clampFloat((float)target.peak_dbfs, -12.0f, 12.0f);
    return clampParams(p);
}

static Candidate evaluateCandidate(const KickParams& rawParams,
                                   const KickMetrics& target,
                                   KickRenderEngine& engine,
                                   juce::AudioBuffer<float>& buffer,
                                   double sampleRate)
{
    KickParams params = clampParams(rawParams);

    const double lengthMs = std::max(300.0, (double)params.tailMsToMinus60Db + 200.0);
    const int numSamples = std::max(1, (int)std::round(sampleRate * (lengthMs / 1000.0)));
    buffer.setSize(1, numSamples, false, false, true);

    engine.render(params, 1.0f, buffer);
    auto result = KickAnalyzer::analyzeBuffer(buffer, sampleRate);
    double score = computeScore(target, result.metrics).score;
    return { params, result.metrics, score };
}

static Candidate refineCoordinateDescent(const Candidate& start,
                                        const KickMetrics& target,
                                        KickRenderEngine& engine,
                                        juce::AudioBuffer<float>& buffer,
                                        double sampleRate,
                                        int refineIters)
{
    Candidate best = start;

    struct FloatParam
    {
        const char* name = "";
        float KickParams::* member = nullptr;
        float lo = 0.0f;
        float hi = 1.0f;
        float step = 0.01f;
        float minStep = 0.001f;
    };

    std::vector<FloatParam> params = {
        {"pitchStartHz", &KickParams::pitchStartHz, 20.0f, 500.0f, 20.0f, 1.0f},
        {"pitchEndHz", &KickParams::pitchEndHz, 20.0f, 200.0f, 10.0f, 1.0f},
        {"pitchTauMs", &KickParams::pitchTauMs, 1.0f, 100.0f, 5.0f, 0.25f},
        {"attackMs", &KickParams::attackMs, 0.0f, 50.0f, 2.0f, 0.1f},
        {"t12Ms", &KickParams::t12Ms, 1.0f, 50.0f, 2.0f, 0.1f},
        {"t24Ms", &KickParams::t24Ms, 5.0f, 200.0f, 5.0f, 0.25f},
        {"tailMsToMinus60Db", &KickParams::tailMsToMinus60Db, 10.0f, 500.0f, 20.0f, 1.0f},
        {"bodyLevel", &KickParams::bodyLevel, 0.0f, 1.0f, 0.05f, 0.005f},
        {"clickLevel", &KickParams::clickLevel, 0.0f, 1.0f, 0.05f, 0.005f},
        {"clickDecayMs", &KickParams::clickDecayMs, 1.0f, 20.0f, 1.0f, 0.1f},
        {"clickHPFHz", &KickParams::clickHPFHz, 500.0f, 10000.0f, 400.0f, 25.0f},
        {"drive", &KickParams::drive, 0.0f, 1.0f, 0.05f, 0.005f},
        {"asymmetry", &KickParams::asymmetry, 0.0f, 1.0f, 0.05f, 0.005f},
        {"outputGainDb", &KickParams::outputGainDb, -12.0f, 12.0f, 1.0f, 0.1f},
        {"outputHPFHz", &KickParams::outputHPFHz, 20.0f, 200.0f, 10.0f, 1.0f},
        {"velocitySensitivity", &KickParams::velocitySensitivity, 0.0f, 1.0f, 0.05f, 0.005f},
    };

    for (int iter = 0; iter < refineIters; ++iter)
    {
        bool improved = false;

        for (auto& spec : params)
        {
            const float center = best.params.*(spec.member);

            KickParams testMinus = best.params;
            testMinus.*(spec.member) = clampFloat(center - spec.step, spec.lo, spec.hi);
            auto candMinus = evaluateCandidate(testMinus, target, engine, buffer, sampleRate);

            KickParams testPlus = best.params;
            testPlus.*(spec.member) = clampFloat(center + spec.step, spec.lo, spec.hi);
            auto candPlus = evaluateCandidate(testPlus, target, engine, buffer, sampleRate);

            if (candMinus.score < best.score && candMinus.score <= candPlus.score)
            {
                best = std::move(candMinus);
                improved = true;
            }
            else if (candPlus.score < best.score)
            {
                best = std::move(candPlus);
                improved = true;
            }
        }

        if (improved)
            continue;

        bool anyStepReduced = false;
        for (auto& spec : params)
        {
            float next = std::max(spec.minStep, spec.step * 0.5f);
            if (next < spec.step)
            {
                spec.step = next;
                anyStepReduced = true;
            }
        }

        if (!anyStepReduced)
            break;
    }

    // Discrete sweep for oscillator / distortion types.
    Candidate bestDiscrete = best;
    for (int osc = 0; osc <= 1; ++osc)
    {
        for (int dist = 0; dist <= 2; ++dist)
        {
            KickParams p = best.params;
            p.bodyOscType = osc;
            p.distortionType = dist;
            auto cand = evaluateCandidate(p, target, engine, buffer, sampleRate);
            if (cand.score < bestDiscrete.score)
                bestDiscrete = std::move(cand);
        }
    }

    return bestDiscrete;
}

static void printTop(const std::vector<Candidate>& candidates, int topN)
{
    topN = std::min(topN, (int)candidates.size());
    if (topN <= 0)
        return;

    std::cout << "Top " << topN << " candidates:\n";
    std::cout << "  #   score   pitchStart  pitchEnd  pitchTau   t12   t24  tail  click  drive  dist osc  outGain\n";

    for (int i = 0; i < topN; ++i)
    {
        const auto& c = candidates[(size_t)i];
        std::cout << std::setw(3) << (i + 1)
                  << std::setw(8) << std::fixed << std::setprecision(3) << c.score
                  << std::setw(11) << std::setprecision(1) << c.params.pitchStartHz
                  << std::setw(10) << c.params.pitchEndHz
                  << std::setw(10) << c.params.pitchTauMs
                  << std::setw(6) << c.params.t12Ms
                  << std::setw(6) << c.params.t24Ms
                  << std::setw(6) << c.params.tailMsToMinus60Db
                  << std::setw(7) << std::setprecision(2) << c.params.clickLevel
                  << std::setw(7) << c.params.drive
                  << std::setw(6) << c.params.distortionType
                  << std::setw(4) << c.params.bodyOscType
                  << std::setw(9) << std::setprecision(1) << c.params.outputGainDb
                  << "\n";
    }
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc < 2)
    {
        std::cout << "Usage: kick_fit <target_metrics.json> [--target-wav target.wav] [--out params.json]"
                     " [--iters 200] [--refine 80] [--seed 42] [--sr 48000]\n";
        return 1;
    }

    juce::File targetFile(argv[1]);
    if (!targetFile.existsAsFile())
    {
        std::cout << "Error: target metrics file not found\n";
        return 1;
    }

    int iterations = 200;
    int refineIters = 80;
    int seed = 42;
    double sampleRate = 48000.0;
    juce::File outFile;
    juce::File targetWavFile;
    for (int i = 2; i < argc; ++i)
    {
        juce::String arg = argv[i];
        if (arg == "--iters" && i + 1 < argc)
            iterations = juce::String(argv[++i]).getIntValue();
        else if (arg == "--refine" && i + 1 < argc)
            refineIters = juce::String(argv[++i]).getIntValue();
        else if (arg == "--seed" && i + 1 < argc)
            seed = juce::String(argv[++i]).getIntValue();
        else if (arg == "--sr" && i + 1 < argc)
            sampleRate = juce::String(argv[++i]).getDoubleValue();
        else if (arg == "--target-wav" && i + 1 < argc)
            targetWavFile = juce::File(argv[++i]);
        else if (arg == "--out" && i + 1 < argc)
            outFile = juce::File(argv[++i]);
    }

    KickMetrics target;
    if (targetWavFile.existsAsFile())
    {
        auto wavResult = KickAnalyzer::analyzeFile(targetWavFile);
        target = wavResult.metrics;
        std::cout << "Using target metrics measured from: " << targetWavFile.getFullPathName() << "\n";
        std::cout << target.toJson() << "\n";
    }
    else
    {
        auto targetJson = juce::JSON::parse(targetFile.loadFileAsString());
        if (targetJson.isVoid() || targetJson.isUndefined())
        {
            std::cout << "Error: invalid JSON in target metrics\n";
            return 1;
        }
        target = KickMetrics::fromJson(targetJson);
    }

    KickParams base = baseFromTarget(target);

    std::mt19937 rng((uint32_t)seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distScale(0.7f, 1.3f);

    std::vector<Candidate> candidates;
    candidates.reserve((size_t)iterations + 300);

    KickRenderEngine engine;
    engine.prepare(sampleRate);
    juce::AudioBuffer<float> renderBuffer;

    // Coarse grid over the most impactful parameters (pitch + decay + click + drive).
    const std::array<float, 3> mul = { 0.75f, 1.0f, 1.25f };
    const std::array<float, 3> clickVals = { 0.15f, 0.5f, 0.85f };
    const std::array<float, 3> driveVals = { 0.15f, 0.5f, 0.85f };

    for (float mStart : mul)
        for (float mTau : mul)
            for (float mDecay : mul)
                for (float click : clickVals)
                    for (float drive : driveVals)
                    {
                        KickParams p = base;
                        p.pitchStartHz = clampFloat(p.pitchStartHz * mStart, 20.0f, 500.0f);
                        p.pitchTauMs = clampFloat(p.pitchTauMs * mTau, 1.0f, 100.0f);
                        p.t12Ms = clampFloat(p.t12Ms * mDecay, 1.0f, 50.0f);
                        p.t24Ms = clampFloat(p.t24Ms * mDecay, 5.0f, 200.0f);
                        p.clickLevel = click;
                        p.drive = drive;
                        candidates.push_back(evaluateCandidate(p, target, engine, renderBuffer, sampleRate));
                    }

    for (int i = 0; i < iterations; ++i)
    {
        KickParams p = base;
        p.pitchStartHz = clampFloat(p.pitchStartHz * distScale(rng), 20.0f, 500.0f);
        p.pitchEndHz = clampFloat(p.pitchEndHz * distScale(rng), 20.0f, 200.0f);
        p.pitchTauMs = clampFloat(p.pitchTauMs * distScale(rng), 1.0f, 100.0f);
        p.attackMs = clampFloat(p.attackMs * distScale(rng), 0.0f, 50.0f);
        p.t12Ms = clampFloat(p.t12Ms * distScale(rng), 1.0f, 50.0f);
        p.t24Ms = clampFloat(p.t24Ms * distScale(rng), 5.0f, 200.0f);
        p.tailMsToMinus60Db = clampFloat(p.tailMsToMinus60Db * distScale(rng), 10.0f, 500.0f);
        p.clickLevel = clampFloat(dist01(rng), 0.0f, 1.0f);
        p.bodyLevel = clampFloat(dist01(rng), 0.1f, 1.0f);
        p.drive = clampFloat(dist01(rng), 0.0f, 1.0f);
        p.asymmetry = clampFloat(dist01(rng), 0.0f, 1.0f);
        p.clickDecayMs = clampFloat(p.clickDecayMs * distScale(rng), 1.0f, 20.0f);
        p.clickHPFHz = clampFloat(p.clickHPFHz * distScale(rng), 500.0f, 10000.0f);
        p.outputHPFHz = clampFloat(p.outputHPFHz * distScale(rng), 20.0f, 200.0f);
        p.bodyOscType = (dist01(rng) > 0.5f) ? 1 : 0;
        p.distortionType = (int)std::floor(dist01(rng) * 3.0f);
        p.outputGainDb = clampFloat(base.outputGainDb + (dist01(rng) - 0.5f) * 6.0f, -12.0f, 12.0f);

        candidates.push_back(evaluateCandidate(p, target, engine, renderBuffer, sampleRate));
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score < b.score;
    });

    printTop(candidates, 10);

    if (!candidates.empty() && refineIters > 0)
    {
        std::cout << "\nRefining best candidate (" << refineIters << " iters)...\n";
        auto refined = refineCoordinateDescent(candidates.front(), target, engine, renderBuffer, sampleRate, refineIters);
        candidates.push_back(refined);
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.score < b.score;
        });
        printTop(candidates, 10);
    }

    if (!candidates.empty())
    {
        if (outFile != juce::File())
        {
            saveKickParamsJson(outFile, candidates.front().params);
            std::cout << "Saved best params to: " << outFile.getFullPathName() << "\n";
        }
        else
        {
            std::cout << "\nBest params JSON:\n" << candidates.front().params.toJson() << "\n";
        }
    }

    return 0;
}
