# Implementation Skills & Staffing Guide

## Core Engineering Skills
- C++ (Modern C++17/C++20): Primary language for real-time audio plugin development.
- JUCE framework: Cross-platform audio, UI, and plugin scaffolding (VST3/AU support).
- VST3 SDK and AudioUnit knowledge: Host integration, parameter automation, latency reporting.
- Real-time DSP: Filters, crossovers (Linkwitz-Riley), oversampling, anti-aliasing techniques.
- Envelope detection & dynamics: RMS and peak detectors, lookahead buffers, time-constant design.

## DSP & Math
- Signal processing math: dB/linear conversions, smoothing filters, IIR/FIR design.
- Filter design: Butterworth cascades for Linkwitz-Riley crossovers (24 dB/octave), filter stability and coefficient calculation.
- Numerical stability: Avoid denormals, implement safe math for small numbers, sample-rate aware algorithms.

## Performance & Optimization
- SIMD / vectorization (Optional): For heavy multi-band or oversampled processing.
- Multi-threading & lock-free audio patterns: Understand audio thread constraints and use real-time safe approaches.
- Profiling: Use instruments (Linux perf, Xcode Instruments, Windows VTune) and CPU meters.

## Testing & Validation
- Unit testing (Catch2 / GoogleTest) for core DSP primitives.
- Automated signal tests: regression tests using test audio to assert envelope and gain reduction expectations.
- Listening tests and QA workflows for artifact detection (pumping, clicks, distortion).

## Build, CI & Packaging
- CMake / Projucer / JUCE project setup: Cross-platform building and IDE integration.
- CI pipelines: GitHub Actions or similar to build across platforms.
- Installer/packaging knowledge for distributing VST3/AU binaries and notarization for macOS.

## UX / Frontend
- GUI design for audio plugins; efficient repaint strategies and parameter smoothing for UI.
- Metering and visualization: sidechain meters, duck amount readout, band displays.

## Optional / Nice-to-have
- Rust bindings or DSP prototyping (Rust/FAUST) for algorithm exploration.
- Faust or MATLAB for offline algorithm prototyping.
- Experience with plugin certification and plugin store submission process.

## Suggested Team Roles
- DSP Engineer: Core algorithm and filter/envelope design.
- C++/JUCE Engineer: Plugin scaffolding, host integration, performance tuning.
- UI/UX Engineer: GUI, preset system, automation mapping.
- QA / Audio Engineer: Listening tests, preset creation, regression tests.

## Quick Hiring Checklist
- Senior DSP: 5+ years audio DSP, familiarity with crossovers and dynamic processors.
- JUCE developer: 3+ years, shipping at least one plugin.
- Audio QA: skilled at blind A/B testing and identifying subtle artifacts.
