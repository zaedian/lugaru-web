# Lugaru-Web Design Direction

## Product identity

Lugaru-Web should be more than a museum port, but preservation remains a first-class principle.

A concise product idea is:

> **Lugaru in your browser: the complete original game preserved and modernized, plus a new browser-native platform built around its combat.**

The strongest long-term extension is a frictionless multiplayer Arena mode inspired by the accessibility and replay loop of games such as STRAFTAT, while keeping Lugaru's combat, physics, movement, and animal-world identity at the center.

---

# 1. One engine, multiple experiences

Do **not** create separate Classic and Expanded engine forks.

The project should aim for:

```text
one codebase
one browser build
one platform/input/persistence layer
        ↓
multiple content packages
multiple gameplay rulesets
multiple presentation paths
```

Historical content and modern content should coexist without forcing either to compromise.

---

# 2. Preservation dimensions should remain separable

Three concepts should not be conflated:

### Content
Which campaign/map is being played?

Examples:

- Original Lugaru
- Temple
- Empire
- Seven Tasks
- historical community maps
- future Lugaru-Web campaign/content

### Gameplay rules
Which mechanics are available?

Conceptual rulesets:

- **Classic**: historical gameplay behavior except for correctness/platform fixes.
- **Expanded**: new single-player/co-op mechanics and AI/system extensions.
- **Arena**: multiplayer-focused rules/scoring/round behavior built on the same core systems.

### Presentation
Which UI/visual presentation is being used?

Examples:

- preserved classic title/menu presentation;
- modern Lugaru-Web frontend;
- classic vs. refreshed visual/audio presentation where feasible.

This separation allows a modern frontend to launch Original Lugaru under Classic rules without pretending the original interface never existed.

---

# 3. Historical defaults

Historical campaigns should default to historical gameplay rules:

- Original Lugaru → Classic
- Temple → Classic
- Empire → Classic
- Seven Tasks → Classic
- recovered historical community campaigns/maps → Classic unless strong evidence says otherwise

A user may later be allowed to replay old content under Expanded rules, with a clear warning that this can change intended balance, stealth solutions, traversal, and challenge behavior.

Possible UI wording:

```text
Gameplay
  Original
  Expanded
```

Expanded-on-historical-content is a bonus mode, not the historical default.

---

# 4. Modern platform improvements are universal

The following should not be treated as gameplay cheats or Expanded mechanics:

- controller support;
- key/controller rebinding;
- analog movement;
- stick deadzones/sensitivity;
- mouse-button rebinding;
- high-refresh correctness;
- widescreen/fullscreen;
- browser persistence;
- accessibility improvements;
- crash/UB fixes;
- correct timing/interpolation.

These improve how the player communicates with the same game.

---

# 5. Expanded gameplay philosophy

New mechanics should earn their place by interacting with existing systems.

Good additions create multiple consequences rather than existing as isolated gimmicks.

Examples:

## Corpse dragging

Potential interactions:

- move a body out of enemy line of sight;
- move a bloody corpse away from wolf smell paths;
- create or remove physical obstacles;
- expose the player while dragging;
- alter stealth routes without scripting a special corpse-dragging puzzle.

## AI body discovery / communication

Potential interactions:

- discovered body changes alert state;
- rabbit/wolf reactions differ through existing senses;
- an alerted enemy can communicate danger to nearby allies;
- thrown objects or environmental noises can redirect investigation.

## Environmental/noise tools

A throwable object is useful if it participates in:

- physics;
- sound propagation;
- AI investigation;
- damage/knockdown when appropriate;
- player improvisation.

The target is more systemic possibility, not a checklist of "modern game features."

---

# 6. Arena

The strongest original Lugaru-Web mode is a low-friction multiplayer Arena/party fighter.

Initial target:

- 1v1 as the simplest useful network case;
- grow toward 2–4 players;
- short rounds;
- compact maps;
- simple room/lobby flow;
- random or rotating arenas/weapons;
- generous cosmetic selection;
- Lugaru combat as the core skill system.

The inspiration is the *frictionless loop*, not copying another game's combat or presentation.

A player should be able to:

```text
open URL
→ Play
→ Arena
→ join/create room
→ choose character look
→ fight
```

without installing a client or managing mods.

---

# 7. Multiplayer technical order

Do not start by bolting networking onto current global input.

Recommended progression:

1. semantic input actions;
2. controlled-person abstraction;
3. two local human players;
4. per-player UI/camera/state;
5. deterministic-enough replication experiments;
6. host-authoritative networking;
7. lobby/room-code UX;
8. 2–4 player scaling.

WebRTC data channels are a plausible browser-native transport, but signaling and authority architecture should be decided only after local multiplayer establishes the game-state boundaries.

---

# 8. Cosmetics and identity

Arena customization should be broad and player-friendly rather than monetization-driven.

Potential dimensions:

- species/model;
- fur/skin;
- clothing;
- clothing colors;
- hats;
- masks;
- player name;
- name color;
- later progression unlocks for special items.

Existing clothing/tint systems make this more feasible than it first appears.

Customization can carry into Expanded/co-op content. Original campaign content remains Turner-focused.

---

# 9. Species

Potential future species include historically relevant and new options such as rabbits, wolves, cats, rats, dogs, foxes, and others.

The initial rule should be:

> Species is identity first, class second.

New species should initially be mechanically equivalent where practical. This avoids adding another generation of rabbit/wolf-specific conditionals throughout legacy combat code.

Only introduce unique species mechanics once the engine has a clean capability/data model for them.

---

# 10. UI/UX direction

The modern Lugaru-Web frontend should feel like a game, not a generic web dashboard.

Conceptual top-level navigation:

```text
LUGARU

CAMPAIGNS
CHALLENGES
ARENA
CUSTOMIZE
CREATE
OPTIONS
```

Art direction should retain Lugaru's hostility and roughness:

- gothic / cathedral-like visual language;
- stone, ink, scratches, worn materials;
- dangerous-animal imagery;
- low-resolution/pixel-ish score/counter details where appropriate;
- modern navigation and accessibility beneath the aesthetic.

Avoid generic SaaS panels.

### Campaign browser

Campaign cards can make historical provenance visible:

```text
Original Lugaru (Wolfire, 2005)
Temple (Silb, 2006)
Empire (Jendraz, 2008)
The Seven Tasks (Philtron R., 2009)
...
```

Long-term card art should ideally use purpose-made illustrations/renders rather than generic screenshots. A useful reference is illustrated fantasy loading-screen art rather than literal map thumbnails.

---

# 11. Preserved classic frontend

The old title/menu experience is itself part of the game's history.

One possible structure:

```text
modern Lugaru-Web frontend
        ↓
Original Lugaru campaign
        ↓
optional transition into preserved classic title/menu/tutorial/challenge presentation
```

A modern direct `Continue` path could still exist later.

Do not force a final UX decision before the frontend architecture is understood.

---

# 12. Visual/audio refresh

A refresh should be conservative:

- correct modern rendering behavior;
- optional higher-quality textures/effects;
- improved sound quality where source/rights permit;
- modern widescreen/fullscreen/high-refresh support;
- preserve original art direction;
- avoid blanket AI upscaling or smoothing that erases texture character.

Classic presentation should remain available where feasible.

---

# 13. Challenges, leaderboards, and speedrunning

Lugaru already has challenge scoring/timing concepts, making browser leaderboards a natural long-term extension.

Local score/time tracking is straightforward.

Global leaderboards require backend infrastructure and anti-cheat expectations should be realistic: an open-source browser client cannot be treated as trusted.

Possible progression:

1. local PBs;
2. community/unverified global boards;
3. build/version metadata;
4. input/replay evidence if a replay system becomes viable.

Leaderboard identity must include gameplay ruleset. Expanded movement or mechanics should not silently compete against Classic runs.

A run identity might eventually include:

```text
challenge/campaign
ruleset
game build/version
time
score
category
```

Timed campaign runs could later support automatic splits and PB comparison.

---

# 14. Replays

A replay system could become unusually valuable:

- challenge verification;
- watching world-record runs;
- ghosts;
- Arena replays;
- bug reproduction;
- regression testing.

However, old physics/timing may make deterministic input-only replay difficult. A hybrid system with periodic state snapshots may be necessary.

Do not promise deterministic replay until experiments prove it.

---

# 15. Tutorial/training

Preserve Rosen-era/Original tutorial behavior under Classic.

Do not rewrite the historical tutorial merely to teach Expanded mechanics.

If Expanded adds enough new verbs, add a separate **Lugaru-Web Training** package that teaches semantic mechanics through the new event layer.

---

# 16. Create mode

Lugaru's existing map editor is part of the heritage and should be leveraged.

Long-term browser flow could become:

```text
CREATE
→ New Arena / New Map
→ place geometry / spawns / weapons / path points
→ save locally
→ playtest
→ export/share package
```

The first milestone is not a fancy editor. It is making the existing editor reliably usable under the browser filesystem.

---

# 17. Historical community content

Historical maps/campaigns are not merely bonus levels. They are part of Lugaru's community history.

The long-term experience could include a clearly labeled **Historical Community** challenge/campaign section, with author/year/provenance/license metadata.

This should remain distinct from new Lugaru-Web content.

---

# 18. Co-op

Co-op remains appealing but optional.

Possible forms:

- historical campaign co-op experimentation;
- purpose-built Expanded co-op missions;
- PvE Arena/survival modes.

Do not let co-op requirements distort the first Arena implementation. The architecture work needed for multiple human-controlled Persons is useful either way.

---

# 19. Overgrowth references

Lugaru-Web can acknowledge Overgrowth as the sequel/design descendant without copying proprietary assets/code.

Potential approaches:

- small tasteful easter eggs;
- historically inspired species or ideas;
- mechanics that revisit documented abandoned Lugaru 2 concepts;
- explicit historical notes in documentation.

The goal is lineage awareness, not imitation.

---

# 20. Decision rule for scope

For every proposed feature, ask:

1. Does it preserve or clarify Original behavior?
2. Does it add a systemic verb rather than a one-off scripted gimmick?
3. Can it live behind a ruleset/capability instead of contaminating historical behavior?
4. Does it create useful seams for more than one future system?
5. Is it appropriate now, or should the browser foundation be stabilized first?

The project should remain ambitious without turning the current stabilization phase into an uncontrolled feature rewrite.

---

# 21. Objectives, campaigns, and Create after the 2026-09-23 archaeology pass

The source/data archaeology changes how several future-facing systems should be designed.

## Keep legacy scripting intact under Classic

Classic content should continue to execute its existing authored mechanisms:

- map all-dead checks;
- exit/target/text/dialogue hotspots;
- numeric dialogue behavior;
- direct Person-index dialogue participants;
- `ChooseNext` campaign behavior, including stealth/in-place transitions;
- historical patrol/pathfinding data;
- tutorial-specific rules.

Do not convert old maps to a new objective language merely to make the engine internally prettier.

## New content should not extend the magic-number pile

Expanded and future Lugaru-Web content should prefer explicit semantic definitions over inventing new ranges such as `Hotspot type 60–69` or `Dialog type 70–79`.

Conceptually, new authoring can expose names such as:

```text
Objective.EnterRegion
Objective.DefeatTargets
Objective.Survive
Objective.Escape
Dialogue.OnEnterRegion
Dialogue.OnInteract
Dialogue.OnCompleteSetHostile
```

while the compatibility layer continues interpreting historical numeric fields unchanged.

This gives modern tools a readable schema without forcing a migration of historical content.

## Modern world-map UI can be presentation over old campaign data

The classic campaign world is fundamentally metadata plus a background image, coordinates, progress, and next-level records. A new campaign browser/map can therefore modernize navigation, provenance, thumbnails, and accessibility without changing Classic level progression.

For newly authored campaigns, a modern graph/layout representation can be introduced separately if branching is actually needed.

## Create should expose the real old systems before replacing them

A first useful browser Create/editor experience should let authors inspect and manipulate the systems the engine actually uses:

- Persons and owned weapons;
- patrol waypoint loops/pause nodes;
- global pathfinding graph points/links;
- dialogues, scenes, participants, and camera poses;
- hotspots and their effective behavior;
- environment/skybox/view-distance settings;
- campaign metadata/world-map coordinates.

It should also make historical limitations visible. For example, persistent loose weapon placement is **not** merely a missing editor button; the current map schema does not persist arbitrary root-level ground weapons.

## Build Expanded stealth on what already exists

The archaeology strengthens the case for systemic stealth additions rather than a parallel stealth layer. Existing AI already has hearing, visual posture/range/occlusion logic, search/last-seen state, primitive visible-body discovery, rabbit help behavior, and wolf blood/corpse smell.

Features such as corpse dragging, distractions, clearer body discovery, and AI communication should compose with those channels. Classic should retain the historical behavior, including quirks, until regression tests prove a safe seam.
