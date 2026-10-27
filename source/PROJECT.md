# Lugaru-Web Project

## Overview

Lugaru-Web is an effort to maintain and extend a browser-native version of **Lugaru**, based on the open-source Lugaru codebase and Ryan Deering's Loupgarenne WebAssembly work.

Ryan's WebAssembly build already ran the game in a modern browser before Lugaru-Web began. The project is therefore not a ground-up browser port.

**Lugaru-Web v1.0.0** is the preservation and browser-stabilization release built on that baseline. It concentrates on understanding and stabilizing the inherited WebAssembly changes, restoring historical behavior and content, browser reliability and usability, and documentation, rather than adding a new gameplay layer.

Large feature work should not come at the expense of understanding or maintaining the existing codebase.

---

## Inherited Baseline

The inherited browser baseline is:

```text
Loupgarenne feature/wasm-support
Commit: b340204635e86a4385ef88dc45e2696e2213d9ab
```

This baseline is preserved with:

```text
baseline/wasm-b340204
```

Relevant references:

```text
main
    Current Lugaru-Web development branch.

loupgarenne-main
    Ryan Deering's Loupgarenne main branch.

upstream/master
    Local reference to WolfireGames/lugaru.
```

The canonical OSS Lugaru development project is maintained at:

```text
https://gitlab.com/osslugaru/lugaru
```

The existing Git history is retained so inherited work remains attributed to its original authors.

---

## Verified Browser Baseline (historical, 2026-09-16)

The WebAssembly baseline was manually tested on **2026-09-16** in a Chromium-based browser (Google Chrome 153; see `Docs/build-provenance.md`).

Surface-level testing confirmed:

* game startup and menus,
* tutorial and campaign levels,
* character models and animation,
* textures and terrain,
* keyboard controls,
* combat,
* audio,
* fullscreen,
* normal game progression.

This establishes `b340204` as the known-good browser baseline.

It does not imply that every subsystem or campaign has been fully validated. `Docs/build-provenance.md` records that later testing found some gameplay inputs could stop responding after browser focus changes, so combat and full input behavior were not considered fully verified by this smoke test.

---

## Current Web Architecture

The browser build currently uses:

* C++ Lugaru source,
* Emscripten and WebAssembly,
* SDL2,
* WebGL, with the build permitting up to WebGL 2,
* GL4ES,
* browser-compatible audio support,
* PNG, JPEG, Ogg and Vorbis support,
* jsoncpp,
* IDBFS (IndexedDB) for saved settings and progress,
* game data preloaded into `lugaru.data` and served from the same origin as the page.

GL4ES provides compatibility for Lugaru's legacy fixed-function OpenGL renderer. The pinned GL4ES revision is unchanged from the baseline; CMake applies one small local patch so that alpha-to-coverage reaches WebGL (B-18).

Because the renderer is currently functional through GL4ES, replacing or substantially rewriting it is not a project objective unless profiling or compatibility testing demonstrates a concrete need.

Incremental improvements should be preferred over unnecessary architectural rewrites.

---

## Development Principles

### Preserve the Working Baseline

The browser build should remain playable throughout development whenever reasonably possible.

Changes to shared engine code should consider native behavior as well as browser behavior. Native builds can act as a reference when determining whether an issue is inherited from Lugaru or introduced by the WebAssembly port.

### Make Small, Reviewable Changes

Changes should address one logical concern whenever practical.

Before a change is committed, its diff should be reviewed, its purpose understood, the affected target built, and the relevant behavior tested.

Suggested or generated code is a proposal until it has been understood, reviewed, built, and tested.

### Preserve History and Attribution

Existing author attribution and upstream history should remain intact.

Imported patches or work derived from other projects should retain appropriate attribution and licensing information.

### Fix Defects Before Expanding Scope

Problems affecting stability, saved progress, input, timing, portability, or maintainability should generally take priority over new gameplay systems.

### Measure Before Optimizing

Performance changes should be based on reproducible measurements or profiling rather than assumptions.

---

## Known Browser Issues

Detailed investigation notes belong in:

```text
Docs/porting-notes.md
```

This file tracks only the current high-level state. Fixing commits, evidence and validation limits for each issue are recorded in `Docs/porting-notes.md`.

| ID   | Summary                                                                           | Status | Priority |
| ---- | --------------------------------------------------------------------------------- | ------ | -------- |
| B-1  | Web settings overridden after configuration load (`abb81d7`)                      | Fixed  | High     |
| B-2  | Browser main loop uses timeout scheduling as a consequence of B-1 (`abb81d7`)     | Fixed  | High     |
| B-3  | Browser persistence migrated from localStorage to IDBFS with safe legacy migration (`3d747d3`) | Fixed  | High     |
| B-4  | Animation bounds checking removed in shared code (`b222ddc`)                      | Fixed  | High     |
| B-5  | Asset case fallback: latent external-content compatibility risk                    | Closed (note retained) | Low |
| B-6  | Web blur intentionally disabled; Options row unavailable (`fad80ac`)              | Fixed  | Low      |
| B-7  | Browser key rebinding replaced with Web-only nonblocking capture (`a7bfd51`)      | Fixed  | Medium   |
| B-8  | Controller gameplay/camera/menu UI Firefox/wired DualSense verified; tutorial physical QA and compatibility/tuning open | Partial | Medium |
| B-9  | Web build is optimized primarily for binary size                                  | Open   | Low      |
| B-10 | Synchronous level loading can block the browser main thread                       | Open   | Low      |
| B-11 | Account creation behavior was changed globally rather than only for Web builds    | Open   | Medium   |
| B-12 | WebAssembly runtime crash: memory access out of bounds (`7d4349b`)                | Fixed  | High     |
| B-13 | Historical yaw, tilt, and head behavior restored; combined physical QA passed (`0305947`) | Fixed  | Medium   |
| B-14 | Web NPOT mipmap compatibility fixed; snow and Fire loading-screen QA passed (`eea54f4`) | Fixed | Medium |
| B-15 | Fire-heavy Seven Tasks task3 Web sprite rendering optimized (`592e158`) | Fixed | High |
| B-16 | Delayed spatial exit lost Empire campaign progression; shared, not Web-specific (`97121db`) | Fixed | High |
| B-17 | Empire Sky Palace NPC clothing corrupted by inherited fire-texture path conversion (`6142c73`, `bf94eca`) | Fixed | Medium |
| B-18 | Dense Web foliage slowdown caused by GL4ES alpha-test rendering (`df72be9`) | Fixed | High |
| B-19 | Jump boost applied too quickly under fixed physics timestep (`21efae8`) | Fixed | High |
| B-20 | Temple tinted JPEG fur rendered with RGB striping (`85477f5`) | Fixed | Medium |

---

## Build Status

### WebAssembly

The v1.0 Web release is built with:

```text
WSL2
Ubuntu 26.04.1
Emscripten 6.0.9
CMake 4.2.3
Release build directory: build-web-release
```

The build produces:

```text
lugaru.html
lugaru.js
lugaru.wasm
lugaru.data
```

The four files must be deployed together. The Web build loads `lugaru.data` from the same origin and directory as `lugaru.html`, not from Ryan Deering's server (`36407bd`). The public GitHub Pages deployment at https://loganwalkerdev.github.io/lugaru-web/ was verified on 2026-10-13: all four files return HTTP 200 with the expected types from the same origin. See [build provenance](Docs/build-provenance.md#public-deployment-verification-2026-10-13). The files must be served over HTTP rather than opened through `file://`.

Exact environment information, dependency revisions, build commands, compiler configuration, known warnings, and the preserved 2026-09-16 baseline record are maintained in:

```text
Docs/build-provenance.md
```

Final v1.0.0 artifact sizes and SHA-256 hashes, built from source commit `1b81e1d`, are recorded in `Docs/build-provenance.md`. Per-issue runtime validation is recorded in `Docs/porting-notes.md`.

### Native Builds

Native builds should remain available as comparison targets where practical. They are not part of the v1.0 Web release and are not release-validated.

A change to shared game code should not knowingly break a supported native target without a documented reason.

The retained GitHub Actions workflow builds native Linux, macOS and Windows comparison targets only; no workflow builds or tests the Web target, and v1.0.0 Web packaging and deployment are manual. See [build provenance](Docs/build-provenance.md#ci) and CI-1 in `Docs/porting-notes.md`.

---

## Current Development Phase

### v1.0.0: Preservation and Browser Stabilization Release

The baseline-preservation and stabilization work that began from `baseline/wasm-b340204` is complete for v1.0.0:

* inherited Web regressions in settings, frame scheduling, persistence, bounds checking, sprite deletion, key rebinding, and texture handling are fixed;
* historical character turning, tilt, head behavior, and jump timing are restored;
* Web sprite and foliage performance regressions are fixed without reducing historical content;
* Empire progression/clothing and Temple fur/clothing content are repaired or restored, with provenance recorded;
* the audited PNG/JPEG image-decode and clothing-compositing paths are hardened with size/dimension validation; known unchecked paths outside that audit remain documented, and unfixed, in investigation-log entry A11;
* controller support covers gameplay, camera, menus, campaign/world map, and challenges;
* the Web build loads `lugaru.data` from the same origin as the page (`36407bd`);
* full release QA covered all bundled campaigns (Original, Seven Tasks, Temple and Empire) and the challenges, and Chrome and Firefox were separately browser smoke/performance tested;
* Web defaults were tuned, and browser fullscreen now starts windowed and stays in sync with the browser.

The complete list of v1.0.0 changes, including the fixes found during release QA, is in [`RELEASE-NOTES.md`](RELEASE-NOTES.md). Evidence, fixing commits and validation limits for each issue are in [`Docs/porting-notes.md`](Docs/porting-notes.md).

Major gameplay additions continue to wait until after v1.0.0; see **Near-Term Priorities**.

---

## Near-Term Priorities

### v1.0.0 deployment and validation follow-ups

The v1.0.0 Web release is live. The open items below are validation follow-ups, not blockers to publication.

1. **Complete (2026-10-13):** the Web build is deployed to GitHub Pages (`gh-pages` commit `6238a50`), and the public same-origin/MIME check passed. See [build provenance](Docs/build-provenance.md#public-deployment-verification-2026-10-13).
2. Physically verify the Triangle tutorial advance and tutorial stage-3 right-stick recognition (B-8), or keep them documented as source/automated-validated only.
3. Optionally confirm the final Web `Blur Effects: Unavailable on Web` Options row physically (B-6).

### After v1.0.0

1. Isolate the inherited global `"Player"` auto-account behavior to Web builds, or document why it should remain shared (B-11).
2. Controller compatibility and feel: broader browser/Bluetooth coverage, sensitivity/threshold tuning, settings/prompts (B-8, investigation log A6). True analog locomotion remains a separate design question; Classic digital intent is unchanged.
3. Benchmark WebAssembly optimization settings (B-9).
4. Improve browser loading behavior if testing shows it is necessary (B-10).
5. CI modernization: restore useful native CI coverage, fix the macOS dependency setup, add a Web build job, and add caching where practical (CI-1, investigation log A5).

Items not listed here remain tracked but are not currently scheduled.

The exact order may change when testing exposes dependencies or more serious defects.

---

## Determinism

Deterministic simulation may become useful for future reproducible game modes, testing, replay systems or other extensions.

The current game uses behavior including standard-library random generation seeded from wall-clock time, with relevant code beginning in:

```text
Source/Math/Random.hpp
```

Simulation timing would also require investigation before identical seeds could be expected to produce identical runs across different machines or frame rates.

This is not an immediate development priority. If systems requiring reproducibility are pursued, determinism should be treated as a prerequisite rather than a later addition.

---

## Future Direction

The project may eventually explore browser-native additions beyond the original Lugaru experience.

Possible areas include new challenge or combat modes, replay-oriented features, customization, and improved support for community-created content.

These are intentionally not defined as committed roadmap items. Specific features should be evaluated only after the browser build is stable and their technical requirements are understood.

---

## Scope Boundaries

The following are not current project objectives:

* rewriting Lugaru from scratch,
* replacing the renderer without demonstrated need,
* large unrelated engine refactors,
* implementing multiplayer,
* adding monetization systems,
* incorporating proprietary Overgrowth code or assets,
* prioritizing major gameplay expansion over baseline stabilization.

These boundaries can be revisited as the project matures.

---

## Licensing and Attribution

Lugaru source code and game content have separate licensing requirements.

The source code is distributed under **GNU GPL version 2 or later (GPLv2+)**.

Content under `Data/` is distributed under Creative Commons Attribution-ShareAlike licenses as documented by `CONTENT-LICENSE.txt`, including **CC BY-SA 3.0** and **CC BY-SA 4.0** content.

GL4ES is distributed under the MIT license and requires its applicable copyright and license notices to be preserved.

Licensing, dependency and contributor information is maintained in:

```text
COPYING.txt
    GNU GPL version 2 license text for the source code.

CONTENT-LICENSE.txt
    Game asset licenses, campaign attribution and Lugaru-Web content modifications.

THIRD-PARTY-NOTICES.txt
    Notices and licenses for third-party software used by the Web build, including GL4ES and jsoncpp.

AUTHORS
    Original developer, OSS contributors, Loupgarenne and Lugaru-Web authorship and lineage.
```

Any public distribution of compiled WebAssembly builds, including hosting them on a public web server, must satisfy the applicable GPL source-distribution requirements.

Modified or derived game assets must follow the applicable Creative Commons license.

Proprietary Overgrowth code or assets must not be incorporated into this project.

### Project Naming

The open-source licenses covering Lugaru's code and content do not themselves grant trademark rights.

The current project name describes its relationship to Lugaru rather than claiming to be an official Wolfire release. Long-term public branding and naming should be reviewed before the project is broadly promoted or distributed.

---

## Documentation

Project documentation should remain concise, technical and maintainable.

```text
README.md
    Public overview, play link, controls and known Web limitations.

PROJECT.md
    Stable project direction, current phase and scope.

Docs/README.md
    Index of the technical, historical, preservation and design documentation.

Docs/build-provenance.md
    Current release build procedure, dependency versions and commands, and the preserved baseline build record.

Docs/porting-notes.md
    Detailed WebAssembly issues, testing observations and resolutions.

Docs/architecture.md
    Current engine mental model, important couplings, source archaeology and extension seams.

Docs/research-backlog.md
    Investigation log: resolved investigation records and open technical and historical questions.

COMPILING.md
    Build entry points for the Web build and inherited native instructions.

RELEASE-NOTES.md
    Lugaru-Web release notes, followed by inherited OSS Lugaru release history.
```

There is no separate contribution guide at present. `AGENTS.md` records the working rules used for automated-assistant contributions.

Documentation should describe the project and its technical reasoning rather than serve as a transcript of development sessions.

---

## Project Direction

The v1.0.0 objective is:

**Turn the existing working Lugaru WebAssembly port into a documented, reproducible, stable and maintainable browser version of Lugaru.**

Future development should build on that foundation rather than replace it.
