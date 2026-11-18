# RESYNTHESIS

A JUCE-based VST3 audio effect inspired by the workflow and visual language of CRQL RESYNTH. This is an independent implementation, not a copy of CRQL's code or assets.

## Current project direction

- Windows-first VST3 effect for FL Studio
- Built-in sine, saw, square, triangle and noise tables
- Runtime WAV wavetable import (no need to rebuild the plugin to add tables)
- XY processing surface and phase / transpose / shift / smoothing controls
- Mid/side processing modes
- Preset/state persistence, including imported table data
- Dark charcoal interface with orange and green accents

## Build

Requirements:
- Windows 10/11 x64
- Visual Studio 2022 with the C++ desktop workload
- CMake 3.22+
- Git

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target RESYNTHESIS_VST3
```

JUCE is fetched by CMake. Review JUCE's current licensing terms before distributing binaries or source.

## Status

Early development scaffold. A successful build and DAW validation are required before treating any build as release-ready.
