# Editor Core

A local-first, MLT-centered video project engine. This repository is currently an architecture plan; implementation starts after the MLT proof of concept passes.

## Direction

- Project package is the authoritative source for assets, timeline intent, templates, data snapshots, and generated-scene provenance.
- A C++ backend uses MLT++ directly to construct timeline graphs, render MP4 through MLT consumers, and emit editable Shotcut `.mlt` projects.
- FFmpeg remains a first-class dependency through MLT's `avformat` module and may also support probing, analysis, proxy, and utility tasks. There is no goal to remove FFmpeg.
- OTIO is an optional interoperability export, generated from the same compiled timeline representation. It is not the project container or render engine.
- `@hyperframes/producer` remains the HTML/CSS/GSAP scene renderer. Its alpha output is a regular Project asset and MLT track source.
- Nim and Auto-Editor are not the foundation for this new core. Legacy timeline import or analysis reuse is a separate decision.

## Documents

- [Architecture](docs/architecture.md): system boundaries, data flow, Project package, and renderer roles.
- [Modules](docs/modules.md): proposed C++ module responsibilities and dependency rules.
- [Roadmap](docs/roadmap.md): proof of concept, acceptance gates, risks, and rough estimates.
- [Decisions](docs/decisions.md): accepted direction and unresolved choices.

## Initial milestone

Build a small C++/MLT++ application that creates a multitrack graph, renders an MP4, writes an MLT XML project, and passes a Shotcut open/edit/save/reopen check on Windows. Do not begin a broad engine rewrite until this vertical slice validates the runtime, text filters, packaging, and media behavior.
