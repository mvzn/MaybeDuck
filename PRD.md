# Ducking VST — Product Requirements Document

## Overview
This is a flexible ducking dynamics VST designed for music production. It accepts two audio inputs—main and sidechain—and uses the sidechain level to modulate the main input's gain. Two working modes are provided:

- **Proportional**: a direct inverse-volume response where the amount of attenuation equals the amount by which the sidechain exceeds the threshold.
- **Gentle**: behaves like a compressor with user-adjustable ratio and knee, giving softer, more musical gain reduction.

Attack can be set fast or slow, and a hold parameter lets the reduced gain persist for a configurable time before a slow, melodic release takes over. Multi-band detection uses phase-coherent Linkwitz-Riley crossovers, and a post-ducking EQ tailors the output. The goal is high-quality, low-latency processing with intuitive controls for both technical and creative workflows.

## Goals
- Provide transparent, sample-accurate ducking where gain reduction (dB) = max(0, sidechain_level_dB - threshold_dB).
- Support multi-band detection using Linkwitz-Riley crossovers.
- Offer a slow, musical release by default; allow switching attack between fast and slow.
- Low CPU, low-latency, host-syncable plugin suitable for use in DAWs.

## Target Platforms
- VST3 on Windows/macOS/Linux (primary)
- AU on macOS (secondary)
- Implement using JUCE for cross-platform build.

## Inputs / Outputs
- Inputs: 2 audio inputs (Main, Sidechain)
- Outputs: 1 processed stereo (or mono) main output
- Sample rates supported: typical DAW rates (44.1kHz — 192kHz)

## UI / Controls (parameters)
- Threshold (dB): Range -60 dB to 0 dB, default -20 dB. (Sidechain detection threshold.)
- Working Mode: Enum {Proportional, Gentle}. Proportional is direct inverse ducking; Gentle behaves like a compressor with adjustable ratio/knee. Default: Proportional.
- Attack Mode: Enum {Fast, Slow}. Fast attack for tight transients, Slow for softer attacks. Default: Fast.
- Attack Time (ms): Derived from mode; configurable range 0.5 ms — 1000 ms. Defaults: Fast=5 ms, Slow=50 ms.
- Hold Time (ms): Range 0 ms — 2000 ms. Duration to maintain the ducked level before starting release. Default: 0 ms.
- Release Time (ms): Range 50 ms — 5000 ms. Default: 900 ms (musical, slow release). Release is intentionally slow/musical.
- Detector Type: Enum {RMS, Peak}, default RMS. (RMS recommended for musical behavior.)
- Lookahead (ms): Range 0 — 10 ms, default 3 ms (optional, reduces distortion on very fast attacks).
- Band Count: Integer 1 — 6 (number of bands created by Linkwitz-Riley crossovers), default 3.
- Crossover Frequencies: One frequency per crossover (Hz) — user-editable. Defaults for 3 bands: 200 Hz, 1000 Hz.
- Band Linking Mode: Enum {Single Detector (sum), Per-band ducking}, default Single Detector. (See signal flow.)
- Post-EQ: 3-band parametric (Gain dB, Frequency Hz, Q), defaults neutral. Applied after ducking.
- Output Makeup Gain (dB): -24 dB — +12 dB, default 0 dB.

## Parameter Defaults (quick)
- Threshold: -20 dB
- Working Mode: Proportional
- Attack: Fast (5 ms)
- Hold: 0 ms
- Release: 900 ms
- Bands: 3 (Crossovers: 200 Hz, 1000 Hz)
- Detector: RMS
- Lookahead: 3 ms

## Signal Flow
1. Main input passes through bypassable pre-EQ (optional) and into the final processing chain.
2. Sidechain input first optionally passes through the same band-splitting Linkwitz-Riley crossover as the detector path.
3. For each band (or the summed sidechain when Band Linking = Single Detector), compute a band envelope via chosen detector (RMS/Peak) with configurable attack/release and lookahead.
4. Compute instantaneous sidechain level in dB. Depending on Working Mode:
    - **Proportional**: duck_dB = max(0, sidechain_level_dB - threshold_dB).
    - **Gentle**: apply ratio/knee curve akin to a compressor; for example, duck_dB = comp_curve(sidechain_level_dB - threshold_dB) where comp_curve uses the ratio parameter.  
Apply Hold time: once duck_dB reaches its target, maintain that level for the configured hold duration before proceeding.
5. Apply attack/hold/release smoothing to duck_dB using the configured time constants. Attack uses chosen mode (fast/slow), release is intentionally slow/musical.
6. Convert smoothed duck_dB to linear gain reduction: gain = 10^( - duck_dB / 20 ). Apply to main signal (per-band if per-band ducking enabled, otherwise globally).
7. Apply post-EQ and makeup gain to produce final output.

Notes on mapping: This is a fully inverse volume modulation: if sidechain is 5 dB above threshold, the main is attenuated by 5 dB, preserving a direct inverse relationship.

## Crossovers and Filters
- Use Linkwitz-Riley crossovers (24 dB/octave; cascaded Butterworth pairs) to create phase-coherent bands.
- Support 1–6 bands. Crossover frequencies are user-specified; maintain ordering and frequency validity checks.
- Crossovers applied to the sidechain detector by default. Optionally apply same crossovers to main signal for per-band ducking (safer for avoiding cross-band smearing).

## Envelope Detector / Smoothing
- Detector: RMS window length configurable (e.g., 10–50 ms typical) or instantaneous peak.
- Working Mode handling: proportional vs gentle must be selectable; gentle mode requires implementing a compressor-style ratio curve and possibly a soft knee parameter.
- Attack smoothing: fast/slow selectable; implement as exponential smoothing with time-constants mapped to sample rate.
- Hold stage: after reaching target duck amount, maintain level for hold time before initiating release.
- Release smoothing: intentionally slow, melodic. Consider adaptive release shaping (longer release for larger duck amounts) to avoid pumping.
- Optional lookahead buffer to ensure attack accuracy while preventing audible clicks.

## Presets
- Default: Neutral with musical release.
- Vocal Sidechain: Threshold -30 dB, Attack Fast, Release 700 ms, Bands=2 (250/2000 Hz).
- Bass Duck: Threshold -18 dB, Attack Fast, Release 1200 ms, Bands=3 (80/400 Hz).

## Automation & Host Integration
- All parameters automatable by host.
- Expose meters for sidechain level, applied ducking (dB), and main output level.

## Performance & Latency
- Aim for <256-sample processing blocks without excessive CPU.
- Lookahead introduces latency equal to lookahead buffer; expose compensation flag for host delay.

## Quality Assurance / Testing
- Unit test envelope follower, crossover filters, and dB-to-linear mapping.
- Compare behaviour against simple offline reference: feed synthetic sidechain sine bursts to verify duck amount matches expected dB reduction.
- Listen tests with vocals/bass to verify musical release and absence of artifacts.

## Future Enhancements
- Sidechain EQ (pre-filter) with adjustable slope.
- Per-band ratio or knee for more flexible ducking curves.
- Sidechain MIDI-triggered ducking.

## Open Questions / Decisions
- Default detector (RMS vs Peak) — RMS recommended for musical material, but provide both.
- Whether crossovers should operate at higher internal oversampling for improved audio fidelity.

## Appendix: Mapping example
- If Threshold = -20 dB and sidechain_level = -15 dB, then sidechain exceeds threshold by 5 dB → duck_dB = 5 dB → main is reduced by 5 dB (gain = 10^(-5/20) ≈ 0.562).

## Future Main Developments
The following items are planned as major future developments; each entry includes technical considerations and integration notes.

- Ability to export and modify the volume automation in DAW
	- Technical considerations:
		- Host integration: expose the ducking gain parameter (per-band or global) as an automatable parameter stream that the host can record. For lookahead-induced latency, ensure proper host delay compensation and report plugin latency so automation aligns correctly with audio.
		- Snapshot export: provide an option to export the computed gain curve (time-stamped gain values) as a standard automation format (e.g., host-specific automation lanes or as a MIDI CC / textual CSV) so the user can import and edit it in the DAW.
		- Resolution and smoothing: choose an export resolution (e.g., 1 ms–10 ms grid) to balance file size and fidelity; include optional smoothing or decimation to reduce edit noise in the DAW.
		- Real-time vs offline: for live export, stream automation points via host parameter updates; for deterministic offline export, implement an offline rendering mode that writes a high-resolution automation file synchronized to project sample rate and tempo map.

- Scaling for each filter band so the ducking for each band can vary
	- Technical considerations:
		- Per-band gain scaling: add per-band duck scaling parameters (linear or dB) that multiply the computed duck_dB before conversion to linear gain. Provide sensible defaults (e.g., 0 dB scaling) and allow negative/positive offsets to reduce or exaggerate band-specific ducking.
		- Detection-routing: decide whether scaling applies to detector output (modify detector envelope) or to the computed duck amount (post-detector). Post-detector scaling preserves threshold behavior and is recommended to keep the threshold semantics consistent across bands.
		- GUI and automation: expose per-band scaling as automatable parameters and provide clear visualization (per-band duck meters and overlay of final applied gain).
		- Phase and summing implications: when applying different scaling across bands, ensure the main signal's recombination does not introduce level discontinuities; maintain Linkwitz-Riley phase-coherent crossovers and consider small smoothing to avoid inter-band pumping.
		- CPU and data paths: per-band scaling is lightweight, but per-band ducking requires applying independent gain to each band of the main signal — design with SIMD-friendly loops or per-band processing blocks to remain efficient.
