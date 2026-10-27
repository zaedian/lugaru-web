# Lugaru-Web History and Lineage

## Purpose

Lugaru-Web is not a clean-room rewrite and should not describe itself as one. It sits at the end of a long technical and creative lineage:

**David Rosen / original Lugaru → platform ports and Lugaru HD → OSS Lugaru → Ryan Deering / Loupgarenne → Ryan's unmerged WebAssembly branch → Lugaru-Web.**

The project should preserve that lineage explicitly. The goal is not to collapse twenty years of work into a single author credit, but to understand what changed at each layer and build responsibly on top of it.

---

## 1. Original Lugaru

### David Rosen and the original game

Original Lugaru was written on Mac using CodeWarrior, OpenGL, FMOD, and QuickTime. Wolfire's historical FAQ says the Windows port was done by Defiance and the Linux port by Ryan C. Gordon (`icculus`).

The important technical character of the original code remains visible in the modern tree: the outer project has gained structure over time, but much of the gameplay brain is still concentrated in large legacy units such as `Person.cpp` and `GameTick.cpp`.

### Design character worth preserving

Lugaru's lasting strength is not content quantity. It is the way a small number of systems interact:

- directional/contextual melee combat;
- ragdoll physics with actual gameplay consequences;
- hard vs. soft landing surfaces;
- weapons that are physical objects rather than abstract inventory numbers;
- stealth perception based on sound, visibility, posture, distance, and smell;
- compact levels whose geometry supports combat movement;
- emergent interactions that produce stories without heavily scripting every situation.

**Design note (Lugaru-Web interpretation, not historical fact):** a useful design principle for Lugaru-Web is therefore:

> Add verbs and systems that create situations, rather than building content that only works because a script forces a particular situation to happen.

This would apply to possible future mechanics such as corpse dragging, throwable/noise-producing objects, environmental hazards, new weapon interactions, and Arena maps. These are ideas, not roadmap commitments; see [`design-direction.md`](design-direction.md).

---

## 2. Open-source Lugaru

Lugaru's source was later released under GPLv2+ and its game assets were relicensed under Creative Commons licenses. The modern OSS tree did substantially more than merely keep the old code compiling.

The OSS lineage added or consolidated work around:

- SDL2 and modern platform support;
- CMake/build cleanup and packaging;
- multiple campaigns;
- JSON-based map loading;
- modern resolution/fullscreen behavior;
- input/platform cleanup;
- CI and portability work;
- campaign preservation and relicensing;
- code organization into directories such as Animation, Audio, Environment, Graphic, Level, Menu, Objects, Platform, User, and Utils.

The modern source should therefore not be described as a frozen dump of Rosen's original code. It is a community-maintained descendant.

### Exact preservation checkpoints (2026-09-27)

**VERIFIED GIT HISTORY:** literal OSS 1.2 is `c7b99378439735c60f84869b05c6ebde53083667`, a release-side-branch child of shared preparation commit `a09b7f8c367dd9dabae2eeac9032217cfc6f901a`. Its release-only commit adds an animation-frame crash workaround. Later master continued separately from the shared parent; do not treat `1.2..master` as a simple linear sequence of post-release changes.

Ryan's inherited OSS fork point is `8edd5eec93285cfe942c6babe036603bac64b33e`. The inspected current OSS checkpoint, `42f148d033358925d1e31cec68ad02ee03ec64c2`, remains a useful living preservation/reference baseline. **RUNTIME-VERIFIED (maintainer comparison):** both official 1.2 and current OSS felt smooth for general character locomotion. This does not make all later OSS changes undesirable or clear Ryan of independently introduced regressions; each mechanism must be checked at the inherited base.

**VERIFIED GIT HISTORY / preservation decision (rabbit dialogue idle):** `25fd991fca18b740e622d8cf50532f18af1b9711` ("Set forgotten rabbit talk anim") assigns the already-existing `TalkIdle` animation to rabbits. Both the 1.2 preparation and literal release lack that assignment; Ryan's base already includes it, and current OSS retains it. `Idle` and `TalkIdle` assets are byte-identical across all four checkpoints. The more bouncy literal-1.2 dialogue fallback is treated as an omission/quirk, not a restoration target. Canonical later OSS TalkIdle remains; the temporary 1.2 experiment was removed. Literal 1.2 is not automatically preferable to legitimate later fixes.

### Community campaigns incorporated by OSS

OSS Lugaru formally incorporated three historical campaigns with explicit licenses:

- **Lugaru: Temple v1.0** by Silb, originally released 2006; CC BY-SA 3.0.
- **Lugaru: Empire v1.0** by Jendraz, originally released 2008; CC BY-SA 4.0.
- **The Seven Tasks v1.0** by Philtron R., originally released 2009; CC BY-SA 4.0.

The OSS preservation issue is important historical evidence because it documents the maintainers actively finding old mods, contacting creators, requesting public relicensing statements, replacing non-free assets, and only then integrating clean versions.

This is the precedent Lugaru-Web should continue.

---

## 3. Ryan Deering and Loupgarenne

Ryan Deering's Loupgarenne fork is the immediate technical parent of Lugaru-Web.

### Loupgarenne mainline focus

The fork's visible work includes:

- unlocked framerate work;
- variable timestep / frame-rate independence work;
- animation/timer/effect behavior at higher frame rates;
- build and UI cleanup;
- general refactoring and platform modernization.

This matters because not every behavioral difference between modern Lugaru and old Lugaru is a WebAssembly issue. Some regressions may have entered through Loupgarenne's timing work before the browser branch existed.

### Character regressions isolated and restored

**VERIFIED GIT HISTORY / VERIFIED SOURCE FINDING:** `1db27611eec7fe4c3f4019e233235e3a80fb2460` changed yaw and body-tilt convergence and head update/application. Source and physical A/B evidence support restoring these specific behaviors; they do not indict every modernization in the commit. The final default restorations and Web draw-order adaptation are documented in [architecture section 5](architecture.md#5-movement-facing-animation-and-high-refresh-behavior), with validation and the separate open scheduler experiment in [B-13](porting-notes.md#b-13-player-direction-changes-snap-instead-of-rotating). No engine-base migration decision follows from this milestone.

### Unmerged WebAssembly branch

Ryan created an unmerged `feature/wasm-support` branch. The verified browser commit is:

`b340204635e86a4385ef88dc45e2696e2213d9ab` (`wasm support`)

The branch was not merged into Loupgarenne main. It should not be described as vaporware or an uncommitted experiment: the work was committed and is substantial.

The browser port includes the broad ingredients expected in an Emscripten port of legacy Lugaru:

- Emscripten CMake integration;
- browser-specific platform code;
- SDL2 and image/audio Emscripten ports;
- GL4ES to bridge legacy OpenGL behavior toward WebGL;
- WebGL2 targeting;
- an Emscripten main loop;
- browser filesystem/persistence work;
- packaged/preloaded game data;
- custom browser shell behavior;
- optional CDN-data support, plus a shell redirect that loaded `lugaru.data` from Ryan's server on non-localhost hostnames (both later removed by Lugaru-Web; see section 4);
- jsoncpp and related dependency configuration.

Lugaru-Web preserves this source point with the annotated tag:

`baseline/wasm-b340204`

That tag represents a known-good playable browser baseline verified on 2026-09-16.

### What Ryan's work means for Lugaru-Web

Ryan solved the first hard question: **can this old codebase be made to run meaningfully in a browser?**

Lugaru-Web's contribution begins from there: validate the port, identify behavior changes, repair browser-specific bugs, make persistence and timing robust, document provenance, improve compatibility, then build a browser-native continuation on top.

This framing is more accurate than claiming Lugaru-Web invented the original WASM port.

---

## 4. Lugaru-Web's work after the baseline

The browser branch was playable, but not production-ready. Lugaru-Web v1.0.0 is the preservation and browser-stabilization release built from that baseline. The complete issue table is in [`PROJECT.md`](../PROJECT.md#known-browser-issues), with evidence in [`porting-notes.md`](porting-notes.md); the first fixes are summarized here because they shaped the project's working method.

### First browser fixes (2026-09-17)

**Web settings overrides**

The Web build had post-load overrides that defeated user settings. Resolution/VSync handling was corrected while motion blur stayed force-disabled on Web. The later B-6 investigation confirmed that the historical blur renders incorrectly on Web and closed with the Options row shown as unavailable (`fad80ac`); native blur is unchanged.

**Browser main-loop timing**

Main-loop registration and timing initialization were corrected so that VSync and timeout scheduling behave more predictably.

**Persistence**

The original browser branch used a base64/localStorage persistence path. Lugaru-Web replaced this with IDBFS mounted at `/home/web_user`, with startup synchronization before C++ execution, automatic persistence, and conservative one-time migration of legacy browser saves.

**Bounds checking**

Animation access had lost `.at()` bounds checking; Lugaru-Web restored it.

**WASM memory error in sprite deletion**

A delete/erase invalidation bug in `Sprite` handling caused a browser out-of-bounds crash. Deletion was deferred safely and iteration behavior corrected.

### Player direction regression: resolved (B-13)

Early validation reported that the player could snap orientation instead of smoothly rotating when reversing direction, including at high refresh rates. The investigation plan at the time was to trace it across:

1. original/OSS behavior;
2. Loupgarenne main;
3. Ryan's WASM branch;
4. current Lugaru-Web;
5. 60 Hz vs. 120/144+ Hz.

and not to assume the browser was the cause until the lineage was established.

That lineage comparison located the cause in Loupgarenne commit `1db2761`, which predates the WASM branch (see section 3). Historical bounded yaw, body tilt and head behavior were restored in `0305947` (2026-09-27) with combined physical QA. The same commit was later found to be the source of an unrelated jump-height regression (B-19, `21efae8`). Both cases illustrate why browser-visible differences should not be attributed to WebAssembly by default.

### Later stabilization and restoration

Work completed for v1.0 after the first fixes includes Web key rebinding (B-7), controller support (B-8; tutorial physical QA still open), Web NPOT texture handling (B-14), Web sprite and foliage performance (B-15, B-18), Empire progression and clothing repairs (B-16, B-17), Temple fur restoration (B-20 and the FurBW restoration `fef0364`), image/clothing compositing hardening (backlog A11), and removal of the inherited external data host so the Web release serves `lugaru.data` from its own origin (`36407bd`). Several of these defects were inherited from OSS content conversion or shared game logic rather than introduced by the browser port; each porting-notes entry records its provenance.

Release campaign QA (2026-10-05/06) fixed inherited shared behavior in dialogue interaction, minimap markers, pause/End Game music and the post-completion campaign map, and removed demonstrably stale copied dialogue from Temple maps and Seven Tasks `task9`. Unusual inherited content was deliberately kept, including the Temple-documented fallback map `svenskip`. See [porting notes](porting-notes.md#release-campaign-qa-fixes-2026-10-05--2026-10-06) and [preservation 19](preservation-and-modding.md#19-changing-inherited-campaign-data-v10-campaign-qa-review). For attribution: the current Temple ending is Lugaru-Web-authored text that replaced an explicit 2017 OSS placeholder (`217da32`). It is not recovered original Temple campaign prose.

---

## 5. Redagito/Lugaru as a reference implementation

The `redagito/Lugaru` repository is useful because it independently attacks some of the same architectural problems Lugaru-Web may face.

Observed characteristics include:

- C++17-oriented cleanup;
- a more explicit layered structure around Foundation/Game/App/Lugaru concerns;
- substantial attempts to modularize dependencies and reduce globals;
- similar remaining gameplay concentration in `Person.cpp` and `GameTick.cpp`;
- hundreds of remaining global references despite the restructuring effort.

The important lesson is not "merge Redagito." It is:

> Folder reorganization alone does not remove gameplay coupling.

Use the repository as a comparative architecture/reference implementation. When Lugaru-Web needs to extract an input layer, gameplay event system, per-player state, or another seam, Redagito may show one possible direction. It should not become the merge base unless there is a concrete reason.

---

# Lugaru 2 and the path into Overgrowth

The early Lugaru 2 material is valuable because it documents what Wolfire itself thought was worth evolving after Lugaru, including ideas that later changed or disappeared.

Sections 6–13 summarize published Wolfire material. Paragraphs marked **Design note** record how that material might inform possible future Lugaru-Web work. They are interpretation, not historical fact, and not roadmap commitments; current direction is described in [`design-direction.md`](design-direction.md) and [`PROJECT.md`](../PROJECT.md).

## 6. Lugaru 2 became Overgrowth

Wolfire formally announced Overgrowth on 2008-09-17 with the direct statement:

> "Overgrowth is the new Lugaru 2."

This establishes a direct design lineage rather than merely a spiritual resemblance.

Wolfire later described Overgrowth as taking place a few years after Lugaru with Turner returning as the main character.

---

## 7. Phoenix and movement/animation work

David Rosen's 2006 `Lugaru 2 movement` post describes an early Phoenix engine in which walking, running, crouching, sneaking, rolling, and jumping were already functional.

The especially important part is the animation philosophy: Phoenix generated or adjusted animation using surface information, physics, personality, and current state, with seamless transitions between movement states. Rosen noted that the animation system had recently been rewritten from scratch at that point.

**Design note:** this is useful to Lugaru-Web not because the old engine should be recreated, but because it shows the direction Rosen wanted to push:

- animation responding to the physical world;
- transitions rather than hard state snapping;
- character motion as part of simulation rather than decoration.

That is why the facing/rotation regression (B-13) was fixed by restoring the historical turning, tilt and head behavior rather than masking it.

---

## 8. Physics evolution

Rosen's 2009 discussion of the Overgrowth physics engine is unusually useful for understanding original Lugaru.

He describes Lugaru ragdolls as Newtonian particles linked by rigid stick constraints. He explicitly says ragdoll physics were integral to gameplay because players could manipulate fights so that Turner landed on soft surfaces while enemies hit hard ones.

For Overgrowth, Wolfire explored richer alternatives because the old system could not naturally represent things such as hinge joints and rigid-body collisions.

### Design note for Lugaru-Web

Do not replace Lugaru's physics merely because a newer physics engine could be more sophisticated.

The old system is part of the game's mechanics. New features should first ask what can be achieved by composing the existing ragdoll/skeleton/weapon systems.

Corpse dragging is a good example: it may not require replacing the ragdoll system at all. A dragged dead/ragdolled `Person` can remain a physical body while selected joints are constrained relative to the player.

---

## 9. Early species concepts

Aubrey Serr published early Lugaru 2 concept art showing broader species ideas, including foxes, tigers, and panthers. In 2009 he explicitly said those races would not be in Overgrowth and joked that they had been "eaten by the Whaleman."

Separately, Wolfire's early Overgrowth FAQ listed rabbits, wolves, cats, rats, and dogs as the planned character types.

This is useful historical context for future Lugaru-Web species work. It supports the idea that expanding beyond rabbits/wolves is compatible with the broader creative lineage, but it should not be presented as canon or as a promise to restore cut content.

**Design note:** for Lugaru-Web, species should initially be treated primarily as identity/model variation rather than as hardcoded classes with unique gameplay. Unique species mechanics can come later if the architecture supports them cleanly.

---

## 10. Mission structure and open-ended objectives

Wolfire described original Lugaru's mission structure bluntly as effectively **"Kill Everyone."** For Overgrowth they discussed additional objective types:

- reconnaissance;
- stealth traversal;
- escape;
- racing/time trials;
- assassination of selected targets;
- survival.

They also described levels as places where players could choose approaches such as stealth vs. frontal attack and lethal vs. non-lethal methods.

A recurring Wolfire design principle was to avoid locking the game into a rigid design too early. They preferred implementing systems, playtesting them, preserving what created good gameplay, and cutting/changing what did not.

### Design note for Lugaru-Web

Expanded content does not need to become script-heavy to gain mission variety. The strongest additions are objective frameworks that sit on top of existing systems:

- reach an area unseen;
- retrieve an item;
- survive for a time;
- kill a particular target;
- escape;
- complete a combat challenge under constraints.

This is compatible with the existing systemic design and with future challenge/Arena infrastructure.

---

## 11. Ruins and level-design lineage

Aubrey Serr's 2009 `The ruins of Lugaru` post describes the simple stone structures in Lugaru as both mysterious and mechanically important. The team wanted to preserve the abstraction while making the world more coherent and imaginative.

**Design note:** for possible future Lugaru-Web Arena design, the useful lesson is that compact geometry should primarily serve movement and combat:

- sightlines;
- elevation;
- jump routes;
- hard/soft landing surfaces;
- edges and fall risk;
- cover/occlusion;
- weapon placement;
- readable combat spaces.

Realistic decorative clutter should not compromise those functions.

---

## 12. Multiplayer lineage

Early Overgrowth material said cooperative multiplayer was planned. Later Overgrowth supported local multiplayer/versus, and Wolfire eventually prototyped online multiplayer as well.

**Design note:** Lugaru-Web should treat this as precedent, not a shortcut: the current Lugaru code still has substantial single-player assumptions.

The likely technical progression is:

1. abstract input actions;
2. bind control to a specific `Person`/player context rather than globally assuming `players[0]`;
3. support two local human-controlled players;
4. separate per-player camera/UI/state;
5. only then introduce networking;
6. prefer host-authoritative multiplayer for the first online implementation.

---

## 13. Input/accessibility lessons from later Overgrowth

By 2018, Overgrowth had accumulated several user-facing input features that make a useful reference target:

- controller navigation in menus;
- controller rebinding;
- per-controller bindings;
- axis sensitivity;
- dead-zone controls;
- mouse-button rebinding.

**Design note:** Lugaru-Web does not need to copy Overgrowth's implementation, but these are good examples of where a mature modernized version of this lineage eventually landed.

---

# Attribution principles

## 14. Do not flatten the lineage

When documenting a feature or bug, avoid statements such as "Rosen wrote this" unless history confirms it.

Use source history where useful:

- `git log`;
- `git blame`;
- OSS issue/merge history;
- Loupgarenne commits;
- preserved Wolfire posts.

A strange comment in `Person.cpp` may be Rosen-era code, later OSS work, or Ryan's change. Attribution should follow evidence.

## 15. Recommended project framing

A concise, accurate description is:

> Lugaru-Web is a browser-native continuation of the open-source Lugaru lineage. It builds on Ryan Deering's unmerged Loupgarenne WebAssembly work, preserves the original game and historical community content, modernizes the browser/platform experience, and provides a foundation for possible future Lugaru-compatible additions.

---

# Primary references

- Wolfire, `Announcing Overgrowth` (2008-09-17): https://www.wolfire.com/blog/2008/09/announcing-overgrowth/
- David Rosen, `Lugaru 2 movement` (2006-06-12): https://www.wolfire.com/blog/2006/06/lugaru-2-movement/
- Wolfire, `Overgrowth FAQ` (2009-02-12): https://www.wolfire.com/blog/2009/02/overgrowth-faq/
- Aubrey Serr, `Races in Overgrowth` (2009-05-10): https://www.wolfire.com/blog/2009/05/races-in-overgrowth/
- Aubrey Serr, `The ruins of Lugaru` (2009-02-05): https://www.wolfire.com/blog/2009/02/the-ruins-of-lugaru/
- David Rosen, `Creating the Overgrowth physics engine` (2009-05-20): https://www.wolfire.com/blog/2009/05/creating-the-overgrowth-physics-engine/
- Wolfire community Q&A (2009-07): https://www.wolfire.com/blog/2009/07/wolfire-answers-community-questions/
- Wolfire, later input/editor changes (2018): https://www.wolfire.com/blog/2018/05/What-s-new-at-Wolfire---Early-May/
- Wolfire, online multiplayer prototype (2021): https://www.wolfire.com/blog/2021/12/Overgrowth-Online-Multiplayer-Prototype/
- OSS Lugaru repository: https://github.com/WolfireGames/lugaru
- OSS content license manifest: https://github.com/osslugaru/lugaru/blob/master/CONTENT-LICENSE.txt
- OSS mod-preservation issue #31: https://gitlab.com/osslugaru/lugaru/-/issues/31
- Redagito reference repository: https://github.com/redagito/Lugaru
