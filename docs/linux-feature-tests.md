# Linux Feature Tests

## Scope

These tests run against the Linux MLT++ runtime and use generated media only. Each feature has its own executable and CTest entry so failures can be run and diagnosed independently. The suite is an early behavior probe, not full Project/Shotcut acceptance.

## Environment and command

Verified in WSL 2 Ubuntu 24.04.4 with GCC 13.3, CMake 3.28.3, Ninja 1.11.1, FFmpeg 6.1.1, and MLT/MLT++ 7.22.0.

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Latest full run: **4/4 tests passed**.

## Independent scenarios

| CTest target | Coverage | Result |
|---|---|---|
| `test_timeline` | Two 90-frame color clips, 15-frame `luma` dissolve, expected 165-frame duration, MLT XML reopen and second render | Passed; direct and reopened video have identical decoded frame hashes |
| `test_audio` | Direct tone PCM checks, 440/660 Hz tone producers, `mix` transition, PCM16 WAV, float WAV, and AAC/M4A consumers | Producer frame and float/AAC paths preserve the requested attenuation; PCM16 WAV output remains anomalously loud |
| `test_text_keyframes` | `qtext` opacity animation from 0 to 1 over 20 frames, XML serialization and render | Passed; XML contains `opacity=0=0;20=1`, extracted frames show the fade |
| `test_alpha_overlay` | Semi-transparent qtext over a red video producer | Passed; extracted frame shows the underlying red with blended text |

Artifacts are under the ignored `build/test-artifacts/` directory. The test sources are kept separate: `test_timeline.cpp`, `test_audio.cpp`, `test_text_keyframes.cpp`, and `test_alpha_overlay.cpp`; shared C++ helpers are in `test_support.hpp`.

## Audio level follow-up

The audio test directly reads an `mlt_audio_float` frame and asserts that `level=-24` matches the expected `10^(-24/20)` peak ratio versus `level=0`; this passes. A `pcm_f32le` WAV preserves -24 dBFS (about -27 dB mean, -24 dB peak), and the AAC/M4A output preserves the average level (about -27 dB mean for a single tone). A mixed AAC/M4A render is non-silent and measures about -28.9 dB mean.

The PCM s16le WAV consumer output is anomalous: it measures near full-scale (about -5.5 dB mean) for the same -24 dB producer, even with `mlt_audio_format=f32le`. Do not rely on MLT 7.22's PCM16 WAV path until this conversion behavior is isolated. The mixed AAC/M4A decoded sample peak is higher than the source peak, so true-peak/headroom still needs a tolerance test.

Reusing the same audio Tractor sequentially for WAV and AAC consumers caused a segmentation fault in an earlier test. Constructing a fresh graph per consumer avoids the crash. Treat graph reuse across consumers as unsupported until verified.

The CTest audio case asserts producer-frame level ratio, float-WAV amplitude bounds, graph duration, and output file creation. It does not yet assert AAC true-peak, precise mix balance, fade curve, or PCM16 headroom.

## Plugin availability

The Ubuntu `libmlt7` package provides 25 MLT module libraries, including `avformat`, `core`, `qt`, `gdk`, `xml`, `frei0r`, `movit`, `glaxnimate`, `opencv`, `rubberband`, `sox`, and `vidstab`. The current tests require no additional module package.

The Frei0r adapter module is installed, but the separate Frei0r effect bundle is not. LADSPA support is present in MLT modules but no external LADSPA plugins are installed, which explains the harmless `No LADSPA plugins were found` startup warning. Ubuntu offers `frei0r-plugins`, `swh-plugins`, and `tap-plugins`; install these only when a defined test needs their effects. No optional plugin packages were installed for this test run.

## Still untested

- Source media with audio, multi-track audio routing, fades, ducking, and gain/headroom correctness.
- Real alpha video/image-sequence assets from `@hyperframes/producer`; this test only verifies qtext's transparent overlay path.
- Keyframed transform/position beyond qtext opacity, VFR input, speed changes, codecs beyond H.264/PCM, and project-relative asset relocation.
- Opening/editing/saving/reopening the output in a Linux Shotcut GUI and compatibility across pinned MLT/Shotcut versions.
- Native Linux package installation outside this Ubuntu/WSL development environment, GPU/OpenGL behavior, and hardware audio.
