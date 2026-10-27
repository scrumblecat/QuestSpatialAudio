# Quest Spatial Audio

An experimental Beat Saber mod for standalone Meta Quest that adds real-time
stereo-to-surround upmixing and head-tracked binaural rendering.

**Target:** Beat Saber `1.40.8_7379`, Scotland2, ARM64. Current version: `0.2.0`.
Use stereo headphones to hear the intended effect.

## Features

- Expands ordinary stereo songs into a broad, stage-anchored sound field.
- Rotates the sound field with headset orientation: a right-panned sound moves
  ahead of the listener after a 90-degree right head turn.
- Estimates shared center content and adds decorrelated rear ambience.
- Renders five virtual components through measured HRTFs at 44.1 or 48 kHz.
- Processes the existing gameplay music source in place, retaining its playback
  clock, clip, mixer routing, and pause/seek/speed controls.
- Uses fixed storage and lock-free atomics in the audio callback, without Unity
  calls, file I/O, logging, heap allocation, or mutexes in that callback.

Native multichannel input and Dolby Atmos decoding/rendering are **not
implemented** in this version. The upmixer uses the stereo mix directly; it
does not isolate individual instruments or recover an original surround mix.

## Build

Install PowerShell 7, CMake 3.22+, Ninja, QPM, and Android NDK r27. Make `cmake`,
`ninja`, and `qpm` available on PATH. Dependencies and the Android toolchain
are downloaded separately; they are not included in this repository.

From the repository root:

```powershell
qpm restore
qpm ndk resolve -d
pwsh ./scripts/build.ps1 -Package
```

If supplying an existing NDK instead, set `ANDROID_NDK_HOME` or create a local
`ndkpath.txt`. This file is ignored by Git. A local NDK configuration must
exist before the build script runs.

The build produces `dist/QuestSpatialAudio-0.2.0.qmod`. Install it through a
Scotland2-compatible mod manager on the matching game version.

Runtime dependencies: beatsaber-hook >=6.4.2, custom-types >=0.18.4, and
paper2_scotland2 >=4.8.0. The dependency manifest and resolved versions are
tracked as `qpm.json` and `qpm.shared.json`; dependency code and binaries are
excluded.

## Settings

The mod creates its settings file when a gameplay music controller starts:

`/sdcard/ModData/com.beatgames.beatsaber/Configs/quest-spatial-audio.json`

```json
{
  "enabled": true,
  "surroundAmount": 0.3,
  "outputGain": 0.5
}
```

Settings load at the start of a level. Rear ambience is clamped to 0-0.6 and
gain to 0-1. Setting `surroundAmount` to zero preserves the broad direct image
and head tracking. Setting `enabled` to false bypasses the effect on the next
level. There is no in-game settings panel yet.

## Tests

For local Windows DSP tests, install Zig on PATH or pass its executable path:

```powershell
pwsh ./scripts/test-local.ps1
# Alternatively: pwsh ./scripts/test-local.ps1 -Zig <path-to-zig.exe>
```

With a compatible host C++ compiler, the same tests can also be built in a fresh
directory using CMake with `-DQSA_TEST_ONLY=ON`.

To build and run the ARM64 tests on an ADB-connected headset:

```powershell
pwsh ./scripts/test.ps1
```

The tests cover stereo reconstruction, center steering, rear decorrelation,
head-rotation direction, ear balance at 90 degrees, bypass, reset, bounded
output, and rendering throughput. Local tests do not establish in-game sound
quality, synchronization, or Quest performance.

## Development deployment

```powershell
pwsh ./scripts/deploy.ps1 -Launch
```

This script requires the exact target game version and the resolved dependency
versions used during development. It backs up a previous copy of this mod,
stops Beat Saber, and copies only this mod's library. It does not register the
mod in a mod manager. Use a QMOD installation for a managed installation.

## Implementation and limitations

The upmixer uses running stereo power/covariance for center extraction and
different allpass chains for rear ambience. Center is subtracted from each
lateral channel so the direct components can reconstruct the original stereo
samples. The virtual components sit at 0, +/-90, and +/-135 degrees in play
space and are transformed into listener space together.

The renderer selects from 368 measured right-hemisphere HRIR pairs, mirrored
for the left hemisphere, and smooths coefficient changes over approximately
8 ms. It does not reproduce Apple's Spatialize Stereo processing. Generic
HRTFs and broadband steering can color music or redistribute complex mixes
imperfectly. Fast turns may produce timbral changes.

The direct path adds no extra audio block queue, but the FIR response is not
zero latency; added rear ambience has its own short delay tails. Translation
does not change pitch or loudness. Output is limited to +/-1 as a final guard,
and unsupported sample rates/channel counts bypass unchanged. Menu music,
saber sounds, and already-spatialized or shared music-source objects are
outside the processing scope. Compatibility with other mods, rotating maps,
and sustained gameplay performance require further testing.

## HRTF data attribution

The checked-in table derives from measurements by **Bill Gardner and Keith
Martin, MIT Media Laboratory, 1994**. See [the data notice](assets/hrtf/NOTICE.md)
for the citation, upstream source, and usage terms. This attribution applies
to the derived table as well as binaries containing it.

The original archive is not needed to build. To regenerate the table, download
the upstream `diffuse.zip` linked in the notice and run:

```powershell
python ./scripts/generate_hrtf.py --archive <path-to-diffuse.zip>
```

The generator verifies the upstream archive's SHA-256 before reading it.
