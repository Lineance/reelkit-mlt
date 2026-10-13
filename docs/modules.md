# Module Design

The module boundaries below describe responsibilities, not final filenames. Keep the Project model independent of MLT classes and UI framework types.

## Proposed layout

```text
editor-core/
  CMakeLists.txt
  include/editor_core/
    project/
    timeline/
    jobs/
  src/
    app/                 # local worker entry point and protocol
    project/             # package IO, schema validation, migration
    assets/              # resolve, hash, package, relink
    timeline/            # normalized IR and validation
    media/               # probe, analysis, proxies, FFmpeg utilities
    mlt/                 # MLT++ graph builder, renderer, XML serialization
    otio/                # optional interchange serializer
    scenes/              # HyperFrames process adapter
    jobs/                # progress, cancellation, diagnostics
    packaging/           # profile/module lookup and resource paths
  tests/
    fixtures/
      projects/
      mlt/
      otio/
      media/
```

## Modules

### `project`

- Read/write the versioned Project package and validate schema before work begins.
- Own domain structs: `Project`, `DatasetRef`, `TemplateRef`, `Asset`, `Track`, `Clip`, `TextLayer`, `SubtitleCue`, `Transition`, `Effect`, `RenderProfile`.
- Preserve stable IDs, provenance, schema migrations, and data/template snapshots.
- Must not include `Mlt::` classes, FFmpeg structs, or Shotcut property names in public domain types.

### `assets`

- Resolve `AssetId` to a local file URI and verify existence/hash/metadata.
- Package or relink media/font/generated assets into a portable project export.
- Enforce project-root and allowed-directory rules; do not fetch arbitrary remote resources.

### `timeline`

- Convert Project semantics to a validated normalized IR using integer frames/rational rates.
- Apply ordering, source trims, layer/z-order, overlap, transition, and duration rules once.
- Produce capability diagnostics before invoking any renderer.
- Provide the common input to MLT compilation and optional OTIO export; do not own persistence.

### `mlt`

- Initialize `Mlt::Factory` with explicit module/resource paths.
- Map normalized clips to MLT producers and playlist entries; compose parallel tracks with multitrack/tractor/field and explicit transitions.
- Map supported transform, volume, fade, subtitle, and title properties using pinned MLT/Shotcut fixtures.
- Connect the final graph to an `avformat` consumer for MP4; configure codec/profile properties through typed render settings.
- Serialize a Shotcut-compatible MLT XML graph and associate the package resource paths.
- Translate MLT callbacks/errors into structured job events.

### `media`

- Probe stream duration, frame rate, dimensions, audio layout, and codec information.
- Offer optional scene/audio analysis, proxies, thumbnails, and direct FFmpeg utility operations.
- Do not build a second authoritative timeline renderer. Analysis can propose edits but cannot silently mutate a saved Project.

### `scenes`

- Invoke `@hyperframes/producer` with a pinned version, trusted component path, validated data snapshot, and deterministic render settings.
- Return a generated asset descriptor including content hash, fps, dimensions, duration, alpha capability, and source provenance.
- Handle job progress/cancel/error without implementing browser capture or HTML animation.

### `otio`

- Export the supported subset of normalized timeline items to OTIO schema objects.
- Preserve stable IDs and explicitly namespaced metadata where appropriate.
- Report unsupported MLT-only services and any flattening or omission.
- Remain optional; no OTIO library is required by the render path.

### `jobs` and `app`

- Expose structured commands such as `validateProject`, `render`, `exportMlt`, `exportOtio`, `probeMedia`, and `renderScene`.
- Return job IDs, progress events, cancellation state, artifact paths, and diagnostics.
- Prefer a dedicated C++ worker executable linked to MLT++ over loading MLT modules into a UI process. Use a versioned local IPC protocol (initially JSON lines over stdio is sufficient) so engine crashes and plugin state do not corrupt the UI.

### `packaging`

- Pin MLT, FFmpeg, Qt/Pango, and module versions for builds.
- Select an explicit runtime module allowlist; set MLT module/profile/preset paths deterministically.
- Create the Windows distribution with required DLL dependencies and verify clean-machine startup without relying on developer PATH.
- Produce license inventory for MLT and every bundled module/dependency; exclude optional modules unless needed.

## Dependency direction

```text
app/jobs -> project -> timeline
app/jobs -> assets
app/jobs -> media
app/jobs -> scenes
app/jobs -> mlt -> normalized timeline + assets
app/jobs -> otio -> normalized timeline + assets
packaging -> mlt runtime deployment
```

`project`, `timeline`, and `assets` are independent of MLT. `media` can use FFmpeg directly, but its analysis results are data supplied to Project operations rather than hidden renderer state.

## Worker boundary

The worker owns MLT initialization, render consumer lifetime, and all module paths. The UI talks in Project IDs and structured commands, not MLT XML or shell command strings. The worker may use internal threads for MLT rendering; only one layer owns cancellation and job state. Direct `melt` invocation is reserved for diagnostics and test comparisons.

## Public compatibility surface

- Stable project format and JSON command protocol are versioned independently from MLT and OTIO versions.
- A `CapabilityReport` lists supported, degraded, and unsupported timeline features for a requested output.
- `RenderResult` lists artifact IDs, duration/profile, hashes, logs, warnings, and source Project revision.
- No public API exposes mutable MLT service property bags as the general authoring model.
