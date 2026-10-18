# Decisions

Status values: **Accepted** means the current implementation follows the choice; **Proposed** means it is the selected default for v1 unless testing disproves it; **Open** means it remains a later product decision.

## Accepted

### editor-core is a C++ library over MLT++

It is not a standalone NLE, Project authoring application, worker service, or Project JSON runtime. It provides a convenient API over MLT++ and lets callers use native MLT++ on the same graph.

### Composition wrappers with explicit `native()`

High-level Runtime/Profile/Track/Timeline wrappers own or share actual MLT++ objects and expose explicit native references. Convenience methods mutate those same objects. The library does not subclass every MLT++ type or maintain a parallel timeline IR.

### Native MLT XML persistence is included

The library can save/load generic MLT XML and use XML snapshots to create fresh render graphs. This is engine graph serialization, not a promise of compatibility with a specific editor GUI.

### Linux only

Initial development/runtime baseline is Ubuntu 24.04 with MLT/MLT++ 7.22.0. No Windows build or release target is in scope.

### FFmpeg remains

MLT's avformat module uses FFmpeg; adopting this wrapper does not remove FFmpeg.

### Project JSON is a separate upper-layer adapter

A future application may compile Project JSON into editor-core calls and persist its own data/template/Asset ID metadata. Project JSON is not required by the library and must not duplicate the live MLT graph as a second source of truth.

## Proposed

### Explicit C++ native handles are the interop boundary

Use references such as `Track::native() -> Mlt::Playlist&`. Raw MLT++ calls remain possible in the same code, and changes affect the same object. Native references are valid only while the owning wrapper/runtime remains alive.

### Render each output from a fresh MLT graph snapshot

Serialize the current graph through MLT XML in memory, reload with `xml-string`, and connect one consumer to that snapshot. This avoids reusing consumer-mutated audio graph objects; tests must cover audio XML round trips before treating the behavior as release-stable.

### C++20 and CMake package

Use C++20, an exported `editor_core::editor_core` CMake target, and the system MLT++ package. Source/API compatibility is versioned; no cross-MLT binary ABI guarantee is made.

## Open for Later Adapters

- Project JSON schema and data/template/asset package.
- OTIO interchange mapping and loss reports.
- HyperFrames execution adapter; core only accepts its generated alpha media as a producer.
- Shotcut/Kdenlive GUI-specific project guarantees; generic MLT XML does not imply GUI conformance.
- Optional Frei0r/LADSPA/OpenFX plugin bundles and their licensing.

## Known Runtime Limitations

- MLT 7.22 `avformat` PCM16 WAV output does not preserve the tested producer level, while float PCM/AAC paths preserve it. High-level presets therefore prefer AAC or float WAV and report the PCM16 caveat.
- Sequentially attaching multiple consumers to the same audio graph caused a segmentation fault in a test; render from a fresh MLT XML snapshot per output.
- GPU/native audio device behavior and native Linux packaging beyond Ubuntu/WSL remain unverified.
