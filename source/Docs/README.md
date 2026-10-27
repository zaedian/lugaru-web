# Lugaru-Web Documentation Index

These documents capture the technical, historical, preservation, and design understanding of Lugaru-Web: the browser port and its fixes, controller support, character restoration, campaign and content repairs, and the clothing/image audit completed for v1.0.

## Current status (2026-10-13)

Lugaru-Web v1.0.0 is the preservation and browser-stabilization release. The issue summary is in [`PROJECT.md`](../PROJECT.md#known-browser-issues), with fixing commits, evidence and validation limits in [`porting-notes.md`](porting-notes.md). The complete list of v1.0.0 changes is in [`RELEASE-NOTES.md`](../RELEASE-NOTES.md).

The final v1.0.0 Web artifacts were built on 2026-10-13 from source commit `1b81e1d` with Emscripten 6.0.9; their sizes, SHA-256 hashes and local smoke test are recorded in [`build-provenance.md`](build-provenance.md#current-v10-release-build). The public GitHub Pages deployment was verified on 2026-10-13 (see [`build-provenance.md`](build-provenance.md#public-deployment-verification-2026-10-13)). Open items are summarized in [`PROJECT.md`](../PROJECT.md#near-term-priorities).

## Documents in this directory

They are intentionally separated by purpose:

- `history-and-lineage.md`: project ancestry, authorship, Lugaru 2/Overgrowth research, and what each lineage layer contributed.
- `architecture.md`: a living mental map of the current codebase, including the campaign/world-map/dialogue/hotspot objective machinery, important couplings, and likely extension seams.
- `design-direction.md`: preservation/product principles, Classic/Expanded/Arena rulesets, UI direction, mechanics, multiplayer, and long-term product identity.
- `preservation-and-modding.md`: historical community content, archaeological leads, mod/tool preservation, licensing workflow, restored-edition policy, and the evidence rule for changing inherited campaign data.
- `research-backlog.md`: investigation log with dated records of resolved investigations, followed by open research questions.
- `porting-notes.md`: browser-port issues B-1 to B-20 and CI-1, plus the release campaign QA fixes, with implementation, build and runtime evidence, fixing commits, and validation limits, including controller gameplay/menu UI and tutorial validation limits.
- `build-provenance.md`: the v1.0 Web release build procedure and final artifact record, and the preserved 2026-09-16 baseline build record (toolchain, dependency pins, commands, historical artifact hashes).

Historical and upstream documentation also lives here and should not automatically override current source or the documents above:

- `DEVTOOLS.txt`: OSS Lugaru in-game developer tools reference.
- `README.Temple.txt`, `README.Empire.txt`, `README.SevenTasks.txt`: original campaign readmes.

## Related root-level files

- `../README.md`: public project overview, play link, controls, and known Web limitations.
- `../PROJECT.md`: project scope, principles, current status, issue table, and near-term priorities.
- `../COMPILING.md`: build entry points (Web and inherited native instructions).
- `../RELEASE-NOTES.md`: Lugaru-Web release notes followed by the inherited OSS Lugaru release history.
- `../COPYING.txt`, `../CONTENT-LICENSE.txt`, `../THIRD-PARTY-NOTICES.txt`, `../AUTHORS`: source license, game-content licenses and attribution, third-party software notices, and authorship/lineage.

## Documentation conventions

Stable documentation should distinguish between:

- **Verified history**: supported by source history, public Wolfire material, OSS records, or repository evidence.
- **Code finding**: observed in the current source tree or a reference repository.
- **Decision**: current Lugaru-Web direction.
- **Idea**: plausible future work, not a commitment.
- **Open question**: something still requiring code archaeology, historical research, testing, or licensing verification.

The project should prefer evidence over folklore. When attribution matters, use repository history, `git blame`, original forum/blog posts, and preserved licensing statements rather than assuming every old line or behavior came directly from David Rosen.

Dated research and validation entries are preserved as written.

## Where to find key topics

- **Controller support**: evidence and limits in [porting notes B-8](porting-notes.md#b-8-controller-gameplay-menuui-and-tutorial-support); remaining compatibility, tuning and scope work in [investigation log A6](research-backlog.md#a6-controller-support-completed-scope-and-remaining-work).
- **Character restoration**: design in [architecture section 5](architecture.md#5-movement-facing-animation-and-high-refresh-behavior); source/runtime evidence in [B-13](porting-notes.md#b-13-player-direction-changes-snap-instead-of-rotating); lineage checkpoints in [history and lineage](history-and-lineage.md#exact-preservation-checkpoints-2026-09-27).
- **Web sprite and foliage performance**: [B-15](porting-notes.md#b-15-dense-fire-sprite-rendering-slows-web) and [B-18](porting-notes.md#b-18-dense-web-foliage-causes-severe-slowdown); investigation records in [A8](research-backlog.md#a8-dense-fire-spawner-web-slowdown-resolved-2026-09-28) and [A9c](research-backlog.md#a9c-empire-sky-palace-dense-foliage-web-slowdown-resolved-2026-09-30).
- **Empire campaign progression**: [B-16](porting-notes.md#b-16-delayed-spatial-exit-loses-campaign-progression-empire-sky-palace), state semantics in [architecture 20.3](architecture.md#delayed-completion-occupancy-vs-recorded-win), and investigation records in [A9](research-backlog.md#a9-empire-sky-palace-campaign-progression-resolved-2026-09-29) and [A9a](research-backlog.md#a9a-throne-room-emperor-dialogue-activation-area-resolved-2026-09-29).
- **Clothing, skins, and Temple fur**: [B-20](porting-notes.md#b-20-temple-tinted-jpeg-fur-renders-with-rgb-striping) and the audit, ASan evidence and preserved compositor quirk in [investigation log A11](research-backlog.md#a11-clothing-skins-and-historical-asset-audit-resolved-2026-10-03).
- **Self-contained Web release**: [build provenance](build-provenance.md#same-origin-game-data).
- **Release campaign QA**: fixes and validation in [porting notes](porting-notes.md#release-campaign-qa-fixes-2026-10-05--2026-10-06); the evidence rule for changing inherited campaign data, and the data deliberately left unchanged, in [preservation 19](preservation-and-modding.md#19-changing-inherited-campaign-data-v10-campaign-qa-review); the full list of release QA fixes in [`RELEASE-NOTES.md`](../RELEASE-NOTES.md).
