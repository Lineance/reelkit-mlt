# Roadmap

The goal is a reusable Linux C++ library over MLT++, not a full video application. Milestones are separate implementation commits. Durations are rough single-engineer estimates.

## Phase 0: Runtime proof — passed

Ubuntu 24.04/MLT 7.22 builds with CMake and links `libmlt++-7` directly. A synthetic timeline renders H.264, saves MLT XML, reloads it through MLT, and produces identical decoded frame hashes.

## Phase 1: Native-compatible API foundation — implemented

Runtime, Profile, FrameRange, Track, Timeline, RenderSpec, MltDocument, and explicit `native()` handles are in separate headers/source modules. The native interop CTest proves raw producer/filter calls and high-level methods mutate the same graph.

## Phase 2: RenderJob and XML snapshot lifecycle — implemented

`RenderJob` wraps the MLT consumer with start/progress/cancel/wait. Timeline rendering snapshots the live native graph through MLT XML and constructs a fresh graph per consumer. Generic MLT XML load/save and an installed downstream CMake consumer test pass.

Gate passed: direct and reloaded renders match for video; package consumer finds, links, and runs `reelkit::mlt`.

## Phase 3: Common convenience API — current

Extend high-level operations for clip trims/speed, transition presets, qtext keyframes/geometry, audio routing/mix, and typed render profiles. All helpers must mutate native MLT services and retain the native escape hatch. Do not implement duplicate effects already available as MLT services.

Gate: CTest verifies behavior, serialization, rendered frame/audio metrics, and service availability. Gate PCM16 WAV support on correct levels; retain AAC and float WAV support meanwhile.

## Phase 4: Linux package hardening

Test install/export CMake config on clean Ubuntu 24.04. Pin supported MLT 7 version(s), document module paths, expose service capability discovery, inventory bundled module licenses, and add tests for absent modules and invalid media.

## Phase 5: Optional adapters

If a host application needs them, add separate Project JSON, OTIO, or HyperFrames adapters. They call reelkit-mlt APIs and remain outside the core library dependency surface.

## Rough Estimate

The current implementation establishes the API skeleton and native interop test. Completing a stable v1 library (ownership/lifecycle, profile/time helpers, common timeline/effect helpers, rendering, generic XML persistence, CMake install validation, audio format checks) is estimated at **4–7 additional engineering weeks** for one C++/media engineer. Project JSON or application features are separate scope.

## v1 Acceptance

- Host code can mix high-level reelkit-mlt calls with raw MLT++ calls and observe changes on the same object graph.
- High-level wrappers preserve clear MLT object ownership and runtime/factory lifetime.
- Native and high-level producers, tracks, filters, transitions, and keyframes serialize to/reload from generic MLT XML.
- Render presets produce verified MP4/AAC and float-WAV outputs; unsupported/known-bad PCM16 WAV paths are reported.
- Each render operates on an independent MLT graph snapshot.
- Linux CMake package installs and can be consumed by a separate CMake project.
