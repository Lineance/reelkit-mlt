# Editor Core

A local-first, MLT-centered video project engine. This repository contains the architecture plan and an initial direct-render proof of concept; the Project compiler and production engine are not implemented yet.

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
- [Linux feature tests](docs/linux-feature-tests.md): independent CTest scenarios, results, and known gaps.
- [Decisions](docs/decisions.md): accepted direction and unresolved choices.

## Initial milestone

The first direct C++/MLT++ slice now builds and renders in WSL Ubuntu 24.04 using MLT 7.22, then reopens the generated MLT XML and renders it again. See [the render report](docs/wsl-mlt-render-poc.md). The current next milestone is the independent [Linux feature-test suite](docs/linux-feature-tests.md), before freezing the Project-to-MLT compiler contract.
