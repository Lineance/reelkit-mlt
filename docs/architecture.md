# Architecture

## Purpose

`editor-core` is a C++ library that makes common MLT++ operations easier while preserving access to MLT's native object graph and services. It does not define a complete nonlinear-editor application or a mandatory declarative Project format.

The core relationship is:

```text
Host C++ application
       |
       v
editor-core convenience API <----> native MLT++ calls
       |                                  |
       +------------- same graph ----------+
                          |
                          v
          libmlt++ / libmlt services
                          |
              avformat module / FFmpeg
```

The convenience facade owns or references actual MLT++ objects. `native()` returns the live object, not a converted copy. If host code adds a filter through `Mlt::Playlist::attach()`, later editor-core operations see that filter; if editor-core appends a producer, raw MLT++ code sees it in the same playlist.

## MLT++ Object Mapping

The API follows MLT's existing hierarchy:

- `Runtime` manages the process-wide factory and repository.
- `Profile` wraps `Mlt::Profile`.
- `Track` wraps `Mlt::Playlist`.
- `Timeline` wraps `Mlt::Tractor`.
- Native clips/producers, filters, transitions, and consumers remain MLT++ objects, accessible through overloads and `native()` references.
- `MltDocument` wraps a loaded MLT producer graph without attempting to reconstruct a separate high-level timeline model.

MLT++ service wrappers inherit shared property and service behavior. MLT wrappers use reference-counted service handles, so editor-core wrapper lifetime must be explicit: native references are valid only while the owning wrapper/runtime remains alive. High-level wrappers are handles to the native graph, not value copies of an independent timeline.

## API Layers

### Native facade

- Owns Runtime/Profile and provides typed access to common MLT services.
- Maps errors, invalid service creation, and frame-range validation to `editor_core::Error`.
- Exposes the underlying `Mlt::` object reference for advanced calls and third-party MLT services.
- Queries the loaded MLT repository so helper methods can report missing services.

### Convenience operations

- Appends native producers or file-backed sources to a Track.
- Uses half-open `FrameRange` values and translates them to MLT's inclusive out-frame at the boundary.
- Provides convenience for blanks, adjacent dissolves, qtext filters, audio/video profiles, MLT XML persistence, and standard render presets.
- Does not maintain a second timeline IR or silently translate unsupported effects.

## Render and XML Lifecycle

The consumer may mutate internal state while rendering. An earlier test crashed when the same audio graph was sequentially connected to WAV and AAC consumers. Therefore, `Timeline::startRender()` serializes the live graph through MLT's XML consumer into a buffer, reloads a fresh graph with the `xml-string` producer, and gives that snapshot to one RenderJob/consumer. `RenderJob` exposes start, progress, cancellation, and wait; `Timeline::render()` is the synchronous convenience wrapper. This preserves raw MLT++ mutations while isolating each render.

`Timeline::saveMltXml()` writes the graph to a file. `MltDocument::load()` reads generic MLT XML and gives callers a native producer handle. This is MLT graph serialization; it does not promise compatibility with any particular NLE GUI or private project conventions.

## Render and Media

- H.264/AAC MP4 and AAC/M4A use MLT's `avformat` consumer.
- Float PCM WAV is available as an explicit preset.
- PCM16 WAV output from the tested MLT 7.22 `avformat` path does not preserve the producer's requested level; it is warned as an unsupported high-level preset until corrected.
- FFmpeg remains a dependency through MLT's avformat modules. The core does not build a second FFmpeg timeline renderer.
- Optional effects come from MLT modules. Runtime capability queries identify available producer/filter/transition/consumer services; the library does not install every effect pack.

## Project JSON Boundary

Project JSON is not a prerequisite for using editor-core. If a host application later needs stable Asset IDs, templates, datasets, HTML scene provenance, or schema migration, add a separate `editor_project` adapter that builds an editor-core graph. It may also persist application metadata alongside MLT XML. It must not silently claim to serialize arbitrary raw MLT services in a separate shadow timeline model.

Other optional adapters, such as OTIO or HyperFrames orchestration, also belong above or beside the core library:

- OTIO maps only the subset its interchange schema can represent.
- HyperFrames generates alpha media; editor-core accepts that media as a native MLT producer.

## Linux Runtime

The supported development baseline is Ubuntu 24.04 with the tested MLT/MLT++ 7.22.0 packages. MLT services are dynamically loaded modules, so applications must make module, profile, preset, and dependent shared-library paths available. The library links to the system MLT++ package and does not bundle optional Frei0r/LADSPA/OpenFX packs by default.

## External References

- [MLT framework design](https://www.mltframework.org/docs/framework/)
- [MLT++ C++ wrapper](https://www.mltframework.org/docs/mlt++/)
- [MLT XML serialization](https://www.mltframework.org/docs/mltxml/)
- [MLT installation and modules](https://www.mltframework.org/docs/install/)
