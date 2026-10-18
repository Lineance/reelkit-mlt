# Module Design

The implementation is organized as a C++ library with separate public headers and source modules. The high-level layer wraps live MLT++ objects; there is no mirrored Project timeline model in the core.

## Public Headers

```text
include/editor_core/
  editor_core.hpp   # umbrella include
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

- Initialize `Mlt::Factory` once per process and retain the MLT repository while any editor-core objects exist.
- Share one Runtime state across copied Runtime handles; reject conflicting module directories in one process.
- Expose producer/filter/transition/consumer service discovery and `nativeRepository()` for advanced MLT++ use.
- Ensure all editor-core MLT objects retain Runtime state for their lifetime.

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
- `Timeline::render()` snapshots the live graph to MLT XML in memory, reloads a fresh `xml-string` producer, then renders with a new `avformat` consumer.
- `Timeline::saveMltXml()` writes the graph; `MltDocument::load()` loads generic MLT XML into a native producer wrapper.
- `RenderResult` reports target, frame count, rational frame rate, duration, and warnings.

## Dependency Direction

```text
editor-core native facade -> libmlt++ -> libmlt modules -> FFmpeg/other module deps
host application -> editor_core::editor_core
optional editor_project adapter -> editor_core API
optional OTIO/HyperFrames adapters -> host application + editor-core as needed
```

The library does not depend on Project JSON, a UI framework, a workflow engine, Shotcut, OTIO, or HyperFrames.

## Native Interoperability Contract

- All `native()` methods return references to the exact MLT++ objects owned by editor-core wrappers.
- No copy/translation occurs when crossing from high-level helpers to raw MLT++.
- Host code must not retain a native reference beyond the lifetime of its owning Runtime/Profile/Track/Timeline handle.
- Raw MLT services/properties remain available even if the convenience API does not model their parameters.
- Each render uses a fresh graph snapshot to avoid reusing consumer-mutated MLT audio graph state.

## Build and Installation

- CMake target: `editor_core::editor_core`.
- Build against `pkg-config` module `mlt++-7`.
- Install headers, static library, exported CMake targets, and `EditorCoreConfig.cmake`.
- No stable binary ABI is promised across MLT versions; consumers build against the declared MLT 7 runtime.
- Do not bundle optional plugin packs unless a named API feature requires them and the license/dependency inventory is complete.
