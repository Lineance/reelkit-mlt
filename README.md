# reelkit-mlt

`reelkit-mlt` is a Linux-first C++20 convenience library over MLT++. It keeps MLT services and object graphs native while adding safer ownership, frame-range helpers, timeline construction, render presets, and an explicit `native()` escape hatch.

It is a library, not a full editor application. It does not require Project JSON, a UI, a workflow engine, Shotcut, or OTIO.

## Design

- High-level wrappers and direct MLT++ calls mutate the same underlying object graph.
- Public `FrameRange` values are half-open; conversion to MLT's inclusive out-frame happens at the API boundary.
- Generic MLT XML load/save is available for persistence and graph snapshots.
- FFmpeg remains in the stack through MLT's `avformat` module.
- Project JSON can be implemented later as a separate adapter that calls this library.

## Current API

- `Runtime`: process-scoped MLT factory and service discovery.
- `Profile`: frame size/rate/aspect and MLT profile access.
- `Timeline` and `Track`: wrappers over a MLT `Tractor` and `Playlist`.
- `FrameRange`: typed source in/out values with half-open semantics.
- `RenderSpec`: MP4/AAC, M4A/AAC, and float-WAV presets plus raw consumer properties.
- `RenderJob`: asynchronous start/progress/cancel/wait over an isolated graph snapshot.
- `MltDocument`: load and operate on generic MLT XML graphs.
- `native()`: returns a reference to the same `Mlt::` object owned by the wrapper.

See [API guide](docs/api.md), [architecture](docs/architecture.md), [modules](docs/modules.md), and [Linux feature tests](docs/linux-feature-tests.md).

## Build

Ubuntu 24.04 is the current tested baseline. Required packages:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config ffmpeg libmlt-dev libmlt++-dev libmlt-data
```

Build and test:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The library installs a CMake package named `ReelKitMlt` and target `reelkit::mlt`:

```bash
cmake --install build --prefix build/stage
```

A downstream CMake consumer test is included in CTest.

## Scope

The core exposes MLT services rather than reimplementing their effects. Optional MLT modules are discovered at runtime; only install effect bundles when a defined feature needs them. PCM16 WAV level behavior is not yet supported by the high-level presets; float WAV and AAC output are tested. Shotcut/NLE GUI compatibility and application-level Project JSON are separate work.

## Current Status

The C++/MLT++ facade, native interop test, generic MLT XML round-trip, render presets, RenderJob lifecycle, and Linux CMake install target are implemented. The current CTest suite is the executable API contract.
