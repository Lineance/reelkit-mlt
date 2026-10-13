# Decisions

Status values: **Accepted** means the current plan assumes the decision; **Proposed** means it is the recommended default pending a proof or owner confirmation; **Open** means implementation should not lock it yet.

## Accepted

### Project is the upstream source of truth

The Project package owns assets, data snapshots, template references, HTML scene sources, semantic timeline objects, and render provenance. MLT XML, MP4, and OTIO are generated artifacts. There is no automatic reverse sync from Shotcut in v1.

### No Nim foundation

The new engine is not based on Nim or Auto-Editor. Prior work can be retained as migration/reference material but is not a dependency of `editor-core`.

### MLT is the timeline execution and render engine

The C++ backend calls MLT++ directly. It uses MLT producers/playlists/tractors/filters/transitions/consumers to execute a compiled Project timeline, render MP4, and emit editable MLT XML for Shotcut.

### FFmpeg remains a dependency

MLT's avformat module uses FFmpeg for media IO and encoding. FFmpeg can also back explicit probing, analysis, proxy, or specialized media utilities. The engine should not remove FFmpeg merely to adopt MLT.

### HTML animation remains an external renderer

`@hyperframes/producer` renders HTML/CSS/GSAP scenes into deterministic alpha media. MLT composites these outputs as regular assets.

## Proposed

### OTIO is optional downstream interchange

The OTIO exporter maps supported timeline concepts from the normalized Project compilation input. It does not sit between Project and MLT, and it is not a prerequisite for rendering. Maintain a loss/capability report.

### MLT worker is a separate process

Use a C++ worker linked to MLT++ with a versioned local JSON protocol. This isolates dynamic plugin loading and render failures from a future UI. The first spike can compare this model with direct in-process library ownership before finalizing IPC.

### Use fixture-driven Shotcut compatibility

Pin a target Shotcut/MLT version and generate `.mlt` structures from small Shotcut-authored golden projects. Do not infer private filter properties from names alone.

## Open

### Automatic edit analysis

Decide separately whether silence/scene/motion analysis is an MVP feature. If required, port or replace just that capability behind `media` services; do not make the renderer depend on the Auto-Editor codebase.

### Linux target matrix

Choose the initial Linux distribution baseline (currently Ubuntu 24.04), CPU/GPU requirements, supported codecs, and whether the product bundles a selected MLT module set or requires a separately installed runtime. WSL Ubuntu is the current development/test environment; native Linux device, GPU, and package behavior remains to be verified.

### Native title subset

Define the v1 subset of Text Simple/Rich, Pango/Qt title services, keyframes, and subtitles only after the Shotcut fixture test. Complex browser scenes remain pre-rendered.

### OTIO adapter scope

Choose target external applications and test actual import behavior before promising that MLT filters or native titles survive OTIO export.

### In-process vs worker IPC

The worker process is preferred for fault isolation, but the first render spike should measure API requirements for progress, cancellation, preview, and memory before freezing the process boundary.

## Rejected for now

- Use OTIO as the only Project container: it does not own the data/template/package lifecycle required by this product.
- Route rendering through `Project -> OTIO -> MLT`: OTIO does not preserve all MLT-specific services and adds a lossy conversion step.
- Rewrite HTML/CSS/GSAP rendering in MLT: HyperFrames remains the dedicated scene renderer.
- Treat `.mlt` as the authoritative Project: it is an engine/editor serialization, not the complete domain package or template/data history.
