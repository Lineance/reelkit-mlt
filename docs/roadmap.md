# Roadmap

The roadmap validates the riskiest runtime assumptions before broad implementation. Durations are single-engineer engineering time and exclude the product UI, data connector suite, and any decision to port Auto-Editor analysis.

## Phase 0: Linux C++/MLT vertical slice

**Status: passed in Ubuntu 24.04.4 / MLT 7.22.0.**

The C++/MLT++ proof of concept creates two color clips, a 15-frame dissolve, and a `qtext` filter; renders H.264 through the MLT `avformat` consumer; serializes MLT XML; then reloads and renders that XML.

Acceptance passed:

- 640x360 progressive output at 30/1 fps, 165 frames, 5.5 seconds.
- Decoded frame hashes match exactly between the direct graph and reloaded MLT XML.
- Title and dissolve are visible in extracted frames.
- The Linux build links MLT++ directly; the `melt` CLI is not required.

The detailed environment and command are in [the WSL render report](wsl-mlt-render-poc.md).

## Phase 0.5: Linux feature matrix

Four independent CTest programs now exercise timeline/dissolve/XML reload, tone mixing, qtext opacity keyframes, and a semi-transparent qtext overlay. All four currently pass on the Ubuntu 24.04 / MLT 7.22 environment. See [the feature-test report](linux-feature-tests.md).

Remaining validation before freezing the compiler contract:

- The audio tone's configured `level` does not yet produce the expected output attenuation; single-tone and mixed-tone renders measure at nearly the same peak. Treat gain/limiting semantics as unresolved.
- The alpha case uses a qtext-generated transparent overlay, not an external alpha video from HyperFrames.
- Shotcut GUI editing/save/reopen has not been exercised yet.

Resolve these behaviors with focused tests before locking Project-to-MLT mappings.

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

## Phase 6: Linux packaging and release hardening

Pin the MLT/FFmpeg/Qt/Pango module matrix. Select the supported Linux distribution/runtime strategy, inventory licenses, test the package in a clean Linux environment, and verify project relocation/relinking. Add CI fixtures for duration, frame hashes, audio levels, missing resources, and version migrations.

## Rough overall estimate

For an MVP backend covering Project-to-MLT compilation, MP4 rendering, editable Shotcut export, basic text, multitrack audio/video, and Linux packaging: **roughly 8–14 engineering weeks after the feature spike**, for one experienced C++/media engineer. OTIO export may add **2–4 weeks** depending on required target applications and effect mapping. A front end, data connectors, or a replacement for Auto-Editor analysis is additional scope.

## Definition of MVP

- Project validates, saves, reopens, and resolves packaged media by stable Asset ID.
- Same Project revision produces both MP4 and `.mlt` with matching clip timing and supported effects.
- Basic text is native/editable in Shotcut where declared supported.
- HyperFrames scenes are traceable generated assets and composite correctly.
- Unsupported or degraded effects are reported before export.
- A clean Linux runtime package can render without development tools installed.
