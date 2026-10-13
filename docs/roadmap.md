# Roadmap

The roadmap validates the riskiest runtime assumptions before broad implementation. Durations are single-engineer engineering time and exclude the product UI, data connector suite, and any decision to port Auto-Editor analysis.

## Phase 0: C++/MLT vertical slice

**Estimate: 1–2 weeks.**

Build with CMake against a pinned MLT/MLT++ version on Windows. From C++ construct:

- Two video tracks and one independent audio track.
- Local MP4/WAV sources with independent source in/out and timeline placement.
- One dissolve, one fade, one volume change, and one title using the target Shotcut-compatible service.
- One alpha video generated outside MLT as a HyperFrames stand-in/fixture.
- An `avformat` MP4 consumer and an MLT XML consumer/output.

Acceptance:

- Rendered duration, frame rate, audio duration, alpha composite, and transition boundaries match the expected fixture.
- `.mlt` opens in the pinned Shotcut version; edit a title and track order, save, close, reopen, and render again.
- The worker starts on a clean Windows environment with its packaged modules/resources and no developer PATH assumptions.
- Progress, cancellation, and error handling are demonstrated with the chosen consumer/API.

**Stop condition:** if text compatibility, plugin deployment, or timing cannot be made deterministic, revise the design before implementing the Project compiler.

## Phase 1: Project package and validation

Define the Project JSON schema, asset manifest, stable IDs, frame/rational-time rules, template/data provenance, migrations, and package/relink behavior. Add schema validation, diagnostics, and deterministic serialization. Keep Project types independent from MLT.

## Phase 2: MLT compiler and MP4 render

Implement normalized Timeline IR and MLT++ graph construction for video/audio clips, gaps, multitrack composition, trims, speed, transforms, volume, fades, and a small transition set. Add render jobs, profiles, cancellation, progress, error mapping, and golden frame/audio tests. Expand only after fixture coverage is reliable.

## Phase 3: Shotcut project packaging and native titles

Generate editable `.mlt` from the compiled graph. Add relative media/font paths, resource packaging, XML structure tests, target-version fixtures, and explicit capability reports. Validate that title objects remain editable, not flattened.

## Phase 4: HyperFrames scene adapter

Call the pinned `@hyperframes/producer` runtime with validated template inputs. Persist the output asset hash and scene provenance; consume the alpha output in the MLT timeline. Add duration/fps/alpha/audio-sync and CJK tests.

## Phase 5: Optional OTIO and additional interoperability

Add OTIO output for supported tracks, clips, gaps, transitions, markers, and speed. Test imports in named target applications. Report effects/metadata that are not portable. OTIO is not a gate for MP4 or Shotcut delivery.

## Phase 6: Packaging and release hardening

Pin the MLT/FFmpeg/Qt/Pango module matrix. Build installers for the declared Windows release matrix, inventory licenses, test on a clean machine, and verify project relocation/relinking. Add CI fixtures for duration, frame hashes, audio levels, missing resources, and version migrations.

## Rough overall estimate

For an MVP backend covering Project-to-MLT compilation, MP4 rendering, editable Shotcut export, basic text, multitrack audio/video, and Windows packaging: **roughly 8–14 engineering weeks after the spike**, for one experienced C++/media engineer. OTIO export may add **2–4 weeks** depending on required target applications and effect mapping. A front end, data connectors, or a replacement for Auto-Editor analysis is additional scope.

## Definition of MVP

- Project validates, saves, reopens, and resolves packaged media by stable Asset ID.
- Same Project revision produces both MP4 and `.mlt` with matching clip timing and supported effects.
- Basic text is native/editable in Shotcut where declared supported.
- HyperFrames scenes are traceable generated assets and composite correctly.
- Unsupported or degraded effects are reported before export.
- Clean Windows installation can render without development tools installed.
