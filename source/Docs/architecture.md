# Lugaru-Web Architecture: Living Mental Map

## Purpose

This document is a working map of the current codebase. It is not a refactor mandate.

The goal is to answer:

1. Where does a behavior enter the engine?
2. Which systems own or mutate it?
3. Which old assumptions make extension difficult?
4. Where can Lugaru-Web add a clean seam without rewriting working gameplay?
5. Which behavior is original/OSS/Loupgarenne/browser-specific?

The preferred approach is **feature-driven refactoring**. Extract a seam when a real feature needs it and the existing coupling makes that feature unsafe or repetitive.

---

# 1. Codebase shape

A recent source inspection found approximately:

- 678 files total in the inspected project snapshot;
- ~92 files under `Source/`;
- ~38,500 C/C++ lines;
- `Person.cpp` ~8,583 lines;
- `GameTick.cpp` ~5,207 lines.

The source tree has a reasonably legible outer organization:

- Animation
- Audio
- Environment
- Graphic
- Level
- Menu
- Objects
- Platform
- User
- Utils

But the architecture is still strongly centralized around several legacy gameplay units, especially `Person.cpp` and `GameTick.cpp`.

This is best thought of as a **modular shell around a legacy gameplay core**, not either a pure monolith or a clean modern component architecture.

---

# 2. High-level control/data flow to trace

The current mental map should eventually be verified file-by-file along this path:

```text
Platform / SDL events
        ↓
raw keyboard/mouse state
        ↓
player input interpretation
        ↓
Person movement/facing/action decisions
        ↓
animation state
        ↓
skeleton / ragdoll / physical body
        ↓
combat + weapons + collisions
        ↓
damage / knockdown / death
        ↓
AI perception and reactions
        ↓
level/campaign/tutorial state
        ↓
rendering + HUD + audio + persistence
```

Parallel concerns:

```text
browser platform → filesystem/persistence → accounts/settings/campaign progress
menu/UI → settings/input/campaign selection
map data → objects/hotspots/players/path points/dialogue
```

This flow should become the basis of a durable architecture map rather than a giant one-time refactor document.

---

# 3. Input

## Current observation

The current SDL event/input layer is small and still exposes low-level state such as `keyDown` / `keyPressed`. There are signs in gameplay code that input abstraction was already recognized as desirable.

Web key rebinding now uses nonblocking SDL event capture (`pollKeyCapture()` in `Menu.cpp`), verified in Chrome on 2026-09-24. Native capture still uses `SDL_CreateThread`; its separate thread-affinity/portability question remains open (research backlog A4a).

**VERIFIED SOURCE FINDING:** Controller foundation milestone 1 is implemented in `e5939fe`. `Source/Utils/Input.cpp` / `Input.hpp` expose `hasController()`, `controllerAxis()`, and `controllerButtonDown()` separately from keyboard/mouse bindings. `Source/main.cpp` calls `InitController()` at setup, `UpdateController()` during updates, and `ShutdownController()` before SDL shutdown. SDL GameController initialization discovers recognized startup devices; discovery repeats at one-second intervals while none is active. One controller is retained by handle/instance identity, with fresh enumeration on reconnect and cached state cleared on disconnect. There is no additional SDL event-queue consumer.

**VERIFIED SOURCE FINDING (gameplay integration):** movement (`4b8af1d`) ORs `Input::controllerMovement()` into the existing `Person::players[0]` forward/back/left/right booleans in `Game::Tick()`. Independent left-axis hysteresis activates at `10000` and releases at `7000`; disconnect/reset clears its latches. Existing Classic speed, animation, facing, and eight-direction combinations remain authoritative. Directional snapping is expected for this digital implementation; proportional movement/arbitrary facing would require separate animation/combat/timing investigation.

Gameplay actions (`2387ebf`) combine held keyboard/mouse state with Cross jump, Circle crouch, Square draw/sheath, L1 throw/pickup, and R1 attack. Existing gameplay transitions and toggles remain in place. `doAttacks()` receives combined held attack with `oldattackkey` unchanged; `Input::Tick()` derives the combined attack press edge used by dialogue initiation (`Dialog::tick()`) and advancement (`Game::Tick()`). Jump-based respawn also accepts combined input. Raw developer input remains separate, and disconnect does not clear held keyboard/mouse input.

Camera look (`ab46b53`) feeds `Input::controllerLook()` into `Game::TickOnce()` beside mouse look. An axial `8000` deadzone rescales remaining travel continuously; yaw/pitch maxima are `120°/s` / `90°/s`. Right/down increase yaw/pitch; mouse sensitivity/inversion do not alter stick response. Integration uses `rate × realmultiplier` once per outer frame, before physics substeps and independently of gameplay time modifiers, subject to the existing 250 ms stall cap. Existing `[-70°, 90°]` pitch limits and menu/dialogue guards remain. Triggers do not rotate the camera.

**VERIFIED SOURCE FINDING (pointer UI, `8d549e4`):** `DoUpdate()` calls `Input::MoveControllerMenuCursor(realmultiplier)` once per outer frame, after mouse movement. Left stick uses continuous response outside an `8000` axial deadzone at up to `320` logical pixels/second; D-pad overrides it for `80` pixels/second precision motion. Both feed the existing shared cursor, scaled from logical 640×480 coordinates and clamped to screen bounds. `Menu::Tick()` retains pointer hit-testing and activation: Cross joins the existing left-click dispatcher locally, Circle selects existing Back/No/Resume actions, and campaign cycling/world-map markers retain existing progress and load logic. There is no discrete-focus menu or synthetic mouse input.

Start joins existing gameplay pause and actual pause-menu resume transitions, including audio, but is not Back in nested screens. Challenge-result Cross/Circle use existing continue/back handlers. `syncControllerContext()` and controller press history suppress held buttons through release across UI/gameplay/modal transitions; raw keyboard/mouse state remains independent. Name entry still needs a keyboard, controller confirm cannot start rebinding, and stereo decrement is mouse-only.

**VERIFIED SOURCE FINDING (tutorial seams):** `bd7b207` adds Triangle press-edge advance through `Input::controllerTutorialAdvancePressed()` and the existing Tab handler, with tutorial/gameplay guards and release suppression. Stage 51 and keyboard Tab behavior are unchanged. `f00ec72` extends stage 3's mouse criterion with `Input::controllerLook()` X/Y after the existing deadzone. Tutorial-call guards plus active-gameplay/input-waiting/dialogue checks exclude inactive contexts. No mouse deltas, deadzone math, stage timing, or camera tuning are changed.

**Build validation / RUNTIME-VERIFIED:** milestones 1–5 and tutorial additions built for Web and passed focused SDL/input/timing checks. Physical wired DualSense/Firefox tests with DS4Windows disabled verified discovery/lifecycle, movement, five actions, attack-driven dialogue, look, pause, shared cursor/confirm/Back, campaign/world-map and challenge/menu flows, and simultaneous keyboard/mouse use. **Physical verification remains pending for Triangle advance and stage-3 controller-look recognition.** See [B-8](porting-notes.md#b-8-controller-gameplay-menuui-and-tutorial-support) for evidence limits; automated multi-rate tests do not establish physical multi-refresh-rate or Bluetooth support. A later user test verified wired DualSense input in current Chrome; see B-8 for the earlier failures and limits of that result.

**Decision / open work:** preserve Classic digital movement while separately evaluating movement feel and camera tuning. True analog locomotion/arbitrary facing is not implemented. NPC steering generally uses continuous target headings; shared turning and presentation were investigated separately from controller input. The residual NPC slope/seam scheduler observation remains open (backlog A2a). Compatibility, configuration, prompts, and scope exclusions are tracked in backlog A6. The broader action layer below remains a desired seam, not a description of the implemented controller path.

## Desired seam

Introduce a semantic action layer such as:

```text
MoveForward
MoveBackward
MoveLeft
MoveRight
Jump
Crouch
Attack
Throw
DrawWeapon
Interact
Pause
```

Bindings then become inputs to actions:

```text
keyboard/mouse ─┐
controller      ├─> InputActionState ─> controlled Person
future network ─┘
```

This is high leverage because it supports:

- keyboard rebinding;
- mouse-button rebinding;
- controller support;
- analog movement;
- stick deadzones and sensitivity;
- menu navigation;
- local multiplayer;
- network input;
- input recording/replays;
- accessibility changes.

### Design constraint

Modern controls are **not** an Expanded gameplay mechanic. Controller support, rebinding, analog input, deadzones, and high-refresh fixes should work in Classic as well.

---

# 4. Person and player control

`Person` remains the center of a huge amount of gameplay behavior:

- locomotion;
- facing;
- animation decisions;
- attacks/counters;
- ragdoll transitions;
- weapon interaction;
- AI/player distinctions;
- tutorial-specific checks;
- species/type assumptions.

The code uses a `Person::players` collection, but many systems still assume the primary human is `players[0]`.

## Multiplayer implication

The first multiplayer architecture task is not networking. It is removing the assumption that "the human" means global player zero.

A controlled-person context should be able to answer:

- which input source owns this Person?
- which camera follows it?
- which HUD/state belongs to it?
- which account/customization does it use?

Once two local human-controlled Persons work correctly, network ownership becomes much less invasive.

---

# 5. Movement, facing, animation, and high-refresh behavior

**VERIFIED SOURCE FINDING (finalized character behavior, 2026-09-27):** `GameTick.cpp::doAerialAcrobatics()` retains the historical state predicates and bounded yaw rates: ordinary `500°/s`, faster states `1000°/s`, entering sneak `2000°/s`, integrated with per-tick `multiplier`. `Game::Tick()` approaches `targettilt` with `stepTowardf(..., multiplier * 150)`; target-tilt generation is unchanged.

`Person::DoHead()` retains historical target conversion/normalization, angular limits, bounded yaw/pitch rates of `500°/s` / `250°/s`, and pitch-only head-joint rotation about the neck. Player targets come from camera view-direction yaw/pitch relative to body orientation; NPC and dialogue target producers and animation eligibility are unchanged. This is not a world-space camera-position look-at operation.

`TickOnceAfter()` calls `ApplyHeadRotation()` after fresh physics to prepare `historicalHeadPending`. The first eligible draw consumes it in `DoHead()`, after draw-time eligibility/IK. Repeated/stereo draws do not reconvert targets or reapply the joint rotation; zero-physics frames retain the prior pose. This preserves the validated adaptation to Web execution rather than blindly copying the historical function. The character restorations are unconditional, with no character A/B definitions required.

**RUNTIME-VERIFIED (maintainer physical comparison):** bounded yaw improved turning, bounded tilt substantially improved locomotion presentation, and restored head tracking reproduced expected Classic look behavior. Forward movement continuously follows camera-forward as the camera rotates. Combined normal gameplay QA found no remaining perceptible player-locomotion problem. Classic digital/eight-direction intent remains unchanged; NPC steering generally produces continuous target headings and shares the character executor.

**VERIFIED SOURCE FINDING (NPC subdivision ordering):** in `Game::Tick()`, `doAerialAcrobatics()` executes character turning, `Person::DoStuff()` movement/terrain response, and object/contact correction before `Person::doAI()`. Corrected coordinates can therefore feed steering in the same subdivision, with the updated intent used by subsequent movement. Grounded velocity reconstruction, contact projection, path-arrival thresholds, and animation/state transitions are subdivision-sensitive; time-scaled terms alone do not make the whole update subdivision-invariant. Core `Person::doAI()`, `DoStuff()`, `DoAnimations()`, and `SphereCheck()` match current OSS `42f148d` ignoring comments/whitespace; surrounding scheduler and `Tick()` orchestration differ, including first-tick/full-frame collision-timer handling.

**OPEN (low-priority NPC slope/seam observation):** the default remains the fixed `1 ms` accumulator with its existing `20 ms` simulation cap. The completed physical A/B at approximately 60 Hz found no reliably distinguishable overall difference; a possible slight OSS-style slope/seam improvement did not establish a causal bug. Retain existing scheduling and NPC gameplay. This does not establish scheduler equivalence or OSS superiority. Temporary comparison/diagnostic source was removed; the scheduler patch remains external. See [B-13](porting-notes.md#scheduler-investigation-closed-low-priority-open-observation) for evidence and reopening criteria. Camera follow remains separate.

---

# 6. Skeleton and ragdoll

A dead or ragdolled character is still fundamentally represented through the existing `Person` + `Skeleton`/ragdoll machinery.

That is important because ragdolls are not merely visual death effects. Original Lugaru uses physics outcomes as gameplay.

## Corpse dragging seam

A plausible implementation does **not** require teleporting a corpse or replacing physics.

Conceptually:

1. player enters an interaction/drag state;
2. find a nearby eligible ragdolled/dead `Person`;
3. choose stable attachment joints/points;
4. constrain or drive those points relative to the player's hands/body;
5. allow the rest of the skeleton to remain physical;
6. release cleanly on input, interruption, or invalid state.

Potential systemic consequences already exist because corpses participate in perception logic.

### Ruleset decision

Corpse dragging should default **off** in Classic/historical gameplay because it changes the possibility space of stealth levels. It belongs naturally in Expanded.

---

# 7. Combat and semantic events

Lugaru's combat logic is deeply embedded in `Person` and related gameplay paths.

Future systems should avoid learning about combat by duplicating internal state checks everywhere.

A high-value seam is a semantic gameplay/combat event layer:

```text
Hit
Counter
Reversal
Disarm
Knockdown
Death
WeaponThrow
EnvironmentalKill
BodyDiscovered
PlayerSpotted
```

Consumers could include:

- Arena scoring;
- HUD/kill feed;
- challenge objectives;
- achievements;
- sound/music reactions;
- tutorials;
- replay metadata;
- analytics/debugging.

The event layer should report what happened; it should not become a second combat engine.

---

# 8. Weapons

Lugaru weapons already participate physically in the world. Historical community modding also pushed weapon/model replacement surprisingly far.

Expanded Lugaru-Web can eventually distinguish between:

- historical/classic weapon presentation;
- new weapon definitions;
- physical traits such as mass/reach/impact;
- rule-specific behavior.

Do not hardcode every new weapon with broad `if (arenaMode)` or species checks. Prefer data and capability-driven behavior when the feature actually arrives.

---

# 9. AI perception and stealth

Existing code inspection found multiple systemic perception channels already present:

- environmental sound with effective radius/volume;
- distance checks;
- rabbit hearing modifiers;
- posture / animation-height influence on visual detection;
- line-of-sight / occlusion behavior;
- wolf smell;
- bloody corpse / body smell stimuli.

This makes stealth one of the strongest places to extend the game without replacing its identity.

Potential Expanded additions that compose with existing systems:

- corpse/body discovery state;
- AI communication/shouting;
- investigate-last-heard-position;
- thrown/noise-maker distractions;
- explicit unconscious vs. dead handling;
- dragging bodies out of sight/smell paths.

These should be built as extensions of existing perception channels rather than a new stealth minigame.

---

# 10. Species and PersonType

The engine already has a `PersonType` abstraction for rabbit/wolf behavior and data.

It contains or influences properties such as:

- proportions/scale;
- animation resources;
- sounds;
- skeleton/model references;
- clothing model;
- skins;
- power;
- damage tolerance;
- claws;
- max run speed;
- knife-catching and related special behavior.

However, numerous direct rabbit/wolf checks remain in gameplay code.

### Practical progression

1. add fur/clothing variants;
2. add compatible-skeleton visual species/models;
3. generalize hardcoded species checks only when needed;
4. postpone unique species gameplay until the data/control architecture can support it cleanly.

For Arena, species should initially be identity/cosmetic rather than class balance.

---

# 11. Cosmetics and clothing

Existing `Person` state already includes clothing collections and RGB tint arrays. The clothing path composites clothing imagery/tints into a character skin.

That gives Lugaru-Web a surprisingly strong base for a future character creator:

- fur/skin choice;
- clothing overlays;
- clothing colors;
- masks/face overlays;
- eventually hats or separate geometry attachments.

This is cheaper than inventing a customization system from nothing.

Historical skin/decal mods should also be studied because they show what the old community wanted to customize.

---

# 12. Maps and levels

`Game::LoadJsonLevel()` reads structured map data including:

- map properties;
- environment/skybox data;
- dialogue;
- objects;
- hotspots;
- players;
- path points;
- world/level center/radius.

This gives Lugaru-Web a useful content seam without requiring an entirely new map format.

## Existing editor

Lugaru historically included an in-game debug map editor. The old documentation covers object placement, patrol paths, pathfinding points, weapons, character placement, dialogue/campaign tooling, and saving maps.

Therefore `CREATE` should not begin by writing a brand-new level editor from scratch.

A better path is:

1. make the existing editor usable/reachable in the browser;
2. stabilize save/load behavior under IDBFS;
3. expose a friendly create-new-map path;
4. add browser-oriented UI around the old editor;
5. later modernize the editor or add specialized Arena tooling as needs become clear.

---

# 13. Campaigns

Modern OSS Lugaru already supports multiple campaigns. Historical campaigns should be treated as first-class packages, not hacked into separate executables.

Potential campaign metadata should eventually include:

```text
id
title
author
year
license
historical/modern classification
recommended ruleset
thumbnail/art
progress
```

This allows the modern front end to expose provenance directly.

---

# 14. Tutorial system

The current tutorial has a recognizable subsystem with state such as:

- `active`;
- `stage`;
- `stagetime`;
- `maxtime`;
- `success`;
- tutorial update/draw/do-stuff methods.

There is a dedicated tutorial map and a flow roughly like:

```text
tutorial map
  → tutorial active
  → stage
  → instruction
  → observe player/game state
  → success
  → next stage
```

However, tutorial awareness leaks heavily into gameplay code, including many references from `Person.cpp`.

### Direction

Do not rewrite the original tutorial merely because this coupling is ugly.

Preserve the original tutorial for Classic/historical play.

If Expanded gains enough new mechanics, create a separate **Lugaru-Web Training** experience. New tutorial logic should preferably subscribe to semantic events (`PLAYER_JUMPED`, `BODY_GRABBED`, `COUNTER_PERFORMED`, etc.) rather than adding more stage-specific conditions throughout `Person.cpp`.

---

# 15. Menu/UI

The current menu is a legacy surface over increasingly modern needs.

Long-term modern UI requirements include:

- keyboard/controller navigation;
- rebindable controls;
- campaign browser;
- challenge browser;
- Arena/lobby UI;
- character customization;
- Create/editor entry;
- browser persistence visibility;
- fullscreen/widescreen/high-refresh settings;
- clear historical provenance.

A modern menu does not imply removing the old presentation. Classic frontend/presentation can remain a selectable or campaign-specific experience.

---

# 16. Account/persistence

Browser persistence now uses IDBFS rather than the earlier localStorage/base64 approach.

Current implementation goals:

- mount persistent path at `/home/web_user`;
- complete initial `syncfs(true)` before C++ execution;
- automatic persistence afterward;
- one-time migration from legacy localStorage only when an IDBFS version is absent;
- malformed legacy data should not destroy a valid store.

An observed behavior to revisit: creating an account alone may not immediately trigger a save; existing gameplay save triggers do persist state.

This may deserve its own future tracker item after confirmation.

---

# 17. Browser/platform layer

Known browser-specific concerns include:

- WebGL/legacy OpenGL translation through GL4ES;
- pointer-lock permission errors/noise;
- browser audio autoplay policy;
- canvas sizing and resolution lists;
- VSync/RAF vs. timeout scheduling;
- persistent filesystem startup ordering;
- no ASYNCIFY, meaning large synchronous loads can block the tab;
- case-sensitive asset behavior and incomplete fallback handling;
- current motion-blur disablement on Web.

The Web target should remain a platform implementation, not a place where gameplay semantics diverge unnecessarily.

**VERIFIED SOURCE FINDING / RUNTIME-VERIFIED:** `Source/Graphic/Sprite.cpp::Sprite::Draw()` now uses CPU billboards and client-array `glDrawArrays` submission on Web while retaining the historical immediate-mode path on native. The Web path keeps sprite order and gameplay particle behavior. This platform-specific renderer seam resolved the fire-heavy Seven Tasks task3 slowdown without changing fire spawning; [B-15](porting-notes.md#b-15-dense-fire-sprite-rendering-slows-web) records the measurements and preservation QA.

---

# 18. Ruleset seam

Lugaru-Web should use one engine and one WASM build.

The conceptual gameplay families are:

- `Classic`
- `Expanded`
- `Arena`

The exact API should be designed only after further code mapping. Avoid scattering hundreds of checks such as:

```cpp
if (classicMode) { ... }
```

A capability/rules structure is likely more maintainable, but should be introduced when concrete mechanics require it.

### Decision test

Ask:

> Does this materially change the decisions and possibilities available to someone playing historical content?

If **no** (bug fixes, controller support, persistence, high-refresh correctness, fullscreen, accessibility), it is probably universal.

If **yes** (corpse dragging, new movement abilities, new AI communication, fundamentally new weapons), it should default to Expanded/Arena.

---

# 19. Refactoring philosophy

The code is a "legible mess": old and coupled in places, but understandable enough to work with.

Avoid a giant cleanup rewrite before features.

Prefer:

1. map the behavior;
2. reproduce and test it;
3. identify the smallest seam a real feature needs;
4. extract that seam;
5. preserve behavior;
6. add the feature;
7. document the new boundary.

The Redagito repository is a useful reminder that a prettier directory tree does not automatically eliminate deep gameplay coupling.

---

# 20. Gameplay scripting and source archaeology (2026-09-23)

This section records the concrete machinery behind several behaviors that look like separate game systems while playing. The important architectural finding is that Lugaru has very little general-purpose scripting. Narrative progression, conversations, objectives, patrols, music state, and many apparent "scene transitions" are composed from a small set of old data structures and global state.

A useful current mental model is:

```text
Campaign text metadata
  → world-map/menu selection
  → LoadLevel(map)
  → map globals + Persons + Dialogues + Hotspots + path data
  → AI/combat/physics simulation
  → one of several win flags becomes true
  → Account progress / score update
  → ChooseNext policy
  → next map immediately OR world map OR stealth/in-place map swap
```

This is more accurate than imagining independent quest, cutscene, objective, transition, and world-map subsystems.

## 20.1 World map: a menu over fixed campaign metadata

**Code finding:** the campaign world map is not a playable map. It is menu case `5` in `Source/Menu/Menu.cpp`, backed by `Source/Level/Campaign.cpp` and a 400×400 world-map texture.

`CampaignLevel` stores:

```text
mapname
human-readable description
choosenext
nextlevel[]
location.x / location.y
width (default 10)
```

`LoadCampaign()` first looks for:

```text
Textures/<campaign>/World.png
```

and falls back to:

```text
Textures/World.png
```

Campaign `LocationX` / `LocationY` values are old 512×512 image-space coordinates. The current conversion is:

```cpp
x = 30 + 120 + location.x * 400 / 512;
y = 30 + 30 + (512 - location.y) * 400 / 512;
```

The historical Wolfire map-editing documentation independently describes these values as pixel coordinates from the top-left of `Data/Textures/World.png`.

Progress is owned by `Account`: campaign choices already made, cumulative campaign score, high score, and time fields. The menu shows the completed prefix plus the next available node(s), then selecting a node resolves the actual map through the previous level's `nextlevel` vector.

### Dormant branching architecture

The data model can hold multiple `nextlevel` values, but none of the four bundled campaigns currently use real branching: every non-final entry has `NumNext: 1` and every final entry has `NumNext: 0`.

The historical editor documentation explicitly says story branching was intended but not completed. The current map renderer also still assumes largely sequential campaign indices when drawing node lines (`campaignlevels[i - 1]` to `campaignlevels[i]`). A future branching campaign should therefore not assume that the old menu renderer is already a correct general graph renderer.

## 20.2 `ChooseNext` is more than presentation

Historical documentation gives the author-facing meanings:

```text
0 → load the next campaign level immediately
1 → return to the world map
2 → skip the normal fiery transition/loading presentation
```

The source reveals an important extra detail for value `2`.

In the campaign-completion path in `Source/GameTick.cpp`:

```cpp
stealthloading = (campaignlevels[actuallevel].choosenext == 2);
```

A stealth load then materially changes `LoadJsonLevel()` behavior:

- terrain decals and sprites are not cleared;
- existing terrain patch-object registrations remain;
- the new JSON object's list is **not** loaded;
- object center/radius/shadows are not rebuilt;
- player 0's current `coords`, `yaw`, and `targetyaw` are captured before Persons are recreated, then restored afterward;
- weapons and Persons are still rebuilt from the destination map;
- dialogues, hotspots, path points, environment, skybox values, and other map globals are replaced.

So `ChooseNext = 2` is best understood as an **in-place narrative scene swap**: preserve the visible world geometry and Turner's position/orientation while swapping much of the authored gameplay state around him.

In bundled campaigns, this mode occurs only once: **Original Lugaru `Lugaru1 → Lugaru2`**. That explains the unusually seamless opening-village continuation.

### Chained-map continuity patterns in Original Lugaru

A JSON comparison of important narrative pairs shows several different techniques rather than one universal transition system:

| Pair | World geometry | Gameplay-state change | Interpretation |
| --- | --- | --- | --- |
| `1 → 2` | 140 objects in each file; 122 identical at the same index, but destination objects are skipped during the actual stealth load | 7→2 Persons, 5→1 dialogues, 4→1 hotspots | true in-place scene swap; the `Lugaru2` object edits matter only if loaded non-stealth/directly |
| `2 → 3` | all 140 objects identical | 2→7 Persons, 1→6 dialogues | normal immediate reload of an identical copy of the village |
| `10 → 11` | all 95 objects identical | hostile `1→0`, dialogue appears | duplicated Raider Base used as a new narrative state |
| `17 → 18` | 152 objects; 143 same-index exact | hostile `0→1`, 6→2 Persons | near-copy with small authored changes |
| `19 → 20` | all 299 objects identical | dialogue/hotspot/person state changes | duplicated Rocky Hall state |
| `21 → 22` | 204→176 objects, no same-index object matches; grass→snow; center/radius/skybox change | different scene despite similar campaign label | same world-map description does **not** imply the same physical location |
| `23 → 24` | 163 objects; 154 same-index exact; skybox changes | 7→2 Persons | near-copy of the Wolf Den/Alpha Wolf area |
| `25 → 26` | 299 objects; 276 same-index exact | narrative state changes | near-copy epilogue state |
| `26 → 27` | all 299 objects identical | Person/dialogue/hotspot changes | duplicated final Rocky Hall state |

The resulting continuity trick is a mixture of true stealth loading, normal loads of exact duplicate maps, and normal loads of near-duplicate maps.

## 20.3 Win and lose conditions: several flags, one completion gate

`Source/Game.hpp` still defines four old map objective constants:

```text
mapkilleveryone = 0
mapgosomewhere = 1
mapkillsomeone = 2
mapkillmost = 3
```

The historical console guide exposes the same four concepts. Current source behavior is much narrower:

- **Kill everyone:** the ordinary `maptype == mapkilleveryone` path checks whether all non-player Persons are dead.
- **Go somewhere:** implemented in practice by a `Hotspot` with `type = -1`; entering its region sets `winhotspot`.
- **Kill selected Person(s):** implemented by hotspot types `11–19`; the type's last digit selects the `Person` index and `Hotspot::killhotspot` becomes true once every marked target is dead.
- **Win by conversation:** dialogue types `30–39` set `windialogue` when the conversation finishes.

The four bundled JSON maps whose serialized map `type` is `1` (`jendraz1`, `jendraz41`, `task11`, `task12`) also contain `type = -1` exit hotspots. No bundled JSON map uses map types `2` or `3`.

That makes `mapgosomewhere`, `mapkillsomeone`, and `mapkillmost` partly historical/descriptive API remnants. The active objective machinery is mostly hotspots, dialogue completion, and the all-dead check.

### Lose behavior

The ordinary fail state is much simpler: when player 0 dies, the current level is reloaded. There is no comparable generalized failure-condition data structure in current map JSON.

### Delayed completion: occupancy vs. recorded win

**VERIFIED SOURCE FINDING:** `winhotspot` is live exit-hotspot occupancy recomputed each tick, while `won` latches that a win was already recorded. The win (and, in campaigns, `winCampaignLevel()`) is recorded when the condition is first met, but routing waits for `changedelay` and re-checks the completion flags. Historically a player could leave a type `-1` exit during that delay and stall the transition. Since `97121db`, `acceptedSpatialWin` latches an accepted spatial win until `ResetBeforeLevelLoad()` for campaign and non-campaign maps alike.

In campaigns, `actuallevel` is the active node and `whichchoice` the incoming edge ordinal; `Account::getCampaignChoicesMade()` is only the count of saved choices. Campaign death reload now uses `campaignlevels[actuallevel]`, and a death after a recorded win but before routing finishes routes that win once. See [B-16](porting-notes.md#b-16-delayed-spatial-exit-loses-campaign-progression-empire-sky-palace).

## 20.4 Hotspots: tiny invisible script volumes

A `Hotspot` is only:

```text
position
size
type
text
```

but numeric `type` values make it act like a primitive script command.

### Current source semantics

| Type | Current behavior |
| --- | --- |
| `0` | static contextual text region |
| `1–10` | region follows `Person::players[type]`; displays contextual text |
| `11–19` | marks `Person::players[type - 10]` as a kill target for level completion |
| `-1` | entering region wins the level |
| `20–40` | while inside the region, launches `Dialog::dialogs[type - 20]` if it has not run |
| `-111` | no current JSON runtime behavior; legacy binary-loader demo marker/remnant |

Two architectural oddities matter:

1. **Dialogue-trigger hotspots live in rendering code.** `GameDraw.cpp` can mutate `Dialog::whichdialogue`, call `play()`, and increment `gonethrough` merely because the player is inside a region. Rendering therefore has a direct gameplay side effect.
2. **`size` is inconsistent with its own visualization.** Trigger tests compare squared distance (`distsq`) directly with `Hotspot::size`, so the effective trigger radius is `sqrt(size)`. The editor visualization, however, scatters particles out to approximately `size`. This should be tested before any editor UI labels `size` as a literal radius.

### Bundled-map hotspot census

Across all **137** bundled JSON maps:

| Type | Occurrences | Maps | Observed use |
| ---: | ---: | ---: | --- |
| `-111` | 41 | 41 | all are size `0`, text `mapname`; inert converted legacy records |
| `-1` | 20 | 11 | win/exit regions; some size-0 records are effectively disabled placeholders |
| `0` | 3 | 2 | contextual tutorial/story text |
| `1` | 1 | 1 | text region attached to Person 1 (`task7`) |
| `20` | 14 | 14 | launch dialogue index 0 |

No bundled JSON currently uses hotspot values `2–19` or `21–40`.

The old Wolfire console guide describes kill-hotspot values as `11–20`, while current source implements `>10 && <20` and reserves `20+` for dialogue launching. Treat this as historical/source-version drift, not something to silently normalize.

### `-111`: fossilized demo metadata

The legacy binary map loader declares:

```cpp
int indemo; // FIXME this should be removed
```

For older map versions it sets `indemo = 1` when a hotspot with type `-111` is encountered. The JSON loader does **not** special-case `-111`; converted maps simply retain those zero-sized `mapname` hotspots as inert records.

This is a useful warning for future schema cleanup: apparently nonsensical JSON entries can be artifacts of a conversion from old binary semantics rather than active content.

## 20.5 Dialogue is the game's cutscene system and a tiny script VM

`Source/Level/Dialog.cpp` plus the dialogue path in `Source/GameTick.cpp` implement much more than text.

A dialogue stores ten participant slots with authored position/yaw data. Each `DialogScene` contains speaker name, text, color, top/bottom location, sound id, focused participant, fixed facing, and authored camera position/yaw/pitch.

While a dialogue is active:

- ordinary control flow is suppressed;
- the camera comes from the current dialogue scene;
- attack/interact advances to the next scene after a short delay;
- scene sounds can play;
- the dialogue type can mutate hostility or win the level when it finishes.

### Participant binding is direct index binding

There is no independent cast/faction/entity binding layer. Participant ids ultimately address `Person::players[index]`.

`Dialog::play()` loops over dialogue **scenes**, takes each scene's `participantfocus`, and moves that Person to the stored participant position/yaw, zeroes velocity, and forces an idle animation.

Important consequence: **dialogue positions are not restored afterward.** The cutscene-authored position becomes the Person's live gameplay position. A participant slot that is never focused by any scene is not moved by this `play()` loop.

Historical dialogue documentation describes the editing-side version of the same idea: characters whose head direction was fixed while authoring became dialogue participants and were teleported to the recorded positions at dialogue start.

### Exact dialogue type semantics

`Dialog::tick()` calculates:

```cpp
playerId = type % 10;
special = type > 9;
```

The last digit therefore chooses the target Person for normal/proximity-triggered dialogues.

| Type family | Current source behavior |
| --- | --- |
| `1–9` | manual interaction near Person `type`; only while global `hostile == 0`; reusable |
| `11–19` | automatic one-shot proximity conversation with Person `type % 10`; non-hostile |
| `20–29` | one-shot proximity conversation; on completion sets global `hostile = 1` |
| `30–39` | one-shot proximity conversation; on completion sets `windialogue = true` and can win the level |
| `41–49` | one-shot conversation that bypasses both normal hostility and distance requirements |
| `50–59` | one-shot proximity conversation; on completion sets `hostile = 1` **and** forces every NPC into `attacktypecutoff` |

Because `41–49` bypass distance and `Dialog::tick()` iterates all dialogues every tick, a common `type = 41` dialogue behaves like an **automatic map-start cutscene** as long as Person 1 exists.

Types whose last digit is `0` do not pass `playerId > 0`, so they cannot start through ordinary `Dialog::tick()`. The only bundled example is `jendraz1` dialogue type `10`; that map has no dialogue-launch hotspot for it, making this specific dialogue appear to be unreachable/stale duplicate data in the current source path.

### Launch paths, missing targets, and interaction distance

**VERIFIED SOURCE FINDING (2026-10-06):** `Dialog::tick()` also requires `playerId < Person::players.size()`. A dialogue whose last digit names a nonexistent Person therefore never starts through ordinary ticking, and it does not read out of bounds. A type `20–40` hotspot (section 20.4) can still launch it: the hotspot path selects `Dialog::dialogs[type - 20]` directly and checks neither the dialogue's target digit nor `hostile`. The dialogue's own type still determines what happens when it finishes.

Bundled data relies on this. `svencouncil` (Temple) and `Lugaru13` / `Lugaru14` (Original) each pair a type-`35` dialogue, whose target Person 5 does not exist, with a type-`20` hotspot. The hotspot starts the conversation, and type `30–39` sets `windialogue` when it ends. All ten type-`30` dialogues (target digit `0`) are launched by type-`20` hotspots in the same way. A zero or missing target therefore does not by itself make a dialogue unreachable; check for a launching hotspot first. See [preservation 19](preservation-and-modding.md#19-changing-inherited-campaign-data-v10-campaign-qa-review) for how this affected the release data review.

Participant indices can also refer to helper Persons. Several Temple maps contain Persons with scale `0.0` or `20000.0`, and `svencouncil`'s council dialogue focuses its two scale-`0.0` Persons. These Persons count toward `Person::players` indexing like any other.

The proximity test was historically a fixed `distsq < 6` between character centres. Since `d997ab3`, when the target's scale is between `0.001` and `10`, the squared threshold becomes the larger of `6` and `1.5 × ((player scale + target scale) × 2.5)²`. Default-sized pairs keep `6`, and helper-scale targets are excluded. In bundled data, the only dialogue target this enlarges is the `2.0`-scale giant in Seven Tasks `task9`. See [porting notes](porting-notes.md#long-ten-giant-encounter-seven-tasks-task9-d997ab3).

### Bundled-dialogue census

Across all bundled JSON maps, the used dialogue types are:

```text
1:7   2:7   3:7   4:7   5:7
10:1
11:16 12:2  13:1  14:1
21:12 23:1  25:1  26:2
30:10 31:13 32:13 34:4 35:3 36:3
41:23
51:2
```

No other type values are used.

*Later change (2026-10-06):* the census above records the 2026-09-23 data: 143 dialogues and 862 scenes. Release campaign QA then removed 32 stale dialogue objects from Temple maps and Seven Tasks `task9`, leaving 111 dialogues and 668 scenes:

```text
1:3   2:5   3:4   4:4   5:4
10:1
11:6  12:2  13:1  14:1
21:12 23:1  25:1  26:2
30:10 31:13 32:7  34:3  35:3  36:3
41:23
51:2
```

Every type value listed above is still used. The scene-level observations below (no `-5`/`-6` sounds, `participantaction = 0`) still hold for the reduced set.

### Scene-level script hooks

The dialogue-advance code also contains two special negative sound values:

```text
sound == -5 → create a type -1 win hotspot at player position, size 10
sound == -6 → set hostile = 1
```

None of the **862** bundled JSON dialogue scenes currently uses `-5` or `-6`; all observed scene sounds are non-negative. These are dormant historical script hooks worth preserving in compatibility notes.

Likewise, every bundled scene serializes `participantaction = 0`, even though the field remains in the binary/JSON format. Its original intended semantics remain an open archaeology question.

### `#` padding hack

`GameDraw.cpp` strips `#` characters from displayed dialogue text/name strings. Historical Wolfire documentation explains why: old dialogue strings of exactly 10 or 13 characters could crash the map, so authors appended `#` as invisible padding. This is not decorative garbage; it is compatibility archaeology.

Historical documentation also records a 128-character dialogue-text limit.

## 20.6 Text that looks related is implemented by unrelated systems

Several player-visible text surfaces have different owners:

- **Dialogue speaker names/text:** `DialogScene` data rendered in dialogue screen-space UI.
- **Hotspot contextual messages:** `Hotspot::text`, centered near the bottom of the screen when inside eligible regions.
- **Tutorial instructions:** hardcoded/stage-driven strings in `Source/Tutorial.cpp` / tutorial drawing paths.
- **Score/bonus/award text:** separate HUD paths in `GameDraw.cpp` and `Level/Awards.cpp`.

There is no general serialized `Person.name` system for normal overhead NPC names in the current map schema. A name seen in a conversation belongs to the scene, not to a reusable named-NPC entity component.

## 20.7 Hostility and AI: global permission plus per-Person states, not factions

The classic game has no general faction/team id in `Person` or map JSON.

The main pieces are:

```text
global hostile flag
+ each Person's aitype
+ hardwired player 0 target assumptions
+ a temporary ally index used by get-help/get-weapon behavior
```

`hostile = 1` does **not** by itself mean that every NPC is already chasing Turner. It permits passive AI perception/damage reactions to transition into combat. A map can therefore start hostile while enemies remain on patrol until they hear/see/smell the player.

Dialogue type families make an important distinction:

- `20–29` only flip global `hostile`, allowing normal perception to take over;
- `50–59` flip `hostile` **and** set every NPC directly to attack state.

### Live and fossil AI constants

`Person.hpp` defines:

```text
passivetype       0
 guardtype        1
 searchtype       2
 attacktype       3
 attacktypecutoff 4
 playercontrolled 5
 gethelptype      6
 getweapontype    7
 pathfindtype     8
```

In the current source tree, `guardtype` and `attacktype` are defined but have no other references. The live chase/combat state is the historically odd `attacktypecutoff = 4`. These two unused values should be treated as provenance questions, not repurposed casually.

Map JSON also does not serialize `aitype`; NPCs construct as passive and gameplay state evolves at runtime.

## 20.8 Patrol routes and pathfinding are two different authoring systems

Per-Person patrol data consists of an ordered waypoint list plus a waypoint type:

```text
wppause = 1
wpkeepwalking = 0
```

Passive active NPCs with multiple waypoints walk the list cyclically. A pause waypoint produces a four-second pause.

The map-global `pathpoints[]` graph is separate. It is used by `pathfindtype` to navigate around obstacles and return an AI toward a final target, often its authored patrol position after losing track of the player.

Historical editor documentation matches this separation: Shift-P creates character patrol points, while Control-P creates navigation/pathfinding points around difficult geometry.

Bundled JSON census:

- 224 Person records contain patrol waypoint lists;
- 1,144 `keepwalking` waypoint records;
- 169 `pause` waypoint records;
- patrols range from 2 to 20 points in the bundled corpus;
- only 24 of 137 maps contain any global pathfinding graph; 113 have none.

So the normal map-authoring style is simple authored patrols, with explicit navigation graphs only where geometry needs help.

## 20.9 AI perception: sight, sound, bodies, smell, search, and primitive communication

`Person::doAI()` already contains more systemic stealth behavior than a simple aggro radius.

### Hearing

Gameplay actions create entries in global environmental-sound arrays. Passive/searching NPCs can react when a sound falls inside their threshold. Sleeping characters subtract 14 from perceived sound volume. Rabbits receive a large hearing multiplier in the threshold expression.

### Sight and posture

There are several layers:

- very close/high-posture checks can trigger attack quickly;
- longer-range sight uses facing-dot-product tests and head-to-head object collision checks;
- low-height/crouched player animations reduce the rate at which `lastseentime` is driven toward detection;
- once suspicion crosses the threshold, AI enters `searchtype` with a remembered `lastseen` position rather than always teleporting directly into perfect knowledge.

There is no dedicated player-facing stealth meter. The information is embodied in NPC behavior, surprise states, combat music, and awards/stat counters.

### Ragdoll/body visual detection already exists

The passive sight loop does not inspect only player 0. It also considers Persons whose `skeleton.free` is true or whose AI is already non-passive. A visible ragdolled/dead/unconscious Person can therefore become the location that drives another NPC into `searchtype`.

That means "body discovery" is not wholly absent. There is already a primitive generic ragdoll/body noticing path; a future Expanded body-discovery feature should first separate and understand this existing behavior rather than adding a parallel system.

### Wolf smell

Wolves can react to:

- Turner within the smell threshold;
- Turner carrying a bloody weapon, which increases the threshold;
- dead bloodied Persons, with a larger threshold.

The sampled smell point is offset slightly by normalized `windvector`, so environment wind participates mechanically, not only visually.

### Search / escape

A searching NPC moves toward `lastseen`, perturbs the location while checking, listens for new sounds, and can reacquire Turner. When `lastseentime` expires, `numescaped` is incremented and the AI enters `pathfindtype` to return toward its authored waypoint.

This statistic feeds the `Escape Artist` end-of-level award.

### Get help

A sufficiently injured rabbit can enter `gethelptype`, seek the nearest passive ally, and on reaching that ally put both characters into `searchtype` while copying last-seen/check timing. This is a small but real AI-to-AI communication mechanic already present in classic Lugaru.

### Rendering/AI occlusion coupling

A major architectural wart remains: `Person::occluded` is updated in `GameDraw.cpp` using collision tests from the **camera/viewer** to the NPC, primarily for render culling. `Person::doAI()` then uses the same `occluded` field as a gate before portions of sight/perception logic.

The AI does still perform its own NPC-head-to-target-head collision check, but it is preceded by this render-owned state. Therefore camera/render visibility can influence whether some AI perception work runs at all.

This must be behavior-tested before being "cleaned up"; it may be accidental optimization coupling, historical gameplay behavior, or both.

## 20.10 Environment is a three-way global rules/presentation switch

The serialized `environment` value is one of:

```text
0 = snow
1 = grass
2 = desert
```

`Setenvironment()` switches whole groups of assets/state at once:

- terrain textures;
- tree/bush/rock/box textures;
- footstep sounds;
- skybox image set;
- ambient stream;
- wind vector.

Snow uses wind and snow-specific surfaces; desert uses desert ambience; grass uses wind. Snow rendering also emits precipitation effects.

Across 137 bundled maps:

```text
grass  64
snow   50
desert 23
```

Per-map data additionally carries `viewdistance`, `fadestart`, whether the skybox texture is enabled, skybox tint RGB, and skybox-light RGB. This is how near-identical maps can change mood without a generalized weather/time-of-day scripting system.

## 20.11 Music is driven by environment and AI state

There is no per-map music-track field in current JSON.

`GameTick.cpp` selects the base level theme from `environment`:

```text
snow → Music1Snow
 grass → Music1Grass
 desert → Music1Desert
```

It switches to the fight theme if any living NPC is in:

```text
attacktypecutoff
getweapontype
gethelptype
searchtype
```

with exclusions for several sneak-attacked animations. Entering fight music emits the alarm sound. A one-second `unseendelay` keeps the fight theme from dropping instantly when the threat condition disappears. Player death and loading can switch to the menu theme.

This means music is a live readout of **AI threat state**, not a dialogue or map-script cue. Even an NPC running for help or searching for the player counts as a musical threat.

## 20.12 Slow motion is a simulation modifier, render effect, and audio-pitch state

Manual debug slow motion toggles global `slomo`; auto slow motion sets it for roughly `0.2` seconds on certain near-lethal finishing impacts and on deaths near the viewer/player.

While active:

- `Source/main.cpp` multiplies gameplay frame time by `slomospeed` (default initialized to `0.25`);
- `GameDraw.cpp` selects motion-blur presentation where available, suppresses camera shake, and counts down `slomodelay`;
- the OpenAL wrapper lowers source pitch using `slomofreq / 44100` (default `8012 / 44100`).

Historical player documentation also notes automatic kill slow motion. This is a cross-system cinematic effect, not a camera cutscene script.

## 20.13 Ragdolls, falling, and world bounds

### Fall damage is collision-physics damage

Fast downward movement (`velocity.y < -30` in the jump-down path) can force a ragdoll. Once ragdolled, `Skeleton::DoConstraints()` derives impact damage from collision bounce/impulse against terrain and objects. `Person` then applies the resulting damage after armor/protection scaling.

There is therefore no simple "fall N meters = X damage" table. The actual landing contact and physical impulse matter, preserving the original hard/soft-surface character of Lugaru's physics.

A living ragdolled player can recover under specific movement/clearance/velocity conditions; ragdoll is not synonymous with death.

### Runtime out-of-bounds clamp is not `mapradius`

`mapcenter/mapradius` are loaded/saved and used to draw/edit a circular boundary in editor mode. No ordinary level-completion transition is attached to leaving that radius.

Separately, Person movement clamps X/Z coordinates against a circle centered on the terrain and sized approximately:

```text
terrain half-width - viewdistance
```

That hidden clamp is the actual ordinary world-edge containment behavior. It is distinct from authored exit hotspots and from editor `mapradius`.

## 20.14 Blood has gameplay meaning

Blood exists on several layers:

- `bloodloss`, bleeding/death-bleeding Person state;
- body/skin blood effects;
- weapon `bloody` / drip state;
- terrain/object blood decals and particles.

Mechanically:

- heavy blood loss can push injured rabbits into get-help/attack behavior;
- wolves can smell dead bloodied bodies;
- a bloody weapon carried by Turner increases wolf smell sensitivity to him.

So blood is not merely an optional decal system. Visual settings can suppress parts of its presentation, but underlying injury/smell state participates in gameplay.

## 20.15 Fire is stateful and partially propagating, not a general fire simulation

`firetype` objects start with `onfire = true`. Persons can `CatchFire()`, accumulate burn effects, emit flame sprites, and can extinguish themselves through the rolling path.

Collision/proximity code explicitly propagates fire in both directions between a Person and `firetype`/`bushtype` objects:

- burning Person → ignites nearby fire/bush object;
- burning fire/bush object → ignites nearby Person.

No equivalent general object-to-object propagation path was found in this pass. Weapons contain an `onfire` field/render path, but no non-constructor assignment to weapon `onfire` was found in the current tree, suggesting dormant/incomplete functionality rather than a usable burning-weapon system.

Document new fire features as extensions, not as restoration of a broad existing propagation engine.

## 20.16 Weapons are physical runtime objects but map persistence is Person-owned

A runtime `Weapon` has owner id, position/tip position, velocity, damage, blood state, and collision state. `owner == -1` denotes a free/ground weapon.

Classic interactions include:

- held/sheath ownership;
- disarming/dropping;
- thrown physics;
- picking up ground weapons;
- AI seeking ownerless weapons via `getweapontype`;
- weapons sticking in bodies;
- extracting a stuck weapon from a victim, including blood consequences;
- dropping held weapons on death.

Map JSON, however, persists weapon types **inside each serialized Person**. There is no root-level list of arbitrary ground weapons. Historical editor documentation says the same thing from the user's perspective: weapons left on the ground disappear after save/reload.

This is an important Create/editor constraint. Supporting intentionally placed persistent loose weapons would require a schema/runtime extension rather than just exposing an existing field.

## 20.17 Scoring: event bonuses, diminishing repeats, time, and derived awards

`Source/Level/Bonuses.def` defines named point events such as Spinecrusher, Ninja Bonus, Nice Aim, Reversal, Bullseye, Wolf Slayer, SLAIN, and Death from Above.

`award_bonus()` only accepts `playerid == 0`. When the same bonus is repeated, the per-bonus `bonusnum[]` divisor increases. Most bonuses increment the divisor by `1`; the Solid Hit/combo family increments by `0.15`. The awarded value is then divided by this counter before adding to `bonustotal`, producing diminishing repeated rewards.

The tutorial explicitly zeros bonus values.

### Challenge progression

Challenge completion stores:

```text
score = bonustotal - startbonustotal
time = leveltime
```

and `Account::winLevel()` updates high score, fastest time, and unlocked progress.

The menu exposes **14** challenge levels, while `LoadLevel(int)` still contains a loader range for indices `0..15` (`map1` through `map16`). Only `map1.json` through `map14.json` are bundled, leaving two dormant loader slots.

### Campaign progression

Campaign completion calls:

```cpp
Account::active().winCampaignLevel(whichchoice, bonustotal, leveltime);
```

which appends the selected branch choice and adds the supplied score to campaign score/high score.

The campaign `time` field is currently assigned the most recent level's `time`, not accumulated. `setCampaignFinalTime()` exists and can maintain a campaign fast-time record, but no caller was found in the current source. The campaign-end menu displays campaign score/high score, not campaign total time.

Treat full-campaign timing as dormant/incomplete architecture unless historical behavior proves otherwise.

### End-of-level awards

`award_awards()` derives award labels from accumulated run statistics rather than map-authored data. Current source conditions include:

- Flawless: no damage taken and no player blood loss;
- Take no prisoners: every NPC `dead == 2`;
- Merciful: every NPC `dead == 1`;
- One with the shadows: `numresponded == 0` and no throw kill;
- weapon mastery based on all attacks being staff/sword/knife;
- Unarmed when all attacks were unarmed, no throw kill, and weapons exist in the level;
- Escape artist if `numescaped > 0`;
- Gymnast from enough successful flips with no flip failures;
- Blade slinger if thrown-weapon kills equal enemy count;
- Brutal if there were after-kill attacks and everyone is fully dead;
- Aikido master if reversals exceed 80% of attacks and exceed three;
- Divide and conquer if `maxalarmed == 1` with more than two Persons;
- Suicidal if `numflipfail > 3`.

Several names remain in `Awards.def` without a corresponding assignment in `award_awards()` (`Coward`, `Hyper`, `Rambo`, and the three speed awards). These appear to be dormant/dead award definitions in the current implementation.

Historical Wiki descriptions are useful design context but are not always identical to current code conditions.

## 20.18 Challenge maps are ordinary maps with separate account/menu plumbing

Challenge maps do not use a fundamentally different level format or simulation. They are ordinary `mapN` files loaded through the same map loader and played through the same Persons, AI, weapons, hotspots, scoring, and physics.

The distinctive layer is around them:

- numeric loader naming;
- separate challenge progress/highscore/fasttime Account arrays;
- challenge menu and unlock gating;
- non-campaign completion path.

This supports a future challenge browser without requiring a challenge-specific gameplay engine.

## 20.19 Tutorial is an embedded special ruleset, not just stage text

The existing architecture document previously described the stage subsystem. Source archaeology shows that `Tutorial::active` reaches much deeper.

Representative tutorial-specific behavior includes:

- `Person::DoDamage()` does not accumulate normal `damage`, `permanentdamage`, or `superpermanentdamage` while the tutorial is active;
- bonus values are forced to zero;
- normal AI anger/hearing/sight transitions are gated through `cananger`;
- tutorial stages directly toggle `cananger` and NPC AI states;
- many blood/decal/impact/speech effects for NPCs are suppressed;
- tutorial NPCs use a translucent/special texture presentation and can be highlighted red during attack behavior;
- Skeleton collision effects/sounds are selectively suppressed for non-player tutorial Persons;
- several attack/weapon/combat branches explicitly test tutorial stage/state;
- ordinary level-change completion is gated while the tutorial is active.

The result is effectively a special Classic tutorial gameplay ruleset distributed across `Tutorial.cpp`, `GameTick.cpp`, `Person.cpp`, `Skeleton.cpp`, `Weapons.cpp`, `Terrain.cpp`, and `GameDraw.cpp`.

This is a strong argument for **preserving** the historical tutorial rather than attempting an early cleanup. A future Lugaru-Web training mode can be cleaner without making the old tutorial a refactor prerequisite.

## 20.20 Legacy binary-map remnants that survive in current JSON/code

The current JSON schema is easier to inspect, but it remains a converted descendant of the binary format rather than a clean-sheet content model.

Verified remnants include:

- `-111` demo-marker hotspots preserved as inert size-0 `mapname` records;
- numeric dialogue type ranges acting as script opcodes;
- negative dialogue sound ids `-5`/`-6` reserved as gameplay hooks but unused by bundled JSON;
- `participantaction` serialized on every dialogue scene but `0` across the bundled corpus;
- map objective constants for modes no bundled map actively implements through `maptype`;
- unused `guardtype` and `attacktype` AI constants;
- campaign branching vectors even though bundled campaigns never branch;
- two extra challenge loader slots without bundled map files;
- world-map descriptions encoded with underscores in text files and converted to spaces when loaded;
- Person-owned weapon persistence with loose-ground weapons discarded by save/load;
- historical dialogue `#` padding preserved because the renderer still strips hashes.

Do not delete these merely because they look redundant. First classify whether each is:

```text
active compatibility behavior
converted historical data
unused but historically meaningful API
or genuinely unreachable dead code
```

## 20.21 Architectural consequences for Lugaru-Web

The archaeology reinforces the project's feature-driven-refactoring rule.

High-value seams are now more concrete:

1. **Objective/completion events** can report existing all-dead, hotspot, dialogue, and target-kill completion without replacing them.
2. **Dialogue/cutscene API** can eventually wrap current index-based Persons and camera scenes while Classic maps continue to execute unchanged.
3. **AI perception events** should be extracted only after testing the camera-occlusion coupling and existing ragdoll/body noticing path.
4. **World-map/campaign UI** can modernize presentation while retaining legacy `main.txt` semantics and provenance.
5. **Create mode** should expose existing patrol/path/hotspot/dialog structures honestly; persistent loose weapon placement would be a real extension.
6. **Expanded objectives** can build on semantic events instead of adding more magic type ranges to hotspots/dialogues.
7. **Classic compatibility tests** should include quirks that look accidental but can affect authored content: dialogue positioning, hotspot squared-distance behavior, AI/render occlusion, stealth loading, and tutorial gates.

The goal is not to make the old implementation look modern. It is to put a modern seam around behavior that has first been understood and regression-tested.

## 20.22 Historical corroboration used in this pass

Useful surviving Wolfire documentation:

- Map editor / campaign format / patrol vs. pathfinding / loose weapon persistence: `https://wiki.wolfire.com/Map_Editing_in_Lugaru`
- Console map objectives / hotspot types / viewdistance / fadestart: `https://wiki.wolfire.com/Lugaru%27s_Console_Guide`
- Dialogue authoring and participant teleport behavior: `https://wiki.wolfire.com/Dialogues`
- Dialogue 10/13-character crash and `#` workaround: `https://wiki.wolfire.com/Lugaru_FAQ`
- Player-facing slow-motion behavior: `https://wiki.wolfire.com/Lugaru_Character_Movement`
- Historical scoring techniques: `https://wiki.wolfire.com/Lugaru_Combat_Strategy`
- Historical challenge award descriptions: `https://wiki.wolfire.com/Lugaru_Walkthrough`

Where historical documentation and current source disagree, this document records the current source behavior explicitly and treats the older text as historical evidence rather than silently reconciling the two.
