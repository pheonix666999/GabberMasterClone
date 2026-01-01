# KickSynth - Measurable Kick Drum Synthesizer

A JUCE-based VST3/standalone kick drum instrument that exposes every parameter needed to match a reference Gabber-style kick using objective metrics and accompanying CLI helpers.

## Features

### Synthesis Engine
- **Body oscillator** (sine or triangle) with a fast exponential pitch sweep from `PitchStartHz` to `PitchEndHz` controlled by `PitchTauMs`.
- **Click layer**: white-noise burst with `ClickHPFHz`, `ClickLevel`, and `ClickDecayMs` to recreate the transient click.
- **Amplitude shaping**: attack + exponential decay, with `T12`, `T24`, and `Tail` timing knobs that are measured directly by the analyzer.
- **Distortion**: three modes (`Tanh`, `Hard Clip`, `Asymmetric Soft Clip`) with `Drive` and `Asymmetry` controls.
- **Output conditioning**: `OutputHPFHz` to tame rogue subs, a soft limiter that keeps the signal under 0 dBFS, and a `Velocity Sensitivity` knob that keeps kicks loud even with low MIDI velocity.
- **Limiter & retrigger**: `LimiterEnabled` plus `RetriggerMode` let you chase either a gated pattern or a one-shot kick.

### Metrics-aware Controls
Every slider maps back to a measurable metric (peak dBFS, attack, decay times, spectral ratios, pitch sweep) so you can match a reference kick by watching the analyzer output.

### GUI Trigger
Use the `Go` button to trigger a kick immediately from the GUI without MIDI. The processor listens for this button and fires the shared voice with the current parameter set and velocity, so you can audition changes instantly and keep dialing the sound.

## Building

### Prerequisites
- Visual Studio 2022 / MSVC 2019+ (Windows)
- CMake 3.22+
- JUCE framework located in `../JUCE`

### Build Steps
```powershell
cd KickSynth
cmake -B build -S .
cmake --build build --config Release
```

### Artifacts
- `build/KickSynth_artefacts/Release/VST3/Kick Synth.vst3`
- `build/KickSynth_artefacts/Release/Standalone/Kick Synth.exe`
- `build/Release/kick_analyze.exe`
- `build/Release/kick_fit.exe`
- `build/Release/kick_render.exe`

## Command-line Tools

These tools are ideal for automating the target-matching workflow without a DAW.

### `kick_analyze`
```
kick_analyze <input.wav> [--out metrics.json] [--verbose]
```
Performs silence trimming, true-peak estimation (4x oversample), RMS/crest timing, YIN-based pitch tracking, and spectral ratio measurements. Outputs clean JSON (and optionally writes it to disk) for use in the next steps.

### `kick_fit`
```
kick_fit <target_metrics.json> [--out suggested_params.json] [--iters 200]
```
Randomly samples the key synthesis parameters, renders kicks with the same engine, re-analyzes them, and ranks candidates by a weighted error score (pitch & decay timing are prioritized). Prints the top-10 table and writes the best parameter set when `--out` is provided.

### `kick_render`
```
kick_render <params.json> <output.wav> [--sr 48000] [--length-ms 500] [--velocity 1.0]
```
Renders a single hit with the current parameter set so you can quickly audition it, re-analyze it, and keep iterating without going through a DAW project.

## Matching Workflow
1. Record a reference kick in WAV (ideally 48 kHz) and run `kick_analyze target.wav --out target_metrics.json`.
2. Run `kick_fit target_metrics.json --out suggested_params.json` to get a starting point that prioritizes attack time, pitch sweep, and decay.
3. Load the plugin (VST3 or standalone) and load the suggested JSON (or dial in the knobs manually).
4. Render a preview with `kick_render suggested_params.json rendered.wav` and compare it to the reference with `kick_analyze rendered.wav`.
5. Repeat steps 2–4 or tweak individual controls to polish the match.

## Plugin Parameters

| Section | Parameter | Description |
|---|---|---|
| **Body** | `BodyLevel`, `Body Osc Type` | Controls the main sine/triangle oscillator. |
| **Pitch Envelope** | `PitchStartHz`, `PitchEndHz`, `PitchTauMs`, `Key Tracking` | Defines the exponential sweep that creates the classic gabber drop. |
| **Amplitude Envelope** | `AttackMs`, `T12Ms`, `T24Ms`, `TailMsToMinus60Db` | Attack + decay times that align with analyzer metrics. |
| **Click Layer** | `ClickLevel`, `ClickDecayMs`, `ClickHPFHz`, `Velocity Sensitivity` | Shapes the transient and ensures dynamic consistency. |
| **Distortion** | `DistortionType`, `Drive`, `Asymmetry` | Choose between soft/hard clipping models and dial the harshness. |
| **Output** | `OutputGain`, `OutputHPFHz`, `LimiterEnabled`, `RetriggerMode` | Final gain, sub-cut HPF, limiter, and retrigger behaviour. |

## Notes
- The analyzer works on trimmed samples and reports both sample-peak and true-peak.
- `kick_fit` uses the same kick engine as the plugin so its suggestions stay accurate.
- Velocity-sensitive kicks stay loud even when the MIDI velocity is low; crank `Velocity Sensitivity` to match original waveforms.


