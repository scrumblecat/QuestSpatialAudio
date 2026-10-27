# Quest Spatial Audio

A Beat Saber mod that adds head-tracked spatial audio to your music on Quest.

## What is this?

Quest Spatial Audio makes stereo songs sound like they're playing around you.
The sound stays anchored to the stage as you turn your head. For example, sound
on your right moves in front of you when you turn 90° to the right.

It works with your existing songs, without stems or extra setup for each map.
It adds a wider sound field and some rear ambience while leaving menu music
and saber sounds alone. Use stereo headphones to hear the effect.

This is an early prototype for **standalone Quest Beat Saber 1.40.8_7379 with
Scotland2**, with development focused on Quest 2. Version 0.2.0 still needs
in-game listening and performance testing.

## How do I install it?

1. Use the matching Beat Saber version with Scotland2 installed.
2. Install `QuestSpatialAudio-0.2.0.qmod` through your Scotland2-compatible mod manager.
3. Start a song and turn your head to hear the effect.

If you're building it yourself, the QMOD is created in `dist/`.

The mod requires beatsaber-hook 6.4.2+, custom-types 0.18.4+, and
paper2_scotland2 4.8.0+. The QMOD includes download links for these dependencies.
Game files are not included.

## Can I change the settings?

There's no in-game settings menu yet. After starting a level, edit:

```text
/sdcard/ModData/com.beatgames.beatsaber/Configs/quest-spatial-audio.json
```

The defaults are:

```json
{
  "enabled": true,
  "surroundAmount": 0.3,
  "outputGain": 0.5
}
```

- `enabled`: set to `false` to turn the effect off and compare it with normal playback.
- `surroundAmount`: rear ambience strength, from `0` to `0.6`. Setting it to `0` keeps head tracking and the wider stereo image.
- `outputGain`: output level, from `0` to `1`. The default leaves room for the added audio without clipping.

Changes apply when you start the next level.

## How do I build it?

You'll need PowerShell 7, CMake 3.22+, Ninja, QPM, and Android NDK r27.
Run these commands from the project folder:

```powershell
qpm restore
qpm ndk resolve -d
pwsh ./scripts/build.ps1 -Package
```

The finished package is `dist/QuestSpatialAudio-0.2.0.qmod`. The build script
also supports the portable CMake and QPM tools under `.tools/`.

For development, connect your headset with ADB and run this after building:

```powershell
pwsh ./scripts/deploy.ps1 -Launch
```

The deploy script checks the game and dependency versions, backs up any existing
copy of this mod, and copies the new library. This installs it directly without
registering it in your mod manager. To remove that installation, stop Beat Saber
and delete only:

```text
/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/libquest-spatial-audio.so
```

## How do I test it?

Run the audio tests locally on Windows with Zig installed, available on PATH
or under `.tools/zig/`:

```powershell
pwsh ./scripts/test-local.ps1
```

You can also pass `-Zig` with the path to its executable. Local tests don't
connect to your headset.

To build and run the audio tests on a connected headset instead, use:

```powershell
pwsh ./scripts/test.ps1
```

These tests check the audio processing, rotations, and reset behavior. They
don't replace testing inside Beat Saber. Try turning your head, pausing,
restarting, and seeking in practice mode, then compare with the effect disabled.

## Anything I should know?

The surround effect is generated from stereo audio; it isn't a song's original
surround mix. Its sound will vary between listeners, and quick head turns may
change the tone of the music. The filters also add a small amount of delay.

Only stereo audio at 44.1 or 48 kHz is processed. Other formats pass through
unchanged. Multiplayer, 90°/360° maps, and compatibility with other audio mods
still need testing.

## Credits

The spatial audio uses KEMAR measurements by Bill Gardner and Keith Martin,
MIT Media Laboratory (1994). See the [data attribution and usage terms](assets/hrtf/NOTICE.md).

The bundled filters are generated from those measurements. To regenerate them,
run `python scripts/generate_hrtf.py`.
