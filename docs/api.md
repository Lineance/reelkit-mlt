# API Guide

This is an in-process C++20 library over MLT++. Create one `Runtime`, build high-level wrappers, and use `native()` whenever a feature needs raw MLT++.

## Build and install

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix build/stage
```

Downstream CMake:

```cmake
find_package(ReelKitMlt CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE reelkit::mlt)
```

## Mixed high-level and native calls

```cpp
#include <reelkit_mlt/reelkit_mlt.hpp>

int main()
{
    reelkit::mlt::Runtime runtime;
    reelkit::mlt::ProfileSpec settings;
    settings.width = 640;
    settings.height = 360;
    settings.frameRate = {30, 1};
    reelkit::mlt::Profile profile(runtime, settings);

    reelkit::mlt::Timeline timeline(profile);
    auto video = timeline.addTrack(reelkit::mlt::TrackKind::Video, "main");

    // Native MLT++ producer passed directly to the convenience API.
    Mlt::Producer red(profile.native(), "color:red");
    red.set("out", 89);
    video.append(red, reelkit::mlt::FrameRange(0, 90));

    // Native MLT++ mutation of the exact Playlist wrapped by Track.
    Mlt::Filter title(profile.native(), "qtext", "");
    title.set("argument", "Mixed API calls");
    title.set("bgcolour", "0x00000000");
    video.native().attach(title);

    // Or use the convenience helper; it also mutates that same Playlist.
    auto helperTitle = video.addText(reelkit::mlt::TextSpec{
        .text = "reelkit-mlt helper",
        .size = 30,
        .opacity = 0.8,
    });

    auto output = reelkit::mlt::RenderSpec::mp4("build/output/mixed.mp4");
    output.enableAudio = false;
    const auto result = timeline.render(output);
    timeline.saveMltXml("build/output/mixed.mlt");
    return result.frames == 90 ? 0 : 1;
}
```

`FrameRange(start, end)` is half-open: start is included and end is excluded. reelkit-mlt converts it to MLT's inclusive out frame. `Track::native()` and `Timeline::native()` return references to the owned live MLT++ objects. Those references must not outlive the owning wrapper and its Runtime.

`Timeline::startRender()` returns a managed asynchronous job:

```cpp
auto job = timeline.startRender(reelkit::mlt::RenderSpec::mp4("output.mp4"));
job.start();
while (job.isRunning()) {
    const auto p = job.progress();
    // p.completedFrames / p.totalFrames
}
auto result = job.wait();
```

`Timeline::render()` is the synchronous convenience call and internally starts/waits for a RenderJob. `cancel()` stops the consumer; a cancelled result may refer to a partial output.

## Render Jobs

`Timeline::startRender()` returns a move-only RenderJob over a fresh XML snapshot. Call `start()`, poll `progress()`, and call `cancel()` if needed; `wait()` returns the final `RenderResult`. `Timeline::render()` is the synchronous convenience call and internally starts/waits for a job. A cancelled result is marked and may point to a partial file.

## Loading Native MLT XML

```cpp
auto document = reelkit::mlt::MltDocument::load(profile, "input.mlt");
const auto output = document.render(reelkit::mlt::RenderSpec::mp4("output.mp4"));
```

`MltDocument` keeps the graph native and does not invent a Project schema for arbitrary MLT services. XML snapshots are used for isolated render consumers.

## Profiles and Services

`Runtime::hasProducer`, `hasFilter`, `hasTransition`, and `hasConsumer` query the loaded MLT repository. Missing plugins are reported before graph construction. `Runtime::nativeRepository()` gives advanced callers direct access to the MLT++ repository.

The runtime is process-scoped. Keep `Runtime`, `Profile`, `Track`, and `Timeline` owners alive while using their native MLT references. A render job uses a fresh graph snapshot rather than reusing a consumer-mutated audio graph.

## Current Output Presets

- `RenderSpec::mp4(path)`: H.264/yuv420p and AAC.
- `RenderSpec::m4a(path)`: AAC audio-only.
- `RenderSpec::floatWav(path)`: float PCM audio-only.

The MLT 7.22 PCM16 WAV output path currently has a level conversion anomaly. Use float WAV or AAC for validated output; the native consumer property map remains available for experiments.
