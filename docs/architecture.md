# Architecture

## Goal

`editor-core` turns a deterministic local Project package into a rendered video and editable MLT-family projects. It is a new C++/MLT-centered engine, not a Nim port of Auto-Editor.

The system has three distinct representations:

1. **Project package**: authoritative product state, including data, templates, source scene files, stable asset identities, and user intent.
2. **Compiled timeline graph**: validated, frame-accurate execution plan built from Project semantics. The C++ compiler maps this into MLT services.
3. **Artifacts**: MP4, Shotcut `.mlt`, optional OTIO, reports, and caches. They are generated outputs; editing an exported NLE project does not silently rewrite the Project source.

## System flow

```text
Data connectors + local media + template inputs
                    |
                    v
        Project package (.vproj)
                    |
       validate / resolve / compile
                    |
          normalized Timeline IR
                    |
              MLT++ graph
      _____________|______________
     |             |              |
     v             v              v
MLT consumer   MLT XML       OTIO adapter
 + avformat    consumer       (optional)
     |             |              |
     v             v              v
    MP4       Shotcut .mlt    interchange .otio
```

HTML scenes follow a separate rendering path:

```text
Project template + validated variables
                  |
          @hyperframes/producer
                  |
     alpha video or image sequence
                  |
        Project generated Asset
                  |
        MLT timeline as a clip
```

The normalized Timeline IR is an implementation boundary, not another persisted source of truth. It makes validation, diagnostics, MLT compilation, and OTIO mapping testable without making the `.mlt` file the Project database.

## Project package

Recommended package layout:

```text
<name>.vproj/
  project.json
  data/source/
  data/normalized/
  templates/
  assets/media/
  assets/generated/
  assets/subtitles/
  renders/
  exports/shotcut/
  exports/otio/
  jobs/
```

`project.json` owns the stable Project ID, schema version, settings, asset manifest, timeline, data/template references, rendering settings, and provenance. Asset references use stable IDs in the Project and project-relative URIs in packaged exports. The packager can copy or hard-link authorized media and fonts; the `.mlt` file itself should be treated as path-based and must be accompanied by its resources.

Persist timing as integer frames and rational frame rates, with one documented interval convention (`[start, end)`). Convert to MLT inclusive out-points only at the backend boundary. Source time and timeline placement remain separate values.

The Project timeline owns semantic objects such as video/audio clips, text layers, subtitles, image/video overlays, transitions, keyframes, and template bindings. MLT properties are compiler output, not the public Project schema. This keeps MLT service names and Shotcut-specific property layouts from leaking into the product contract.

## MLT runtime and rendering

Use MLT++ directly from the C++ backend. The compiler creates producers and playlists, connects tracks through a multitrack/tractor graph, attaches filters and transitions, and connects the final producer to an output consumer.

- MP4 output uses the MLT `avformat` consumer and a pinned render profile. MLT's avformat module depends on FFmpeg; retaining FFmpeg is expected.
- Shotcut output serializes a compatible MLT graph to `.mlt`. Use Shotcut-authored golden projects for title filters, project properties, and version-specific structure.
- MLT dynamic service modules must be discoverable at runtime. The Linux package must include the selected `.so` modules, profiles, presets, dependent shared libraries, and required support files, and initialize MLT with deterministic paths.
- Render jobs need progress, cancellation, structured diagnostics, and a deterministic profile/resource configuration. These are product-layer services around the MLT consumer.

Do not make the production pipeline depend on invoking `melt` as an opaque subprocess. `melt` remains useful for smoke tests and developer diagnosis. The application backend should call MLT++ directly; it may run in a dedicated worker process to isolate plugin loading and render failures from the UI.

## FFmpeg relationship

MLT is the timeline and render orchestration layer; FFmpeg remains its media foundation through the avformat modules. Keep media probing, analysis, proxy generation, stream inspection, and specialized remux/transcode utilities as explicit services. Avoid maintaining two independent full-timeline render graphs unless a measured requirement justifies it.

Before promising parity with Auto-Editor, verify stream-copy/partial-reencode behavior, timestamp handling, variable-frame-rate inputs, codec options, audio resampling, and cancellation. MLT availability does not prove that every existing FFmpeg optimization maps one-to-one.

## Text, subtitles, and generated scenes

- Basic and animated text should use MLT/Shotcut-native services where the target Shotcut version exposes editable properties. Validate CJK font behavior, keyframes, position/opacity animation, and font fallback using golden `.mlt` fixtures.
- Subtitle cues remain semantic Project data and may be exported as separate subtitle assets or mapped to tested native services. Burn-in remains an explicit render choice.
- Complex HTML/CSS/GSAP remains with `@hyperframes/producer`. Store its source component, input snapshot, renderer version, output hash, frame rate, dimensions, and duration in Project provenance. MLT consumes the resulting alpha media; it does not attempt to preserve arbitrary browser source as an NLE-native effect.
- Unsupported mappings must be reported before export. Never silently omit a Project effect.

## OTIO relationship

OTIO is an optional interchange target, not a mandatory middle layer between Project and MLT. Map supported clips, tracks, gaps, transitions, media references, markers, and linear time-warp from the same normalized timeline used to compile MLT. Attach namespaced metadata only for explicitly documented private fields.

OTIO does not guarantee that an external editor will understand MLT-specific filters, Qt/Pango titles, or arbitrary template metadata. The OTIO exporter must produce a capability/loss report. It must not claim full-fidelity round-trip unless tested against a named application and version.

## Compatibility and ownership

- Project is the only upstream source of truth.
- A Shotcut edit of an exported `.mlt` is a delivery copy. Reverse synchronization is out of scope for the first release.
- Target MLT and Shotcut versions are pinned for generation and acceptance fixtures.
- Existing Auto-Editor/Nim work may inform migration utilities, but it is not a runtime dependency or module owner in `editor-core`.

## External references

- [MLT framework design](https://www.mltframework.org/docs/framework/)
- [MLT++ C++ wrapper](https://www.mltframework.org/docs/mlt++/)
- [MLT XML serialization](https://www.mltframework.org/docs/mltxml/)
- [MLT installation and modules](https://www.mltframework.org/docs/install/)
- [OTIO schema reference](https://opentimelineio.readthedocs.io/en/latest/api/python/opentimelineio.schema.html)
