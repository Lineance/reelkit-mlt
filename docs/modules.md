# Module Design

The implementation is organized as a C++ library with separate public headers and source modules. The high-level layer wraps live MLT++ objects; there is no mirrored Project timeline model in the core.

## Public Headers

```text
include/reelkit_mlt/
  reelkit_mlt.hpp   # umbrella include
  types.hpp         # errors, profiles, rational rates, frame ranges, track kinds
  runtime.hpp       # MLT factory lifetime and service discovery
  profile.hpp       # Profile wrapper and native() access
  track.hpp         # Playlist wrapper and clip/filter helpers
  timeline.hpp      # Tractor wrapper and multi-track helpers
  render.hpp        # render specs/results and MLT XML document wrapper
```

## Source Modules

```text
src/
  types.cpp
  runtime.cpp
  profile.cpp
  track.cpp
  timeline.cpp
  render.cpp
  mlt_document.cpp
  detail/state.hpp  # shared internal state definitions; not public API
```

## Core Types

### Runtime

- Initialize `Mlt::Factory` once per process and retain the MLT repository while any reelkit-mlt objects exist.
- Share one Runtime state across copied Runtime handles; reject conflicting module directories in one process.
- Expose producer/filter/transition/consumer service discovery and `nativeRepository()` for advanced MLT++ use.
- Ensure all reelkit-mlt MLT objects retain Runtime state for their lifetime.

### Profile

- Store width, height, rational frame rate, sample/display aspect, progressive flag, and optional colorspace.
- Own one `Mlt::Profile`; expose `native()` and profile settings.
- Timeline, Track, and MLT documents retain the shared profile state.

### Track

- Own/share a `Mlt::Playlist` and identify its index/kind/name within a Timeline.
- `append(Mlt::Producer&, FrameRange)` directly appends a native producer.
- `appendFile()` creates a file-backed MLT producer and appends it using the same path.
- `blank()` accepts a frame count and translates to MLT's inclusive duration convention.
- `dissolve()` is a convenience over `Playlist::mix`; `mixAdjacent()` accepts a native MLT Transition.
- `attach(Mlt::Filter&)` and `native()` expose the same playlist service.
- `addText()` creates a qtext MLT filter and returns the native filter handle for further animation/property calls.

### Timeline

- Own a `Mlt::Tractor` plus shared Track handles.
- `addTrack()` uses current native track count so raw `Tractor::set_track()` and high-level additions do not overwrite one another.
- `setNativeTrack()` and `plantTransition()` support direct MLT++ calls.
- `durationFrames()` refreshes and queries the underlying tractor.
- `native()` returns the same live `Mlt::Tractor`.

### Render and MLT XML

- `RenderSpec` has MP4/H.264/AAC, M4A/AAC, and float-WAV presets plus a string property map for consumer options.
- `RenderJob` wraps the MLT consumer lifecycle with `start`, `progress`, `cancel`, and `wait`.
- `Timeline::render()` is the synchronous convenience call; it creates and waits on a RenderJob.
- `Timeline::startRender()` returns a caller-managed RenderJob.
- `Timeline::saveMltXml()` writes the graph; `MltDocument::load()` loads generic MLT XML into a native producer wrapper.
- `RenderResult` reports target, frame count, rational frame rate, duration, cancellation, and warnings.

## Dependency Direction

```text
reelkit-mlt native facade -> libmlt++ -> libmlt modules -> FFmpeg/other module deps
host application -> reelkit::mlt
optional editor_project adapter -> reelkit_mlt API
optional OTIO/HyperFrames adapters -> host application + reelkit-mlt as needed
```

The library does not depend on Project JSON, a UI framework, a workflow engine, Shotcut, OTIO, or HyperFrames.

## Native Interoperability Contract

- All `native()` methods return references to the exact MLT++ objects owned by reelkit-mlt wrappers.
- No copy/translation occurs when crossing from high-level helpers to raw MLT++.
- Host code must not retain a native reference beyond the lifetime of its owning Runtime/Profile/Track/Timeline handle.
- Raw MLT services/properties remain available even if the convenience API does not model their parameters.
`Timeline::startRender()` snapshots the live graph through MLT XML into a fresh producer before constructing a RenderJob. The job owns that snapshot and one consumer; progress and cancellation operate on the consumer's lifecycle. This avoids reusing consumer-mutated graph state. Native C++ calls made before the snapshot are preserved in the XML graph.

## Build and Installation

- CMake target: `reelkit::mlt`.
- Build against `pkg-config` module `mlt++-7`.
- Install headers, static library, exported CMake targets, and `ReelKitMltConfig.cmake`.
- No stable binary ABI is promised across MLT versions; consumers build against the declared MLT 7 runtime.
- Do not bundle optional plugin packs unless a named API feature requires them and the license/dependency inventory is complete.
