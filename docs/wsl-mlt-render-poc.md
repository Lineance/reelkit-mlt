# WSL MLT Render PoC

## Scope

This proof of concept verifies that a C++ application can link MLT++ directly, construct a small edit graph, render with MLT's `avformat` consumer, serialize that graph to MLT XML, reload the XML through MLT, and render the reloaded graph again.

It does not process user media and is not the Project compiler or production renderer.

## Environment

- WSL 2, Ubuntu 24.04.4 LTS
- GCC 13.3.0
- CMake 3.28.3 and Ninja 1.11.1
- MLT/MLT++ 7.22.0
- FFmpeg 6.1.1

The generic `bash` tool in this session still resolves to MSYS/Git Bash. Run Linux commands through the explicit WSL shell.

## Build and run

From the repository root in Ubuntu WSL:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/mlt_render_poc build/output/render.mp4 build/output/render.mlt
```

The executable uses MLT++ directly; the `melt` CLI is not required.

## Timeline exercised

- 640x360 progressive video at 30/1 fps.
- A 90-frame red producer followed by a 90-frame blue producer.
- A 15-frame MLT `luma` dissolve; the resulting timeline is 165 frames.
- A `qtext` filter on the playlist with white text over the video.
- An MLT XML consumer to write the graph.
- An `avformat` consumer configured for H.264/yuv420p output, with audio disabled for this video-only test.
- A second `avformat` consumer rendering the `.mlt` file after reopening it through MLT.

## Results

The verified output is H.264, 640x360, 30/1 fps, 165 frames, and 5.5 seconds. The MP4 contains no audio stream. The title and the red-to-blue dissolve are visible in extracted start, midpoint, and end frames.

The decoded-frame MD5 output from the direct graph and the reloaded MLT XML is identical for all 165 frames. The reloaded graph renders successfully with the same dimensions, frame rate, duration, title, and transition.

The generated artifacts are under the ignored `build/output/` directory.

## Non-fatal environment warnings

- `No LADSPA plugins were found`: the optional LADSPA module has no external LADSPA plugin directory configured; this test does not use LADSPA.
- Qt reports `/mnt/wslg/runtime-dir` permissions as `0777` instead of `0700`. The qtext render completes; no system permission was changed.

No FFmpeg timestamp warnings remained after disabling audio for this video-only output.

## Not yet validated

- Opening, editing, saving, and reopening the generated project in the Shotcut GUI. This WSL test only reopens it through MLT itself.
- Audio tracks, audio mixing, source media with audio, and audio timestamp behavior.
- User-provided Project packages, Asset ID resolution, project-relative packaging, fonts beyond the installed Ubuntu set, and path relocation.
- Windows-native MLT module/DLL packaging, GPU paths, and compatibility with a pinned Shotcut release.
- The MLT version shipped by the eventual application. This result is specific to the Ubuntu 24.04 package version 7.22.0.
