# WebAssembly Porting Notes

**Last updated:** 2026-10-13

This document tracks WebAssembly-specific defects, inherited behavior changes, browser compatibility issues, and related implementation notes for Lugaru-Web.

`PROJECT.md` contains the high-level issue summary. This file contains the technical detail needed to investigate, test, and resolve each item.

Issue IDs remain stable once assigned. Current IDs run from B-1 to B-20, plus CI-1.

**v1.0.0 status:** every issue marked **Fixed** below is committed on the development line; fixing commits are recorded in each entry. B-8 (controller support) is partially complete, and B-9, B-10, B-11 and CI-1 remain open. Shared gameplay and campaign-data fixes found during release campaign QA are recorded without new IDs under [Release campaign QA fixes](#release-campaign-qa-fixes-2026-10-05--2026-10-06). Dated validation notes inside each entry are preserved as historical evidence of what was tested at that time, including the local build directories used then. The current release build procedure is in [`build-provenance.md`](build-provenance.md#current-v10-release-build).

---

## Status Definitions

* **Open**: confirmed or strongly supported by available evidence and still unresolved.
* **Investigating**: actively being reproduced, tested, or traced.
* **Fixed**: corrected in the development branch.
* **Deferred**: understood but intentionally postponed.
* **Unable to reproduce**: previously observed but not currently reproducible.

An issue should move to **Investigating** when active work begins rather than jumping directly from Open to Fixed.

## Evidence Definitions

Each issue should identify the strongest evidence currently available:

* **Source inspection**: behavior is supported by code inspection but has not necessarily been reproduced at runtime.
* **Runtime observation**: behavior has been directly observed in a running local build.
* **Source inspection + runtime observation**: both code and observed behavior support the issue.
* **CI observation**: behavior has been directly observed in an automated build or test environment.

Source-derived issues should not be described as runtime-confirmed until they have actually been reproduced.

---

## B-1: Web Settings Overridden After Configuration Load

**Status:** Fixed
**Priority:** High
**Fix:** `abb81d7` (`fix: respect web settings and initialize loop timing`; together with B-2)
**Evidence:** Source inspection + browser runtime validation

### Location

```text
Source/User/Settings.cpp
Source/main.cpp
LoadSettings()
DefaultSettings()
Web resolution enumeration
```

### Previous Behavior

The inherited WebAssembly build applied browser-specific settings after normal configuration loading.

The Web-only block in `LoadSettings()` forced behavior equivalent to:

```cpp
#ifdef __EMSCRIPTEN__
    ismotionblur = 0;
    alwaysblur = 0;
    disablevsync = 1;
    set_vsync(0);
    kContextWidth = 1920;
    kContextHeight = 1080;
    newscreenwidth = 1920;
    newscreenheight = 1080;
#endif
```

Because this occurred after configuration parsing, previously saved VSync and resolution values were overwritten every time settings were loaded.

The Web build also received only one resolution from SDL/Emscripten display-mode enumeration:

```text
1920 x 1080
```

As a result, the resolution option appeared in the menu but could not cycle to another value.

### Resolution

The Web-specific post-load overrides for VSync and resolution were removed.

The Web build still disables motion blur after configuration loading because that rendering feature is unsupported on Web:

```cpp
#ifdef __EMSCRIPTEN__
    // Motion blur is unsupported in the Web build.
    ismotionblur = 0;
    alwaysblur = 0;
#endif
```

Browser-specific values are now treated as defaults rather than forced settings.

`DefaultSettings()` continues to use:

```text
1920 x 1080
```

as the default Web render resolution, but a saved user-selected resolution is no longer overwritten during configuration loading.

In the tested Web build, SDL/Emscripten exposed only the current canvas size through display-mode enumeration, so the Web build now provides a small set of internal 16:9 render resolutions:

```text
1280 x 720
1600 x 900
1920 x 1080
2560 x 1440
```

Native builds continue to populate the resolution list from SDL display modes.

### Validation

Browser runtime testing confirmed:

- VSync can be changed between On and Off,
- the selected VSync setting survives a reload,
- the Resolution option cycles through all four Web render resolutions,
- the selected resolution survives a reload,
- selecting `2560 x 1440` results in a canvas drawing buffer of `2560 x 1440`,
- selecting `1280 x 720` results in a canvas drawing buffer of `1280 x 720`,
- the visible canvas continues filling the browser viewport because `shell.html` scales it independently through CSS.

The Web build therefore retains 1920×1080 as its default while allowing users to choose and persist another supported internal render resolution.

### Impact

Browser configuration now behaves like normal user configuration rather than being silently overwritten during every load.

Saved VSync and resolution preferences are respected, while unsupported motion blur remains disabled intentionally.

---

## B-2: Browser Main Loop Uses Timeout Scheduling Instead of Animation Frames

**Status:** Fixed
**Priority:** High
**Fix:** `abb81d7` (`fix: respect web settings and initialize loop timing`; together with B-1)
**Evidence:** Source inspection + browser runtime validation
**Related:** B-1

### Location

```text
Source/Game.hpp
Source/main.cpp
set_vsync()
emscripten_main_loop()
```

### Previous Behavior

The inherited Web build forced:

```text
disablevsync = 1
```

through B-1, causing:

```cpp
set_vsync(false);
```

to select Emscripten zero-delay timeout scheduling rather than browser animation-frame scheduling.

An additional initialization-order bug caused the Web startup path to execute:

```cpp
g_emscriptenMainLoopRunning = true;
set_vsync(!disablevsync);
emscripten_set_main_loop(emscripten_main_loop, 0, 1);
```

This marked the loop as running and attempted to change its timing before `emscripten_set_main_loop()` had actually created the loop.

Chrome consequently reported:

```text
emscripten_set_main_loop_timing: Cannot set timing mode for main loop since a main loop does not exist! Call emscripten_set_main_loop first to set one up.
```

### Resolution

The Emscripten main loop is now registered without attempting to configure its timing first:

```cpp
emscripten_set_main_loop(emscripten_main_loop, 0, 1);
```

Timing initialization is deferred until the first actual execution of `emscripten_main_loop()`:

```cpp
if (!g_emscriptenMainLoopRunning) {
    g_emscriptenMainLoopRunning = true;
    set_vsync(!disablevsync);
}
```

At that point the Emscripten loop exists, so `set_vsync()` can safely configure its timing.

B-1 was fixed at the same time so the browser no longer forces VSync off after loading configuration.

With VSync enabled, the Web build now uses:

```text
EM_TIMING_RAF
```

and follows the browser's animation-frame scheduling.

If the user explicitly disables VSync, the existing timeout-based scheduling behavior remains available intentionally.

### Validation

After the initialization-order change:

- the previous `emscripten_set_main_loop_timing` startup warning no longer appears,
- the game starts and runs normally,
- VSync can be changed from the settings menu,
- the VSync preference survives browser reloads,
- the default Web configuration can use browser animation-frame scheduling instead of being forced to timeout scheduling.

### Impact

The Web build no longer attempts to configure main-loop timing before the loop exists and is no longer forced into timeout scheduling by its settings loader.

Browser frame scheduling now follows the user's VSync preference, with animation-frame scheduling available as the normal default behavior.

---

## B-3: Browser Save Persistence Can Silently Discard Progress

**Status:** Fixed
**Priority:** High
**Fix:** `3d747d3` (`fix: migrate web persistence to IDBFS`)
**Evidence:** Source inspection and browser runtime testing

### Location

Primary browser-persistence code is located in:

```text
Misc/web-persistence.js
Source/User/Account.cpp
Source/User/Settings.cpp
```

Relevant browser storage behavior includes the IDBFS persistence layer and legacy `localStorage` migration path.

### Current Behavior

Browser persistence uses Emscripten IDBFS backed by IndexedDB.

The IDBFS filesystem is mounted at `/home/web_user` and loaded before the game starts. Files written beneath that mount are persisted automatically.

Legacy browser saves stored in `localStorage` are migrated to IDBFS only when no IndexedDB save already exists. Legacy account data is Base64 decoded during migration, and invalid legacy data is left untouched rather than deleted.

Existing IndexedDB data takes precedence over legacy `localStorage` data.

### Resolution

The previous `localStorage` save/restore implementation was removed.

The replacement persistence layer:

- mounts IDBFS before game startup,
- waits for the initial IndexedDB sync before allowing the game to run,
- automatically persists filesystem writes,
- migrates existing legacy config and account data when appropriate,
- preserves malformed legacy data instead of silently deleting it,
- avoids overwriting an existing IndexedDB save during migration.

Runtime testing confirmed that account progress persists across page reloads and that legacy `localStorage` saves migrate successfully to IndexedDB.

### Implementation Notes

Several details of the Web persistence implementation are intentional and should be preserved unless the surrounding build configuration is changed.

- `Module['preRun']` uses bracket notation intentionally. With the current `--closure 1` build, using `Module.preRun` allowed Closure Compiler to rename the property while the Emscripten runtime continued looking for `preRun`, preventing the persistence startup hook from running.
- `Misc/web-persistence.js` is registered as a `LINK_DEPENDS` dependency of the `lugaru` target. Without this dependency, editing the pre-JS file did not reliably cause CMake to relink the generated Web application.
- The initial `FS.syncfs(true)` is protected by an Emscripten run dependency. This ensures that IndexedDB contents are loaded into the mounted filesystem before the C++ game startup code accesses configuration or account files.
- Existing IndexedDB files take precedence over legacy `localStorage` data. Migration only occurs when the corresponding IDBFS file does not already exist.
- Invalid or malformed legacy account data is intentionally left untouched. The previous implementation could delete the stored value after failed validation, which made recovery impossible.

### Validation

The replacement persistence system was manually tested using isolated browser origins.

Testing confirmed that:

- account progress stored through IDBFS survives a page reload,
- an existing IndexedDB save is not overwritten by legacy `localStorage`,
- a valid legacy `localStorage` save migrates successfully when no IndexedDB save exists,
- migrated account data loads normally after migration,
- malformed legacy data is not silently deleted,
- the persistence initialization completes before normal game startup.

Disposable localhost ports were used during migration testing so the primary development origin and its existing browser data were not modified.

### Future Save UX

B-3 addresses the browser persistence backend and destructive legacy-save behavior. It does not change the game's higher-level save cadence.

Account state is still written according to the game's existing save triggers rather than after every relevant player action. During testing, creating an account alone was not sufficient to persist it; normal gameplay later triggered `Account::saveFile()`.

This behavior is currently acceptable, but save timing should be revisited during a future browser UI/UX pass. Possible improvements include saving at additional safe transition points or exposing clearer save-state feedback without changing the underlying IDBFS persistence model.

---

## B-4: Animation access lost bounds checking

**Status:** Fixed
**Priority:** High
**Fix:** `b222ddc` (`fix: restore animation frame bounds checking`)
**Evidence:** Source inspection + native build + WebAssembly build + browser runtime comparison

**Location:** `Source/Objects/Person.hpp`

**Behavior:**
The inherited WebAssembly commit replaced checked `std::vector::at()` access with unchecked `operator[]` access in `currentFrame()` and `targetFrame()`.

**Resolution:**
Restored `.at()` for both animation and frame lookup, matching upstream checked-access semantics.

**Verification:**
- Native Release build completed successfully with GCC 15.2.0.
- WebAssembly Release build completed successfully with Emscripten 6.0.9.
- Browser gameplay remained functional with the change applied.
- A later `RuntimeError: memory access out of bounds` was reproduced on the untouched baseline with this patch stashed, confirming that crash is not introduced by B-4.
- Native runtime interaction could not be fully smoke-tested because the WSLg window would not accept input/focus.

**Introduced by:** inherited WebAssembly commit `b340204`

---

## B-5: Asset Filename Case Fallback Is Incomplete

**Status:** Investigation closed; compatibility note retained
**Priority:** Low
**Classification:** Latent compatibility risk, not a current runtime bug
**Evidence:** Source/history inspection, tracked-reference/package audit, isolated filesystem harnesses

### Verified Coverage

**VERIFIED SOURCE FINDING:** `Source/Utils/Folders.cpp::findCaseInsensitiveFile()` tries the exact path first. Its fallback is Emscripten-only and is called only through `openMandatoryFile()`. It scans the requested parent directory for a basename match without case; it does not repair parent-directory case. `getResourcePath()` only prepends the data directory.

Models, skeletons, and animations use `openMandatoryFile()`. Images (`load_image()`), audio opens, campaign/dialogue streams, JSON map loading, and `file_exists()` do not use the fallback. Binary map loading also performs an exact-case existence check before reaching `openMandatoryFile()`. These uncovered paths can matter for external/custom/historical content with incorrect casing. Current OSS `42f148d` does not provide broader case-insensitive handling; no missing upstream fix was identified.

### Content and Packaging Audit

**VERIFIED SOURCE FINDING / static audit:** current tracked Lugaru-Web content has zero confirmed case-only asset-reference mismatches and no duplicate tracked filenames differing only by case. Audited references include source literals, animation/sound definitions, campaign map names, all 1,874 JSON clothing paths, and generated tutorial/configured challenge names. Optional absent campaign world textures use the existing default-world fallback.

**VERIFIED SOURCE FINDING / package audit (2026-09-27):** the inspected local Web package contains exactly the 542 tracked asset filenames, preserving case without aliases. CMake preloads `Data` at `/Data`; packaging does not mask a detected mismatch. CDN selection changes the aggregate data-file URL, not internal lookup semantics. Remote CDN contents were not independently validated.

*Later changes (2026-10-04):* the tracked `Data/` count is now 544. The two additions are `Data/Textures/Fur.png` (B-20, `85477f5`) and `Data/Textures/FurBW.jpg` (Temple FurBW restoration, `fef0364`); both are exact-case references from their maps. CDN data loading was removed in `36407bd`, so the Web release always fetches `lugaru.data` from the same origin; the remote-CDN caveat above no longer applies to current builds. The package audit itself was not re-run.

**RUNTIME-VERIFIED (isolated harnesses):** native Linux and Emscripten builds of the actual `Folders.cpp`, tested with the unchanged tracked `World.png`, confirmed exact-case success, raw wrong-case lookup failure, Web-only basename recovery through `openMandatoryFile()`, and failure to repair parent-directory case. The Emscripten harness ran under Node; this was not a full browser gameplay test. No shipped-content runtime failure was reproduced.

### Decision

Close the active B-5 investigation as a low-priority compatibility note. No source or data change is justified. Prefer exact-case references; do not expand filesystem fallback without a demonstrated compatibility need. Reopen only if a concrete custom/historical-content failure is reproduced. Dynamic user-entered paths and external content remain outside the exhaustive static audit.

---

## B-6: Motion Blur Option Is Exposed but Unsupported on Web

**Status:** Fixed; investigation closed
**Priority:** Low
**Fix:** `fad80ac` (`fix: disable unsupported blur option on Web`)
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED experimental rendering; final menu guard source-validated

### Findings

**VERIFIED SOURCE FINDING:** historical blur remains implemented for native builds in `GameDraw.cpp`. It uses framebuffer-to-texture copies and textured-quad compositing, with native front-buffer behavior for slow-motion/velocity blur; it does not use an accumulation buffer. The Web path excludes front-buffer selection and the ordinary two-frame blur selector. GL4ES implements copy/compositing operations but stubs `glDrawBuffer()` / `glReadBuffer()`. These findings do not establish that WebGL fundamentally cannot implement motion blur.

`Settings.cpp` defaults Web blur off and forces `ismotionblur = 0` / `alwaysblur = 0` after settings loading. Previously, Options could toggle the live `ismotionblur` value and save it, despite the reload override. The control was therefore misleading, not a universal no-op: some effect paths remained reachable during that session.

**RUNTIME-VERIFIED (maintainer physical A/B):** the experimental Web ON build reproduced visibly malformed slow-motion rendering: an apparent resolution/viewport change and a washed-out gray/white composite. Enabling the historical effect as-is is unacceptable. The precise rendering fault was not isolated further; no unsupported-feature or crash claim is inferred from these symptoms.

### Resolution and Preservation Decision

Keep historical motion blur disabled on Web. Exact renderer restoration was judged disproportionate to the feature's value for this milestone. No renderer redesign or modern replacement effect was introduced; native behavior remains unchanged.

`Menu::updateSettingsMenu()` now displays `Blur Effects: Unavailable on Web`. The existing row/index is retained, and the shared activation dispatcher ignores the blur toggle under Emscripten. Mouse and controller confirmation therefore cannot enable it. Native builds retain the original label and toggle. Neighboring options, cursor hit-testing, keyboard Escape, contextual controller Back, and Options Back/save retain their existing paths.

### Validation and Cleanup

Temporary experiment definitions and BLUR diagnostics were removed; `Source/User/Settings.cpp` and `Source/GameDraw.cpp` were restored to the committed baseline. The only final source change is the Web-only menu label/activation guard in `Source/Menu/Menu.cpp`. Final validation covers the normal Release Web build, `git diff --check`, native preprocessor equivalence, and focused Web dispatch checks. Physical confirmation of the final unavailable row remains a short manual UI check; the malformed experimental rendering itself was physically verified.

---

## B-7: Browser Key Rebinding Failed at SDL Thread Creation

**Status:** Fixed
**Priority:** Medium
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED (Chrome, 2026-09-24)
**Fix:** `a7bfd51` (`fix: restore web key rebinding`)
**Verified:** 2026-09-24

### Root Cause and Previous Behavior

`Source/Menu/Menu.cpp::setKeySelected()` attempted `SDL_CreateThread()` with the Web build's non-threaded SDL backend. Chrome reproduced the error `Unable to create thread: Threads are not supported on this platform`. Failure cleared `waiting` but left `keyselect` set: the control displayed `_`, replacement keys did nothing, and another control could not be selected. Escape behaved normally; refreshing restored the original binding display, so the failed capture state was not persisted.

### Resolution

Web-only nonblocking SDL capture replaces thread creation, with menu/input/main-loop guards against reusing captured input for navigation or hotkeys. Keyboard scancodes, mouse-button encoding, control ordering, Escape cancellation, and settings/save behavior are preserved. Capture clears both `waiting` and `keyselect`; native threaded capture is intentionally unchanged. Pthreads and Asyncify remain disabled.

### Validation

The Web target built successfully after retrying an Emscripten cache-lock sandbox failure with cache access; the successful build log contained no compiler/linker warnings.

Chrome runtime testing confirmed normal capture, disappearance of the thread error, keyboard and mouse rebinding, clean Escape cancellation, subsequent control selection, rebound actions in gameplay, and persistence after leaving Options and reloading. Held-hotkey, focus-transition, and browser-reserved-key coverage remains unverified.

Native SDL event-thread portability remains a separate open question in the investigation log (A4a).

---

## B-8: Controller Gameplay, Menu/UI, and Tutorial Support

**Status:** Milestones 1–5 implemented, Web-built, and physically runtime-verified (Firefox + wired DualSense, DS4Windows disabled). Tutorial additions are source/automated-validated; physical verification is pending. Compatibility, tuning, and scope exclusions remain open.
**Priority:** Medium
**Evidence:** Committed source, reported build/check results, isolated native SDL harness, and manual physical-controller runtime testing

### Implementation: Milestone 1 (2026-09-25)

**VERIFIED SOURCE FINDING:** Commit `e5939fe` (`feat: add controller input foundation`) on `investigate/controller-support` adds the foundation in `Source/Utils/Input.cpp`, `Source/Utils/Input.hpp`, and `Source/main.cpp`.

- `Input::InitController()` initializes `SDL_INIT_GAMECONTROLLER` and immediately checks for recognized controllers already available at startup.
- `Input::UpdateController()` calls `SDL_GameControllerUpdate()` and, while no controller is active, retries discovery at one-second intervals. One `SDL_GameController*` is active at a time.
- Device indices are used during enumeration/opening only. The active device is retained by its handle and `controllerInstance` (`SDL_JoystickID`), with attachment checked through `SDL_GameControllerGetAttached()`. Reconnection enumerates again rather than relying on a stale device index.
- Disconnect closes the handle and calls `clearControllerState()` to reset cached axes/buttons. `Input::ShutdownController()` closes the device, clears state, and releases the subsystem before `SDL_Quit()`.
- `Input::hasController()`, `Input::controllerAxis()`, and `Input::controllerButtonDown()` expose standardized SDL axes/buttons through the existing input area. Raw controller state remains separate from `keyDown`, `keyPressed`, and keyboard/mouse bindings.
- `SetUp()`, `DoUpdate()`, and `CleanUp()` own the lifecycle calls. No additional SDL event-queue consumer was introduced.

At milestone 1, no controller gameplay, camera, menu, or rebinding behavior was mapped. Subsequent milestones below add gameplay, look, UI, and tutorial support while preserving keyboard/mouse input; this remains narrower than a generalized input-system rewrite.

### Milestone 1 build and isolated harness validation

**Build validation (reported for the source milestone):** `cmake --build build-web -j2` succeeded, and `git diff --check` passed before the source commit.

**RUNTIME-VERIFIED (isolated native SDL virtual-controller harness):** no-controller startup, startup discovery, hot-plug, button/axis state reads, device-index changes, disconnect/reset, reconnect, and shutdown. This was not a full native Lugaru gameplay test.

### Milestone 1 physical runtime verification

**RUNTIME-VERIFIED (manual test):** Sony DualSense / PS5 controller, Windows host, Firefox, Lugaru-Web served from localhost, DS4Windows disabled, wired USB connection. Lugaru logged:

```text
Controller: detected device 0 (Standard Gamepad)
Controller: opened instance 0 (Standard Gamepad)
```

- Left-stick analog values were received across the SDL signed axis range, including intermediate values and small near-center values. The observed center noise informed the movement and look deadzones added below.
- Right-stick `rightx` and `righty` produced analog values in both positive and negative ranges.
- `lefttrigger` and `righttrigger` were observed as binary Web-axis values (`0` and `32767`), consistent with the previously identified bundled SDL Emscripten backend behavior; analog trigger precision is not established by this test.
- Shoulder and stick-click buttons produced clean down/up events. Additional controller buttons were exercised, without claiming exhaustive button coverage.
- Physical disconnect and reconnect were detected; the controller reopened and resumed reporting input. No stuck controller state was observed.
- Existing mouse controls still worked. At milestone 1, controller input remained intentionally unmapped to gameplay/menu behavior.

### Milestone 2: Digital Classic movement (`4b8af1d`)

**VERIFIED SOURCE FINDING:** `Input::controllerMovement()` in `Source/Utils/Input.cpp` exposes left-stick directions using independent axis hysteresis: activate at magnitude `10000`, release at `7000`. `clearControllerState()` clears the direction latches on disconnect/reset. In `Game::Tick()` (`Source/GameTick.cpp`), controller directions are ORed with keyboard bindings into `Person::players[0]->forwardkeydown`, `backkeydown`, `leftkeydown`, and `rightkeydown`, under existing gameplay/dialogue guards.

Existing Classic speed, facing, animation, and diagonal logic remain authoritative. There is no proportional analog movement: the stick selects the existing digital/eight-direction combinations. Snapping movement/facing into those combinations is expected, not a milestone 2 defect. True analog movement/arbitrary facing is separate future work because it affects animation, facing, combat, and timing.

**Build validation:** `cmake --build build-web -j2` and `git diff --check` passed.

**RUNTIME-VERIFIED (isolated native SDL harness):** thresholds, hysteresis, diagonals, reversals, keyboard-state preservation, disconnect/reconnect, and lifecycle handling passed. This is not full native Lugaru gameplay validation.

**RUNTIME-VERIFIED (physical manual test):** wired DualSense, Firefox, DS4Windows disabled. Movement worked correctly: center noise caused no unintended movement; threshold/hysteresis start and stop behavior and diagonals worked; keyboard/controller movement coexisted; disconnect/reconnect left no stale movement. Mouse controls remained functional.

### Milestone 3: Core gameplay actions (`2387ebf`)

**VERIFIED SOURCE FINDING:** fixed mappings in `Source/Utils/Input.cpp`:

| Physical control | SDL GameController button | Existing action |
| --- | --- | --- |
| Cross / bottom face | `SDL_CONTROLLER_BUTTON_A` | Jump |
| Circle / right face | `SDL_CONTROLLER_BUTTON_B` | Crouch |
| Square / left face | `SDL_CONTROLLER_BUTTON_X` | Draw / sheath |
| L1 | `SDL_CONTROLLER_BUTTON_LEFTSHOULDER` | Throw / pickup / contextual weapon interaction |
| R1 | `SDL_CONTROLLER_BUTTON_RIGHTSHOULDER` | Attack |

`isJumpDown()`, `isCrouchDown()`, `isDrawDown()`, `isThrowDown()`, and `isAttackDown()` combine controller and existing keyboard/mouse held state. Releasing either source does not release an action still held by the other. Existing jump/crouch transitions and draw/throw toggle logic remain authoritative, including contextual sneak/roll/reversal and weapon behavior.

`doAttacks()` consumes combined held attack while retaining `oldattackkey` logic. `Input::Tick()` derives `isAttackPressed()` from the combined transition for one input/physics tick. Both `Dialog::tick()` in `Source/Level/Dialog.cpp` (initiation) and `Game::Tick()` (advancement) consume this press edge, rather than relying only on `Person::attackkeydown`. Jump-based respawn uses combined jump input. Disconnect removes controller-derived action state without clearing keyboard/mouse state; combined attack history stays separate from controller lifecycle so a keyboard-held attack does not acquire a false new edge. Raw developer/text/key handling remains separate. Developer dialogue-camera controls remain keyboard-only. Challenge-completion confirmation was keyboard-only at milestone 3; milestone 5 adds controller continue/back through the existing result handlers.

**Build validation:** Web build and `git diff --check` passed.

**RUNTIME-VERIFIED (isolated SDL harness):** all five mappings, keyboard/controller handoffs, one-tick attack edges, raw-input isolation, unmapped controls, and disconnect/reconnect passed.

**RUNTIME-VERIFIED (physical Firefox/DualSense test):** all five mappings worked: Cross jump, Circle crouch and related existing contexts, Square draw/sheath, L1 throw/pickup, R1 combat and dialogue interaction/advancement. Held attack did not cause unintended repeated dialogue advancement. Keyboard/mouse and controller actions coexisted, and disconnect/reconnect left no stale action state.

At milestone 3, right stick, Triangle, triggers, Options/Start, D-pad, and other unused controls remained unmapped. Milestone 4 adds right-stick look; milestone 5 adds Start/D-pad UI behavior; the tutorial follow-up maps Triangle only to tutorial advance. Triggers remain unmapped. The face-button layout is intentional and does not copy the historical Switch layout.

### Milestone 4: Right-stick camera/look (`ab46b53`)

**VERIFIED SOURCE FINDING:** `Input::controllerLook()` uses `controllerLookAxis()` with an axial deadzone of `8000` (approximately 24%). Remaining travel is continuously rescaled from zero to full response, avoiding a speed jump at the boundary. `Game::TickOnce()` adds the resulting angular rate alongside mouse input, outside menus and dialogue/directing, before the existing pitch clamp:

- Maximum yaw rate: `120°/s`; maximum pitch rate: `90°/s`.
- Right increases yaw; down increases pitch, matching non-inverted mouse behavior.
- Pitch limits remain `[-70°, 90°]`.
- Controller scaling is independent of mouse sensitivity and inversion. Mouse and stick look combine without exclusive device selection.
- Attachment-checked axis reads remove controller-derived look on disconnect; centered reconnect has no retained look state. Triggers are not used for camera rotation.

**VERIFIED SOURCE FINDING (timing):** stick input represents angular velocity, not mouse displacement. The implementation integrates `look rate × realmultiplier` once per outer frame in `TickOnce()`, before physics substeps. `DoUpdate()` in `Source/main.cpp` sets `realmultiplier` from elapsed frame seconds before game-speed, difficulty, loading, and slow-motion modifiers. Sustained deflection therefore yields approximately the same angular rotation per real-world second across render rates. The existing 250 ms stall cap still applies; long stalls are not integrated as unrestricted elapsed time. A fixed increment per rendered frame would be refresh-rate dependent in this loop.

**Build validation:** Web build and `git diff --check` passed. The Web milestones required Emscripten cache access for linking; existing unused-variable warnings in `main.cpp` and `GameDraw.cpp` were not controller failures.

**RUNTIME-VERIFIED (isolated timing/camera harness):** 30, 60, 120, 144, and 240 Hz plus irregular timing; deadzone, signs, pitch limits, mouse combination/inversion, camera guards, disconnect/reconnect, and unmapped triggers passed. Milestone 3 action harness regression checks also passed. These are isolated checks, not physical browser measurements across those refresh rates or full native gameplay tests.

**RUNTIME-VERIFIED (physical Firefox/DualSense test):** camera control worked and felt mechanically correct. Centered stick and inside-deadzone movement caused no drift; horizontal/vertical look and diagonal/circular motion were smooth; pitch limits held. Mouse look remained functional and coexisted with controller look. Disconnect/reconnect left no stale camera movement. Left-stick movement and all milestone 3 buttons still worked; triggers and other intentionally unmapped controls had no gameplay effect.

**OPEN QUESTION (tuning):** physical testing found camera sensitivity somewhat slow for preference, not mechanically incorrect. Values remain `120°/s` yaw and `90°/s` pitch. Future tuning and controller-specific sensitivity configuration remain open.

### Milestone 5: Pause and virtual-cursor UI (`8d549e4`)

**VERIFIED SOURCE FINDING:** controller UI reuses the existing pointer-oriented menu in `Source/Menu/Menu.cpp`, with no discrete-focus system or controller-specific campaign navigation:

- Options / `SDL_CONTROLLER_BUTTON_START` joins the existing pause/resume transitions in `Game::ProcessInput()`, including music/audio behavior. It opens Pause from gameplay and resumes from the actual pause menu (`mainmenu == 2`); it is not a universal Back button in nested menus.
- Cross / SDL A confirms the item selected by existing cursor hit-testing. The menu-local press joins the left-click activation dispatcher without changing `Input::MouseClicked()`, synthesizing SDL mouse events, or overwriting raw mouse buttons.
- Circle / SDL B chooses the existing contextual Back/Cancel/No/Resume action: Pause resumes; Options saves on Back; Controls returns to Options; Stereo applies its pending mode on Back; Delete chooses No; Accounts, Challenges, and campaign ending use their existing destinations. Circle does nothing at the root without an active game or on Difficulty (screen 8).
- Campaign cycling and world-map marker activation retain existing availability/progress checks and loading behavior. Challenge results separately accept Cross for existing continue and Circle for existing back behavior, preserving bookkeeping and destination handling.
- Square, Triangle, shoulders, triggers, and right stick have no menu action. Stereo decrement remains mouse-only. Controller confirm cannot start key rebinding. Account names require keyboard text entry; Circle can cancel the existing name-entry path and Web key capture. Native threaded key capture is unchanged.

`Input::MoveControllerMenuCursor(realmultiplier)` runs in `DoUpdate()` after `DoMouse()`, once per outer frame rather than once per `Menu::Tick()`. It uses the existing continuous `8000` axial-deadzone response on the left stick, with maximum speed **320 logical pixels/second**. D-pad takes precedence for precision motion at **80 logical pixels/second**; diagonals have bounded speed. The logical **640×480** motion is scaled into the existing shared `mousecoordh`/`mousecoordv`, retaining fractional motion and clamping to screen bounds. Mouse and controller therefore move the same visible cursor. Elapsed-time integration provides approximately consistent speed across render rates, subject to the existing stall cap.

Controller press edges and `syncControllerContext()` consume held buttons through release across menu/gameplay/modal transitions. Cross used for Resume cannot immediately Jump, Circle cannot become Crouch, and held gameplay buttons cannot activate a newly entered UI context. Disconnect clears cached controller input, pending UI presses, suppression state, and cursor remainder without clearing keyboard/mouse state; centered/released reconnect has no stale movement or activation.

**Build validation:** Web build and `git diff --check` passed.

**RUNTIME-VERIFIED (isolated SDL/input harnesses):** cursor scaling, deadzone, 30/60/120/144/240 Hz and irregular timing, D-pad precision/diagonals, boundary clamping and slow movement away from edges; confirm edges; contextual Back destinations; pause/audio transitions; challenge results; modal cancellation; release suppression; raw-input isolation; disconnect/reconnect. Gameplay-action handoff and held-button regression checks also passed. These are not full native gameplay tests or physical browser measurements at those refresh rates.

**RUNTIME-VERIFIED (maintainer QA):** Firefox, wired DualSense, DS4Windows disabled. Milestone 5 QA passed: pause/resume, virtual cursor, confirmation/contextual Back, campaign switching, world-map interaction, and challenge/menu flows. Earlier movement, actions/dialogue, camera, simultaneous keyboard/mouse use, and disconnect/reconnect remain physically verified. This report does not extend physical verification to the subsequent tutorial changes.

### Tutorial compatibility: source and automated validation; physical QA pending

**VERIFIED SOURCE FINDING:** `bd7b207` (`feat: add controller tutorial advance`) adds `Input::controllerTutorialAdvancePressed()` for Triangle / `SDL_CONTROLLER_BUTTON_Y`. It reuses controller press edges, transition suppression, and lifecycle reset, restricted to active tutorial gameplay outside menus, modal/text capture, console, freeze, and results. It joins the existing Tab handler in `Game::ProcessInput()` without global Tab emulation: if stage is not `51`, set `Tutorial::stagetime = Tutorial::maxtime`; preserve the existing sound. Stage 51 remains unskippable, and keyboard Tab semantics are unchanged.

**VERIFIED SOURCE FINDING:** `f00ec72` (`fix: recognize controller look in tutorial`) changes only stage-3 recognition in `Source/Tutorial.cpp`. The existing `deltah || deltav` mouse criterion is ORed with meaningful X or Y from `Input::controllerLook()`, the same post-deadzone signal used by the gameplay camera. There is no new Input helper, second deadzone calculation, synthetic mouse input, or write to mouse deltas. Existing tutorial-call guards exclude menus/pause, console freeze, and results; controller recognition additionally requires active gameplay, no input waiting, and no dialogue/directing. Stage timing, success accumulation, setup, and camera tuning are unchanged.

**Build validation / RUNTIME-VERIFIED (isolated harnesses only):** both changes passed the Web build and `git diff --check`. Triangle tests cover press/hold/release, stage 51 and sound, context guards, held entry/pause/resume, disconnect/reconnect, unchanged Tab and simultaneous input. Stage-3 tests cover mouse, horizontal/vertical stick input, deadzone boundaries, context exclusions, and clean reconnect. Camera timing/response and Triangle/menu regression checks passed. **OPEN QUESTION (physical verification):** neither tutorial addition has yet been reported as physically verified in Firefox/DualSense. Tutorial prompts remain keyboard-oriented.

### Browser observations and limits

**RUNTIME-VERIFIED:** Firefox exposed the wired DualSense through the standard Gamepad API; Emscripten/SDL exposed it to Lugaru as `Standard Gamepad`. Fullscreen is not required for controller input once the browser has exposed the device to the page.

**OPEN QUESTION (Bluetooth):** raw Bluetooth DualSense support is not verified. Exposure was inconsistent/not successfully established during the final runtime test.

**RUNTIME-VERIFIED (Chrome, updated 2026-09-27):** earlier tests on this development machine repeatedly returned null `navigator.getGamepads()` entries, including after renderer focus/fullscreen testing. Maintainer QA now confirms working wired DualSense input in current Chrome with the final character-restored Web build. The cause of the discrepancy is unknown: character restoration changed no controller/browser-input code, so no causal link is established. Firefox/wired DualSense remains the most extensively validated setup. This one Chrome result does not establish Bluetooth, other Chromium versions, other operating systems, or other controllers. No WebHID or Chrome-specific workaround was added.

### Historical context and remaining scope

**HISTORICAL EVIDENCE (prior research):** original/local Lugaru ancestry did not reveal inherited controller support. The 2019 `usineur/lugaru` Switch implementation (reference commit `9af3344`) remains useful historical reference. Its SDL GameController and left-stick digital movement approaches informed the preservation-safe direction; startup-only assumptions, its exact face-button mapping, and trigger-assisted camera rotation were not copied. Its fixed per-frame camera increments are unsuitable for the current Web loop's frame-rate-independent stick integration.

**HISTORICAL EVIDENCE (prior menu investigation):** the 2019 fork's `9af3344` added controller Start to pause handling; `7b81f2f` used cursor emulation and an A-as-mouse-click hack. Its fixed-per-frame cursor/camera increments and trigger-assisted motion are historical behavior, not the timing or binding contract for Lugaru-Web. Current UI reuses the pointer model but uses elapsed-time cursor integration, menu-local confirm, and contextual Back. The inspected historical tutorial handler remained Tab-only. The old face-button layout is reference material, not a requirement for the current mappings.

**RUNTIME-VERIFIED (progression):** the earlier observation that mouse clicking changes campaigns motivated pointer-path reuse. Milestone 5 physical QA now verifies controller campaign switching and world-map interaction through that same UI.

**OPEN QUESTION / remaining work:** physical tutorial QA; broader Chrome/Bluetooth compatibility; camera sensitivity and movement-threshold feel tuning; controller settings/inversion and glyphs/prompts; keyboard-only account naming and controller rebinding. True analog locomotion is a separate design problem; multiple-controller ownership, rumble/advanced DualSense features, trigger mappings, stereo decrement from controller, and WebHID remain outside implemented scope. NPC steering and shared character turning have now been investigated; residual slope/seam scheduler behavior remains open separately in B-13. See [backlog A6](research-backlog.md#a6-controller-support-completed-scope-and-remaining-work) for classification and follow-up priorities.

---

## B-9: WebAssembly Build Is Optimized Primarily for Size

**Status:** Open
**Priority:** Low
**Evidence:** Source inspection

### Location

```text
CMakeLists.txt
WebAssembly compile and link options
```

### Current Behavior

The browser build uses size-oriented optimization including:

```text
-Oz
-flto
Closure optimization
```

The generated `.wasm` binary is relatively small compared with the packaged game data.

### Expected Direction

Benchmark before changing anything.

Useful comparisons may include:

```text
-Oz
-O2
-O3
```

Record at minimum:

* WASM size,
* JS size,
* startup time,
* representative frame-time behavior.

Do not change optimization settings based on assumptions alone.

---

## B-10: Synchronous Level Loading Blocks the Browser Main Thread

**Status:** Open
**Priority:** Low
**Evidence:** Source inspection

### Location

Relevant areas include:

```text
CMakeLists.txt
WebAssembly link configuration
level-loading code paths
loading-screen rendering paths
```

The current build configuration does not enable Asyncify.

### Current Behavior

Level loading remains synchronous.

The build does not currently use Asyncify or another asynchronous loading architecture for these operations.

### Impact

Long-running loading operations can temporarily prevent the browser from presenting new frames or responding normally.

Existing loading-screen rendering may not visibly update until the blocking work completes.

### Expected Direction

Acceptable during initial stabilization unless testing shows severe stalls.

Revisit only after higher-priority correctness and persistence issues are resolved.

For v1.0.0 this remains open and is documented as a known Web limitation in the root `README.md`; no Asyncify or asynchronous-loading change was made.

---

## B-11: Account Creation Behavior Was Changed Globally

**Status:** Open
**Priority:** Medium
**Evidence:** Source inspection
**Introduced by:** inherited WebAssembly commit `b340204`

### Location

```text
Source/User/Account.cpp
Account::active()
```

### Current Behavior

Inherited changes cause a default `"Player"` account to be created automatically when no account exists.

This change is not restricted to WebAssembly builds.

### Impact

Native Lugaru behavior is changed as a side effect of the Web port.

The behavior may be appropriate for a browser build, but the current implementation applies it globally.

### Expected Direction

Compare against the original native behavior.

If automatic first-run account creation is needed only for Web builds, isolate that behavior appropriately rather than altering shared behavior without intent.

---

## B-12: WebAssembly Runtime Crash: Memory Access Out of Bounds

**Status:** Fixed
**Priority:** High
**Fix:** `7d4349b` (`fix: prevent invalid sprite access after deletion`)
**Evidence:** Runtime debugging and source inspection

### Location

```text
Source/Graphic/Sprite.cpp
Sprite::Draw()
Sprite::DeleteSprite(int)
```

### Environment

```text
Google Chrome 153.0.8010.47
Windows 11 25H2
Emscripten 6.0.9
Lugaru Web baseline b340204
```

The crash was reproduced both on the inherited baseline build and after the B-4 animation bounds-check restoration, confirming that B-4 did not introduce the defect.

### Observed Behavior

During normal gameplay, particularly after several maps and combat encounters, the WebAssembly build would eventually terminate with:

```text
Uncaught RuntimeError: memory access out of bounds
```

A debug WebAssembly build using `-g3`, `-O0`, Emscripten assertions, and GL assertions produced a useful symbolic stack trace. The first meaningful game-side failure was:

```text
std::unique_ptr<Sprite>::release(...)
std::unique_ptr<Sprite>::operator=(...)
std::vector<std::unique_ptr<Sprite>>::erase(...)
Sprite::DeleteSprite(int)
Sprite::Draw()
```

Later SDL event errors and stack-overflow messages occurred after the initial failure and were treated as secondary effects rather than the root cause.

### Root Cause

`Sprite::Draw()` iterates forward through the global sprite vector.

Several blood-sprite collision paths called:

```cpp
DeleteSprite(i);
```

while execution was still inside the current sprite's update logic.

`DeleteSprite()` performs:

```cpp
sprites.erase(sprites.begin() + i);
```

Erasing the element shifts subsequent vector elements and invalidates the current element. Some control paths then continued using `sprites[i]`, resulting in invalid or unintended sprite access after the erase.

Source-history inspection showed that this deletion pattern predates the WebAssembly port and is inherited from earlier Lugaru code rather than being introduced by the browser port.

### Resolution

Blood-sprite collision paths now record the collision through `spritehit` without immediately erasing the sprite.

Deletion is deferred until the blood-sprite collision logic has finished:

```cpp
if (spritehit) {
    DeleteSprite(i);
    continue;
}
```

The `continue` prevents any further access to the erased sprite.

The original forward-loop iteration behavior was otherwise preserved.

An earlier experimental fix changed the loop to manually manage its index after deletion. Although it prevented the original memory trap, it caused reproducible gameplay freezes after map loading. That broader behavioral change was rejected in favor of the narrower fix.

### Validation

The fix was tested in a debug WebAssembly build that previously reproduced the crash consistently.

After the change:

- multiple maps and combat encounters completed without reproducing the memory-access trap,
- the earlier experimental freeze was no longer present,
- the normal Release WebAssembly build completed successfully,
- extended gameplay in the Release build also failed to reproduce the crash.

The Release build used the project's normal optimization configuration, including `-Oz`, LTO, Closure compilation, and disabled Emscripten assertions.

A native build sanity check was also attempted. Native compilation was blocked before reaching `Sprite.cpp` by a pre-existing Linux SDL2 CMake include-path problem:

```text
fatal error: SDL.h: No such file or directory
```

That native-build issue is separate from B-12 and does not affect the successful WebAssembly validation.

### Impact

The previously reproducible WebAssembly gameplay crash is no longer observed in either Debug or Release browser builds.

B-12 is considered fixed based on the identified unsafe erase pattern, the matching debug stack trace, and successful repeated runtime validation.

---

## B-13: Player Direction Changes Snap Instead of Rotating

**Status:** Fixed (finalized default source and physical QA)
**Priority:** Medium
**Fix:** `0305947` (`fix: restore classic character behavior`; documentation `2d28dfe`, merged at `d21048e`)
**Evidence:** VERIFIED SOURCE FINDING / VERIFIED GIT HISTORY / RUNTIME-VERIFIED

### Original symptom and provenance

The inherited Web fork was reported to snap on direction reversals instead of visibly turning, including a report above 100 Hz. Later physical comparisons found native OSS 1.2 and current OSS `42f148d` smooth, while Web remained rough after yaw alone was restored. Classic eight-direction movement intent is distinct from this regression in executing/presenting turns.

**VERIFIED GIT HISTORY:** Ryan commit `1db27611eec7fe4c3f4019e233235e3a80fb2460` replaced bounded yaw and tilt with exponential damping using full-frame `frameDeltaTime` inside repeated physics subdivisions. It also changed head normalization/convergence/application and guarded draw-time `DoHead()` updates with `isFirstPhysicsTickOfFrame`. Normal physics frames clear that flag before drawing, skipping target conversion and convergence. Zero-tick frames are an exception, not a reliable update contract. These identified regressions do not establish that every change in that commit is broken.

### Final restoration

**VERIFIED SOURCE FINDING:** `Source/GameTick.cpp` restores the existing historical yaw predicates and `500/1000/2000°/s` rates, and bounded body tilt at `150°/s`, using `multiplier`. `Source/Objects/Person.cpp` / `Person.hpp` restore historical head conversion, limits, `500°/s` yaw and `250°/s` pitch, and pitch-only head-joint application. Head behavior matched across OSS 1.2, current OSS, and pre-modernization `8cee3e3`.

The final head implementation prepares one per-actor pending update after fresh simulation and consumes it at the first eligible draw. It preserves state eligibility and target producers while preventing repeated/stereo mutation and retaining the previous pose on zero-physics frames. This draw-order adaptation is intentional. Character experiment macros and Ryan's alternate yaw/tilt/head paths are removed; canonical rabbit TalkIdle remains. Input, locomotion intent, target-heading generation, velocity integration, AI, animation progression, camera follow, and browser/controller systems are unchanged by this restoration.

### Validation

**RUNTIME-VERIFIED (maintainer physical QA):** yaw materially improved turning; tilt substantially improved player locomotion presentation; head restoration worked well and resembled original Lugaru. Holding forward while rotating the camera continuously retargeted physical movement correctly. The combined restored build played normally with controller/gameplay behavior intact and no remaining perceptible player-locomotion problem. No verified remaining player-locomotion regression currently warrants another code change.

**Build/source validation:** isolated historical-law and head-equivalence checks preceded physical QA. The physically validated Release Web build at the time was a local validation build made without character or scheduler experiment definitions. Rebuilding after scheduler separation and `git diff --check` passed; bundled assets remain unchanged. Source behavior matches the physically validated restored character configuration. The restoration was subsequently committed as `0305947` after manual review. That validation build is not the current release build; see [build provenance](build-provenance.md#current-v10-release-build).

### Scheduler investigation closed: low-priority open observation

**Historical runtime observation:** the earlier scheduler A/B preceded the tilt/head restorations and suggested small NPC slope/seam benefits with little player effect. Different damping-call counts and simulated-time budgets complicated attribution. The subsequent investigation used the committed character-restoration baseline (`0305947`, documentation `2d28dfe`, merged at `d21048e`) and held those restorations constant.

**VERIFIED SOURCE FINDING:** movement and contact correction run before AI steering within `Game::Tick()`. Corrected NPC coordinates can feed subsequent steering. Terrain/velocity reconstruction, contact projection, pathpoint arrival checks, avoidance-timer ordering, and animation/state transitions are subdivision-sensitive. Ordinary pursuit heading updates are timer-gated; path-following can retarget each tick. Core `Person::doAI()`, `DoStuff()`, `DoAnimations()`, and `SphereCheck()` match OSS `42f148d033358925d1e31cec68ad02ee03ec64c2` ignoring comments/whitespace. Surrounding orchestration differs, including collision/avoidance decay using full-frame time on the first Web tick. These findings identify possible mechanisms, not the cause of a demonstrated runtime failure.

**RUNTIME-VERIFIED (maintainer physical A/B at approximately 60 Hz):** fixed `1 ms` and experimental OSS-style scheduling showed no clear or substantial overall difference. OSS-style scheduling appeared possibly slightly smoother when NPCs traversed slopes/seams, but the maintainer could not reliably distinguish the builds. No reproducible gameplay failure or meaningful player-facing regression was established. No diagnostic trace established causation.

**Decision / OPEN:** classification remains **no verified causal bug**. Close the active investigation and retain a low-priority open observation, pending a concrete reproduction. Keep the fixed scheduler and NPC gameplay unchanged. Neither scheduler equivalence nor OSS superiority has been established; no scheduler-wide or local gameplay fix is warranted by this evidence.

**Validation / cleanup:** both Release comparison builds (fixed and OSS-style scheduling) passed with identical bundled data. The extracted scheduler harness checked 540 frames per variant, modifiers, stalls, resets, and outer-frame invariants. Temporary bounded diagnostics sampled one NPC; they were not a deterministic replay. After the physical comparison, all scheduler-switch and diagnostic changes in `Source/main.cpp` and `Source/GameTick.cpp` were removed, restoring committed source exactly. Temporary instrumentation was discarded. No character, controller, asset, or NPC gameplay change is part of this closure.

The exact `Source/main.cpp` experiment was kept as a patch outside the repository and is not part of any commit. It used `LUGARU_EXPERIMENT_OSS_SCHEDULER`, selected `max(2, int(T * 200))` before gameplay modifiers, and preserved the current outer-frame/controller integration and `250 ms` stall cap. Its base is `8820f6125030127b1c7e8c328511c72a71d9299b`; patch SHA-256 is `e259a2f61248792b250462923666162b66182c8778dd333178ea4b2f58935448`.

See [backlog A2a](research-backlog.md#a2a-npc-slopeseam-and-scheduler-comparison-open). Camera follow remains separate and is not an unfinished locomotion blocker.


---

## B-14: Web NPOT Textures Render Black with Mipmapped Sampling

**Status:** Fixed; investigation closed
**Priority:** Medium
**Fix:** `eea54f4b7e079c27964b7decde3d6c7f56e0a5fd` (`fix: handle NPOT mipmapped textures on Web`)
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED (maintainer QA)

### Reproduction and Validation

**RUNTIME-VERIFIED (maintainer QA):** the shipped 40×40 RGBA `Data/Textures/SnowFlake.png` rendered as solid black squares when requested with mipmaps. Changing only its load argument from `1` to `0` restored correct snow. Restoring `1` and replacing Web automatic generation with explicit `glGenerateMipmap()` did not fix the black squares. The generic Web-only NPOT fallback then restored correct snow while keeping the original `Sprite::snowflaketexture.load("Textures/SnowFlake.png", 1)` call. Final manual QA also confirmed correct rendering of the 300×300 `Fire.jpg` loading-screen texture.

The Release Web build and `git diff --check` passed during implementation. Native preprocessing was equivalent apart from the local boolean alias; extracted policy checks covered 168 dimension/request combinations per native/Web variant. These automated checks supplement, not replace, the maintainer's physical rendering results.

### Compatibility Finding and Preservation

**VERIFIED SOURCE FINDING:** `Source/Graphic/Texture.cpp::TextureRes::load()` knows decoded image dimensions and controls minification/upload. The pinned GL4ES path handles legacy mipmap generation internally. The project permits WebGL2 but does not explicitly require it; the inspected SDL GLES2/Emscripten context path corresponds to WebGL1. Explicit generation alone cannot be assumed to solve limited NPOT sampling through that translation layer. This is not the blanket claim that WebGL cannot mipmap NPOT textures: WebGL2 supports NPOT mipmapping.

**HISTORICAL:** the relevant pre-fix loader behavior exists in current OSS `42f148d` and the known-good WASM baseline `b340204`; this was not identified as a recent Lugaru-Web regression.

**INFERENCE:** the evidence strongly supports an NPOT + mipmapped-sampling compatibility issue in the current Web/GL4ES translation path. The black result is consistent with incomplete/unsupported sampling state involving mipmapped filtering and GL4ES handling. **OPEN:** the exact runtime GL4ES branch and texture state producing the black result were not instrumented and proven.

### Final Policy

| Texture path | Behavior |
| --- | --- |
| Native | Unchanged, including historical `GL_GENERATE_MIPMAP` behavior |
| Web POT, mipmaps requested | Unchanged |
| Web NPOT, mipmaps requested | Use non-mipmapped `GL_LINEAR` minification; do not request automatic mipmap generation |
| Mipmaps not requested | Unchanged |

The local `useMipmaps` decision preserves the requested `hasMipmap` value. No asset-specific exception, resize, asset edit, or SnowFlake call-site workaround remains. Handling dimensions in the shared loader also covers dynamic/custom NPOT images reaching that loader without global mipmap disablement.

### Asset Audit

All 136 tracked texture images at the time of the fix were inspected; exactly four were NPOT. (*Later, 2026-10-04:* the two texture images added since, `Fur.png` from B-20 and the restored `FurBW.jpg`, are both 512×512 POT, so the NPOT set below is unchanged.)

| Asset | Dimensions / format | Shipped use and mipmap request |
| --- | --- | --- |
| `SnowFlake.png` | 40×40 RGBA PNG | Snow sprite; requests mipmaps; reproduced defect and final fix physically verified |
| `Fire.jpg` | 300×300 RGB JPEG | Loading screen; requests mipmaps; final appearance physically verified |
| `Quit.png` | 166×64 RGB PNG | Menu texture; no mipmaps requested; unaffected |
| `GrassDirt2.jpg` | 921×921 RGB JPEG | Packaged; no established shipped source/map load found |

All 13 shipped character skins are POT, as are the 18 shipped skybox faces and representative Smoke, Flame, BloodParticle, Leaf and Splinter sprites. `Fire.jpg` also appears as clothing in `jendraz1`; that path blends image data into a POT skin on the CPU rather than creating a separate NPOT mipmapped texture. The separate `Person::DoMipmaps()` skin re-upload path is unchanged; arbitrary external/custom skin behavior was not exhaustively validated.

### Separate Performance Observation

**RUNTIME-VERIFIED (maintainer observation):** many active fire spawners caused noticeable Web slowdown. This was investigated and fixed separately in [B-15](#b-15-dense-fire-sprite-rendering-slows-web); no causal link to the B-14 NPOT fallback was established.

---

## B-15: Dense Fire Sprite Rendering Slows Web

**Status:** Fixed; release blocker for shipped content closed
**Priority:** High
**Fix:** `592e158` (`fix: optimize Web sprite rendering with client arrays`)
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED (manual browser QA)

### Shipped Reproduction and Mechanism

**RUNTIME-VERIFIED:** Seven Tasks `Data/Maps/task3.json`, "Desert Village - evening," contains six historical fire objects. A temporary exact copy, `Data/Maps/task3_nofire.json`, removed only those six objects and ran smoothly. This A/B establishes that the fire-generated sprite population was sufficient to trigger the Web slowdown; the control was temporary test equipment and was never committed.

**VERIFIED SOURCE FINDING:** fire emits many flame particles, which also produce smoke. The historical `Source/Graphic/Sprite.cpp::Sprite::Draw()` path rendered each sprite individually with per-particle matrix work and immediate-mode geometry. Web/GL4ES paid substantial CPU overhead for that path. The fire objects and their spawn behavior are historical; this was not a new Web spawn regression. Reducing particle counts, rates, lifetimes, or visual fidelity was unnecessary.

### Manual Browser Measurements and Experiments

These are approximate observations from the investigated browser build and scene, not universal benchmarks. `calls_s` counts diagnostic `Sprite::Draw()` invocations per second; it is **not** literal displayed FPS.

| Path | Approximate task3 observation | What it established |
| --- | --- | --- |
| Legacy Web | ~30 `calls_s`; ~1,670 sprites/call; ~18.2 ms Sprite draw CPU; ~1,435 active flame and ~231 smoke | Visuals were correct, but per-sprite Web CPU cost was high. |
| CPU billboards | ~6.5 ms draw CPU with ~2,100 sprites | Removing per-particle matrix operations removed a major cost. |
| Immediate-mode batching | ~6.5 ms draw CPU with ~2,300 sprites | Fewer `glBegin`/`glEnd` groups alone did not remove the remaining cost. |
| Client arrays | Settled worst-case ~60–65 `calls_s`; ~2,350 sprites/call; ~1.9–2.0 ms draw CPU; ~1,990 active flame and ~320–340 smoke | The loop sustained more historical particles while rendering them more cheaply. |

In the array path, approximate draw CPU split into ~0.43–0.48 ms billboard geometry and ~1.4–1.6 ms submission. The observed CPU sprite-render cost per submitted particle fell by approximately 90% or more versus the legacy Web path. The larger active counts reflect the game loop no longer being starved by sprite rendering; they are not a particle-count reduction.

### Production Renderer and Preservation

**VERIFIED SOURCE FINDING:** under `__EMSCRIPTEN__`, `Sprite::Draw()` computes CPU billboards, packs position/UV/final-color client arrays, and submits `glDrawArrays` for consecutive compatible sprites. It preserves sprite storage and triangle order, retains the 1,024-sprite grouping cap, and does not sort or cull. Native retains its historical matrix and immediate-mode path. The normal Web build selects the optimized path automatically; there is no runtime sprite-path selector or experimental startup output. Optional effect diagnostics are compile-time disabled by default.

The change preserves particle counts, spawn and lifetime behavior, motion, opacity, size, rotation, billboarding, snow orbit, flame growth, textures, UVs, colors, blend/alpha rules, secondary smoke, blood collision/decal behavior, update/deletion logic, and sprite-type special cases. Static experiment validation included randomized billboard-geometry and rendering-stream-order comparisons.

### Regression and Final Production QA

**RUNTIME-VERIFIED (manual browser tests):** fire/smoke-heavy task3 looked normal and was dramatically smoother. The no-fire control remained smooth with ~17–25 incidental sprites and ~0.03–0.05 ms draw CPU, showing no pathological fixed overhead. Map1 snow looked normal with ~300 active particles, ~20 snow spawns/s, and ~0.28 ms draw CPU; the prior Web NPOT snow fix remained intact. Mixed combat effects during heavy fire showed cloud, blood, impact, bloodflame, shine, flame, and smoke with normal-looking visuals and ~1.7–2.2 ms draw CPU.

The cleaned normal Web build and diagnostic Web build both passed, as did `git diff --check` during implementation. Maintainer QA then ran the production build with no experimental query parameters; it felt good, and the fix was accepted. These are manual scene-specific results, not a cross-browser or native runtime benchmark.

---

## B-16: Delayed Spatial Exit Loses Campaign Progression (Empire Sky Palace)

**Status:** Fixed; Empire progression release blocker closed
**Priority:** High
**Fix:** `97121db` (`fix: preserve campaign progression across delayed exits`; `Source/GameTick.cpp` only)
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED (controlled diagnostic runs and manual production QA)
**Provenance:** shared game logic, not WebAssembly-specific. The susceptibility is inherited from upstream/historical completion and death-reload code; it is **not** established as a Web-introduced regression. Tracked here because it was found during Web campaign QA.

### State semantics

**VERIFIED SOURCE FINDING** (`Source/GameTick.cpp`, `Source/User/Account.hpp`):

| Name | Meaning |
| --- | --- |
| `winhotspot` | Live occupancy of any type `-1` exit hotspot, recomputed every tick in `TickOnceAfter()` (`distsq < Hotspot::size`). It is **not** a completion latch. |
| `won` | Generic in-memory "win already recorded" latch; cleared by `ResetBeforeLevelLoad()`. |
| `actuallevel` | Active campaign node index into `campaignlevels`. |
| `whichchoice` | Ordinal of the incoming `nextlevel` edge chosen to reach the node. |
| `Account::getCampaignChoicesMade()` | `choices.size()`, the **count** of saved edge choices, not a node ID. Campaign files allow arbitrary successor indices, so it cannot safely substitute for `actuallevel`. |

`Data/Maps/jendraz1.json` (Empire "Sky Palace") has two type `-1` hotspots on the bridge: a size-`0.0` placeholder and a size-`15.0` exit (effective radius about `3.87` under the squared-distance check; see [backlog H2](research-backlog.md#h2-hotspot-radius-semantics)).

### Failure mechanism

**RUNTIME-VERIFIED (controlled diagnostics):**

1. Entering the bridge hotspot recorded the campaign win immediately (`winCampaignLevel()`, `won = true`).
2. `changedelay` then counted down for roughly 0.7 s before routing.
3. A fast-moving player could leave the hotspot before the delay finished.
4. The later completion checks re-tested `winhotspot`, now false, so the transition stalled even though the win/progression was already saved.
5. If the player then fell and died, the old campaign death reload chose `campaignlevels[Account::getCampaignChoicesMade()]`. With one extra saved choice this loaded `jendraz2` (Throne Room) while `actuallevel` stayed `0`.
6. Completing the Emperor scene once routed with node-0 semantics and loaded `jendraz2` again; completing it a second time advanced again.
7. The accumulated extra choices then skipped `jendraz3` (Rebel Outpost).

**RUNTIME-VERIFIED (control):** remaining inside the bridge trigger until the delay completed behaved correctly (`jendraz1 → jendraz2 →` campaign menu `→ jendraz3`). The hotspot data and position are therefore not the defect; the fragile part is the delayed completion re-checking live occupancy, compounded by count-as-node death reload.

**HISTORICAL (separate):** Empire's visible draw-distance/fade pop-in was earlier concluded to be original behavior. It is unrelated to this defect and was not changed.

### Fix

**VERIFIED SOURCE FINDING** (`97121db`):

- Adds file-static `acceptedSpatialWin`, latched when a successful win is recorded while `winhotspot` is true. It survives the player leaving the trigger during the delay and resets in `ResetBeforeLevelLoad()`.
- Shared completion checks use `winhotspot || acceptedSpatialWin`.
- Ordinary campaign death before a win reloads `campaignlevels[actuallevel]`. The `loading == 2` reload is now campaign-gated; non-campaign reload handling remains separate and unchanged.
- A campaign death after the win is recorded but before delayed routing finishes resolves the recorded completion exactly once through the existing `winfreeze`/campaign-routing flow (`changedelay = -999`, matching the normal `winfreeze` convention).
- For `ChooseNext == 2` stealth transitions, the player's `coords`/`yaw`/`targetyaw` at win time are cached and restored in that death-recovery path, so dead/fallen coordinates are not carried into the successor map.

**Shared-mechanism note:** the latch is not campaign-only, so it also fixes the same fast-exit stall for non-campaign spatial-exit wins. This is a small intentional improvement to shared completion behavior, not a separate campaign feature.

### Review and QA

**Independent review:** an AI-assisted review with Claude Code examined the final production diff after runtime QA and found no blocking or high-severity defects; minor residual risk is recorded below. The review confirmed in source that `acceptedSpatialWin` resets through the common load path; campaign wins cannot be recorded twice through the new path; death-before-win and death-after-recorded-win are mutually exclusive; loader state reset makes the cached coords/yaw/targetyaw sufficient for the `ChooseNext == 2` case; non-campaign `loading == 2` handling remains intact; and `changedelay = -999` matches the existing `winfreeze` convention.

**RUNTIME-VERIFIED (production fix, all passed):**

1. Fresh Empire fast bridge run: player leaves the hotspot before the old delay expires; the transition to `jendraz2` still occurs.
2. Fresh Empire stop-inside control: remaining in the hotspot still transitions normally.
3. Throne Room: the Emperor scene completes once; the campaign menu offers Rebel Outpost (`jendraz3`) next; no duplicate Throne Room completion.
4. Ordinary campaign death before win: Sky Palace reloaded normally, NPC conversations still worked, and later bridge → Throne Room → Rebel Outpost progression remained correct.
5. Main campaign Village: normal completion, campaign/title flow, and next-mission selection behaved normally.

**VERIFIED SOURCE FINDING (not runtime-forced):** a death during the very small window after the win is recorded but before routing. Independent review traced `ChooseNext` `0`, `1`, and `2` and found it correct; temporary instrumentation to manufacture that state was intentionally not added.

### Residual risk

- **OPEN:** the post-win/pre-route death path is source-verified only.
- **VERIFIED SOURCE FINDING / OPEN:** campaign menu entry still derives `actuallevel` from `campaignlevels[getCampaignChoicesMade() - 1].nextlevel[whichchoice]` (`Source/Menu/Menu.cpp`), i.e. it treats the choice count as the predecessor node. This is correct for the linear bundled campaigns but unproven for branching campaigns; out of scope for `97121db` and tracked under [backlog H7](research-backlog.md#h7-campaign-branching-testhistory).
- **RESOLVED (separate polish):** the Throne Room Emperor dialogue activation area was improved with a map-local type `21` hotspot; see [backlog A9a](research-backlog.md#a9a-throne-room-emperor-dialogue-activation-area-resolved-2026-09-29). This was localized polish rather than part of B-16 or a Web-specific regression.
- **RESOLVED (separate exit-area polish):** the existing `jendraz1` type `-1` bridge exit hotspot was widened from size `15.0` (~3.87-unit effective radius) to `100.0` (10-unit effective radius) after runtime testing. The underlying exit/progression logic is unchanged; this is a map-local usability adjustment.

---

## B-17: Empire Sky Palace NPCs Receive Fire.jpg as Clothing

**Status:** Fixed; Empire visual regression closed
**Priority:** Medium
**Fix:** `6142c73` (`fix: restore Empire Sky Palace NPC clothing`); `bf94eca` (`fix: stop remapping missing Empire fire clothing`)
**Evidence:** VERIFIED SOURCE FINDING + HISTORICAL ASSET VERIFICATION + RUNTIME-VERIFIED controlled A/B
**Provenance:** inherited OSS content-conversion regression, not WebAssembly-specific. The visible defect came from converted Empire map data and shared clothing compositing behavior.

### Observed behavior

In Empire's opening Sky Palace map (`Data/Maps/jendraz1.json`), two NPCs rendered with a dark red patterned texture across much of their bodies instead of the normal clothing visible in historical Empire gameplay.

Both affected players contained:

`Textures/Fire.jpg`, tint `0.5, 0.5, 0.5`.

### Historical asset and map evidence

**VERIFIED HISTORICAL DATA:** the untouched 2008 Empire `jendraz1` binary contains two corresponding references to `:Data:Textures:fire.png`.

The distributed Empire archive does not contain `fire.png`.

A verified Lugaru v1.05 Windows installer (`LugaruSetup.exe`, MD5 `a6a790f4b0e82e9441ff834a7e452a3f`) was extracted with Inno Setup tooling. Its stock textures contain `Data/Textures/Fire.jpg` but no `fire.png`.

The v1.05 `Fire.jpg` is a 512x512 three-channel JPEG. Current OSS `Data/Textures/Fire.jpg` is a different 300x300 three-channel JPEG with a different hash.

The OSS hardcoded-path conversion table contained `:Data:Textures:fire.png -> Textures/Fire.jpg`. This changed a historically unresolved PNG clothing reference into a successfully loadable JPEG.

### Failure mechanism

**VERIFIED SOURCE FINDING:** `Person::addClothes()` composites requested clothing images into the character skin buffer. For three-channel RGB images such as JPEGs, no alpha mask exists and the compositor treats the image as fully opaque.

The conversion therefore changed effective behavior from an unresolved clothing layer into an opaque `Fire.jpg` layer that overwrote the underlying rabbit skin, producing the dark/red full-body corruption.

This is a content-conversion regression rather than a Web texture-upload or GL4ES defect.

### Controlled reproduction

**RUNTIME-VERIFIED:** the two `Textures/Fire.jpg` entries were temporarily changed back to the historically referenced missing `Textures/fire.png`.

After rebuilding the Web package and replaying Sky Palace, both NPCs returned to the normal appearance seen in historical Empire gameplay. This isolated the `Fire.jpg` substitution as sufficient to cause the defect.

### Fix

`6142c73` removes only the two bogus `Textures/Fire.jpg` clothing objects from `Data/Maps/jendraz1.json`.

The final map does not retain the missing `fire.png` paths. Removing the dead layers preserves their original effective behavior without generating missing-resource failures.

`bf94eca` removes the obsolete `:Data:Textures:fire.png -> Textures/Fire.jpg` conversion rule from `Misc/list-hardcoded-paths-fixes.txt`, preventing future conversions from recreating the regression.

No clothing compositor or renderer behavior was changed.

### Review and QA

**RUNTIME-VERIFIED (production fix):**

- Empire Sky Palace loads normally.
- Both affected NPCs now render with normal clothing.
- The missing-path diagnostic produced the same corrected appearance.
- `jendraz1.json` remains valid JSON.
- No `Fire.jpg` clothing references remain in `jendraz1.json`.
- Legitimate `Fire.jpg` loading-screen behavior is untouched.

### Residual risk

The exact authoring origin of the historical `fire.png` references is unknown. They may represent stale editor data or an asset that existed only in the campaign author's local environment. Neither the distributed Empire archive nor the verified Lugaru v1.05 installation supplies that PNG.

That uncertainty does not affect the demonstrated regression: mapping the unresolved path to opaque `Fire.jpg` changed the visible result, while removing that substitution restores the historical appearance.

---

## B-18: Dense Web Foliage Causes Severe Slowdown

**Status:** Fixed; Web performance regression closed
**Priority:** High
**Fix:** `df72be9` (`fix: optimize Web foliage rendering`)
**Evidence:** RUNTIME-VERIFIED controlled A/B + VERIFIED SOURCE FINDING + clean-build validation
**Provenance:** Web/GL4ES rendering regression. Empire map content and foliage assets were not defective; the comparable native OSS build did not exhibit the same severe slowdown.

### Observed behavior

In Empire's opening Sky Palace map (`Data/Maps/jendraz1.json`), the Web build slowed substantially in the dense tree cluster around the opening area near Leila. The slowdown was most noticeable while moving, jumping, or rotating the camera inside the foliage. Areas of the same map without dense trees remained substantially smoother.

The native OSS build did not show comparable slowdown in the same area.

### Controlled A/B findings

Removing the tree-leaf objects made the affected area smooth, establishing foliage as the trigger.

Reducing the leaf-object count by approximately half did not materially reduce the slowdown, so the defect did not scale primarily with the number of leaf objects or draw calls.

Enabling backface culling did not resolve the slowdown and visibly broke the foliage.

Disabling the normal-pass alpha test improved performance but did not fully resolve it by itself.

Disabling the translucent foliage second pass did not resolve the remaining slowdown.

Rendering the full leaf vertex count as points instead of triangles was smooth. This showed that vertex submission itself was not the dominant cost; rasterizing the overlapping leaf triangles was.

Sorting normal-pass tree leaves front-to-back substantially improved performance. Combining front-to-back ordering with removal of the legacy alpha-test path made the dense cluster smooth.

### GL4ES alpha-test finding

**VERIFIED SOURCE FINDING:** GL4ES fixed-pipeline emulation implements legacy `GL_ALPHA_TEST` by generating fragment-shader discard logic.

That path was disproportionately expensive for Lugaru's dense, overlapping foliage in WebGL. Front-to-back ordering reduced overdraw substantially, but the alpha-test path remained a major cost in the densest foliage.

`GL_SAMPLE_ALPHA_TO_COVERAGE` was available through GL4ES state tracking, but its fixed-pipeline enable path did not forward the state to the underlying GLES/WebGL context. Directly enabling alpha-to-coverage through WebGL restored the foliage cutouts while retaining the improved performance.

### Resolution

`df72be9` applies the Web-specific rendering fix without modifying Empire content or foliage assets:

- Web builds request a 4-sample multisampled OpenGL/WebGL context.
- Normal-pass tree leaves are rendered front-to-back on Web to reduce overdraw.
- Web tree leaves use multisample alpha-to-coverage instead of the legacy alpha-test path.
- Native builds retain the original object ordering and alpha-test behavior.
- `cmake/patches/gl4es-alpha-to-coverage.patch` changes the pinned GL4ES dependency so `GL_SAMPLE_ALPHA_TO_COVERAGE` is forwarded to the underlying WebGL state.
- CMake applies that GL4ES patch automatically when the dependency is fetched.

The original foliage second pass remains intact.

### Validation

A completely fresh build directory was configured using Emscripten 6.0.9.

A subsequent rebuild of the same configured build directory exposed a reproducibility issue in the original CMake patch step: `git apply` attempted to apply the GL4ES patch a second time and failed because the source tree was already patched.

A follow-up build-system fix added `cmake/apply-gl4es-patch.cmake`, which checks whether the alpha-to-coverage change is already present before applying the patch. The patch step is therefore idempotent: fresh dependency trees receive the patch, while already-patched trees are accepted without failure.

Both paths were verified: the existing patched build tree rebuilt successfully, and a separate fresh build tree automatically received the patch and built successfully.

The clean dependency fetch automatically applied `cmake/patches/gl4es-alpha-to-coverage.patch`, producing the expected GL4ES `proxy_GO(GL_SAMPLE_ALPHA_TO_COVERAGE, ...)` path.

The resulting browser context reported antialiasing enabled with four samples.

Runtime QA on `jendraz1` confirmed:

- foliage retained its transparent cutout appearance;
- the severe slowdown in the dense Sky Palace tree cluster was eliminated;
- only minor residual slowdown could occasionally be observed in the worst-case foliage concentration;
- no obvious foliage rendering regressions were found.

No Empire map objects, leaf models, or textures were removed or simplified.

See [backlog A9c](research-backlog.md#a9c-empire-sky-palace-dense-foliage-web-slowdown-resolved-2026-09-30).

---

## B-19: Jump Boost Applied Too Quickly Under Fixed Physics Timestep

**Status:** Fixed
**Priority:** High
**Fix:** `21efae8` (`fix: restore time-scaled jump boost`)

### Symptom

Player jumps in the inherited Loupgarenne/Web baseline are noticeably higher and feel floatier than current upstream Lugaru. Quick taps and held jumps also produce much less variation in jump height.

### Source finding

Lugaru `1.2` and Loupgarenne immediately before commit `1db2761` apply stored jump power using the time-scaled increment `multiplier * 6`.

Loupgarenne commit `1db2761` replaced that behavior with:

```cpp
const float jumpIncrement = std::max(multiplier * 6.0f, 0.1f);
```

The same commit introduced the fixed 1 ms simulation timestep. At `multiplier == 0.001`, the Lugaru 1.2/upstream increment is `0.006`, while the new minimum forces `0.1`, causing stored jump power to be applied approximately 16.7 times faster than intended. Roughly four units of remaining jump power can therefore be consumed in about 40 ms, making nearly every ordinary jump approach full boost.

The change predates the WASM-support commit `b340204`, so it is inherited by `baseline/wasm-b340204` and was not introduced by later lugaru-web work.

### Fix

Restore the time-scaled Lugaru 1.2/upstream behavior:

```cpp
const float jumpIncrement = multiplier * 6.0f;
```

The fixed 1 ms simulation timestep remains unchanged.

### Validation

A controlled one-line Web A/B retained the current scheduler, gravity, takeoff velocity, jump-power recharge, and movement integration. With only the `0.1f` floor removed, quick taps became shorter, held jumps again produced progressively greater height, and overall jump behavior matched the separately tested upstream OSS build by physical gameplay comparison.

This source and runtime evidence isolates the jump-boost floor, rather than the fixed timestep itself, as the cause of the excessive jump height.

### Historical scope

The fix is verified against the Lugaru `1.2` source tag and current upstream behavior. Parity with the older commercial/native 1.05 executable has not yet been physically measured, so this is documented specifically as restoration of Lugaru 1.2/upstream behavior.

### Separate follow-up

Commit `1db2761` also changed other multiplier-sensitive systems, including ragdoll constraint calculations. Those changes are not part of B-19 and should be investigated independently rather than bundled with the jump fix.

---

## B-20: Temple Tinted JPEG Fur Renders with RGB Striping

**Status:** Fixed
**Priority:** Medium
**Fix:** `85477f5` (`fix: restore Temple tinted fur rendering`)

### Symptom

Three characters in the bundled Temple campaign render with repeating RGB-colored vertical stripes across their fur:

- `svendecoy` player 0
- `svencouncil` player 4
- `svendoor` player 1

All three use `Textures/Fur.jpg` as a full-body clothing layer with tint `(0.6, 0.5, 0.45)`. A control case using the same `Fur.jpg` at tint `(1.0, 1.0, 1.0)` renders normally.

### Source finding

`Person::addClothes()` applies clothing tint while iterating raw image bytes. JPEG textures load as 24bpp RGB, but the tint-selection logic uses `i % 4`, effectively assuming four-byte RGBA pixels:

```cpp
if ((i % 4) == 0) texture.data[i] *= tintr;
if ((i % 4) == 1) texture.data[i] *= tintg;
if ((i % 4) == 2) texture.data[i] *= tintb;
```

For three-byte RGB input, the four-byte tint cycle becomes misaligned with the three-byte RGB pixel layout. Non-unity tint therefore produces a repeating striped color pattern. Unity tint hides the defect because multiplying each affected byte by `1.0` leaves the image unchanged.

PNG textures load as 32bpp RGBA and therefore align with the existing compositor logic.

### Historical provenance

The Temple campaign originally referenced `furwhite.png`. Upstream commit `acadd9e` changed those references to `Fur.jpg` because the two images had identical pixel content.

That substitution preserved the decoded RGB image but not its runtime format semantics: the PNG path supplies 32bpp data while `Fur.jpg` supplies 24bpp data. As a result, three non-unity-tinted Temple fur layers began exercising the compositor's RGB tint-indexing quirk.

This predates Loupgarenne, the WASM port, and lugaru-web.

### Fix

Commit `85477f5` adds `Data/Textures/Fur.png`, an RGBA PNG re-encode of the existing `Fur.jpg`.

The new asset was verified to have:

- identical 512×512 dimensions
- RGB pixels identical to the decoded `Fur.jpg`
- alpha `255` for every pixel

Only the three affected non-unity-tinted Temple references were changed from `Textures/Fur.jpg` to `Textures/Fur.png`. Existing unity-tinted `Fur.jpg` references remain unchanged.

No compositor behavior was modified.

### Validation

Before the fix:

- `svendecoy`, `svencouncil`, and `svendoor` reproduced visible RGB striping on the affected characters.
- `svenchasm`, using the same `Fur.jpg` with unity tint, rendered normally.

After changing only the three affected references to `Fur.png`, all three previously striped characters rendered cleanly with their intended tint.

This runtime A/B agrees with the source analysis and isolates the format-sensitive clothing tint path as the cause.

### Scope

B-20 intentionally uses a narrow data restoration rather than globally changing the historical `addClothes()` compositor. The broader clothing audit found additional historical, devtool, and preservation issues that are tracked separately under backlog A11.

---

## Release campaign QA fixes (2026-10-05 – 2026-10-06)

**Status:** Fixed; merged into `release/v1.0.0-prep` at `87c182a`
**Evidence:** VERIFIED SOURCE FINDING + RUNTIME-VERIFIED (maintainer campaign playthroughs and targeted map tests)
**Provenance:** shared game code and inherited campaign data, not WebAssembly-specific. The pre-fix code paths below are present unchanged in `baseline/wasm-b340204`. Like B-16, these were found during Web campaign QA and are recorded here; no B-number was assigned.

Full campaign playthroughs during release QA are complete for Original (27/27 levels), Seven Tasks (18/18) and Temple (31/31). **OPEN:** the full 41-map Empire campaign playthrough has not yet been run. The intended method is to play the real campaign route, record suspicious behavior, avoid changing data mid-run unless progression is blocked, and investigate confirmed issues afterwards. Challenge-map coverage and browser/release edge testing also remain.

The data-cleanup evidence and the decisions not to change suspicious-looking data are recorded in [preservation and modding section 19](preservation-and-modding.md#19-changing-inherited-campaign-data-v10-campaign-qa-review); the dialogue launch rules involved are in [architecture 20.5](architecture.md#205-dialogue-is-the-games-cutscene-system-and-a-tiny-script-vm).

### Minimap markers for helper actors (`f23faa2`)

Several Temple maps and Empire `jendraz41` contain Persons with scale `0.0` or `20000.0` that appear to serve cutscene/staging roles. The minimap loop in `Game::DrawGLScene()` (`Source/GameDraw.cpp`) drew a marker for every Person, producing misleading markers for these helpers.

The loop now skips Persons with `scale <= 0.001` or `scale >= 1000.0`. Ordinary actors (bundled scales around `0.2`) and the `2.0`-scale Long-ten Giant in Seven Tasks `task9` are drawn as before. No actor data was changed.

**RUNTIME-VERIFIED (maintainer QA):** after a rebuild, affected maps containing helper/sentinel actors were checked, and `task9` was checked to confirm that the legitimate Long-ten Giant minimap marker remained visible while extreme-scale helper markers were suppressed.

### Long-ten Giant encounter, Seven Tasks `task9` (`d997ab3`)

**VERIFIED SOURCE FINDING (data):** `task9` contained three dialogues. D0 (type `31`) is Wort's level-ending dialogue. D2 (type `12`, target Person 2, the `2.0`-scale giant) is the giant encounter. D1 (type `11`) contained the same giant encounter, but its type targets Person 1 (Wort), so it was attached to the wrong actor.

**VERIFIED SOURCE FINDING (mechanism):** `Dialog::tick()` started proximity dialogue only when `distsq` between character centres was below a fixed `6` (about 2.45 units). The giant's size prevented the player from reliably getting close enough to trigger D2.

**Fix:**

- `task9.json`: remove D1; keep D0 and D2 unchanged apart from normalizing D2's speaker name from `Longten Giant` to `Long-ten Giant`. All non-dialogue map data is unchanged.
- `Source/Level/Dialog.cpp`: when the target's scale is between `0.001` and `10`, the squared interaction distance becomes the larger of `6` and `1.5 × ((player scale + target scale) × 2.5)²`. Default-sized pairs keep the historical `6`, and helper-scale targets (`0.0`, `20000.0`) are excluded. In bundled data, the giant is the only dialogue target that this enlarges.
- `Source/GameDraw.cpp`: dialogue speaker names that would overflow the left edge of the dialogue box are scaled down to fit. Names that already fit are drawn as before.

**RUNTIME-VERIFIED (maintainer QA):** approaching the giant triggers the correct encounter once, the full `LONG-TEN GIANT:` speaker label fits, Wort gives only his level-ending dialogue, and ordinary NPC dialogue interaction is unchanged.

### Combat music survives Pause → End Game (`d07fea4`)

**Symptom:** after Pause → End Game during combat, the fight theme could continue playing in the menu.

**VERIFIED SOURCE FINDING:** the pause and resume paths in `Game::ProcessInput()` and the Pause-menu Resume button in `Menu::Tick()` paused and resumed only `leveltheme`, not `stream_fighttheme`. End Game also left gameplay music state in place.

**Fix:** pausing now also pauses `stream_fighttheme`, and resuming restores whichever stream `musictype` records (fight theme or level theme). End Game pauses both gameplay streams and zeroes their `musicvolume` / `oldmusicvolume` entries before returning to the main menu.

**RUNTIME-VERIFIED (maintainer QA):** combat Pause → Resume through the menu button and through Esc/Back, combat Pause → End Game, peaceful Pause → Resume, and starting another level after End Game all behaved correctly.

### Campaign map shows final state after completion (`3363eae`)

**Symptom:** after completing a campaign and leaving the ending screen, the campaign map could keep showing the completed final state, even though progress had been reset internally.

**VERIFIED SOURCE FINDING:** `Menu::Tick()` calls `Account::active().endGame()`, which clears campaign progress, but the campaign menu had already been built from the previous state.

**Fix:** `Menu::Load()` is now called immediately after `Account::active().endGame(); endgame = 0;`, so the visible menu reflects the reset state. This is a menu/state synchronization fix. Account data and save format are unchanged.

### Campaign data and text (`20be11c`, `744c7de`, `4d27256`, `488aae6`)

- Removed stale copied dialogue objects from Temple maps (`svenninja`, `svenruins`, `sventemple`, `sventemple2`, `svenfireice`, `svenforest`, `svenfort`, `svenredbwafter`, `svenshan`, `svenclimb`, `svenheaven`, `svenchasm`, `sventhrone`). Legitimate Temple dialogue is retained unchanged, except one wrong name reference (`Garett!` → `Shan!` in `svenshan`).
- Normalized first campaign-node labels to `..._start_new_campaign!` in `main.txt`, `empire.txt` and `seventasks.txt`; Temple already used that form.
- Changed the Temple final node label from `Ice_Cave_(Last_Level)` to `Ice_Cave`.
- Changed the Empire ending text from `Lugaru's island` to `Lugaru Island`.
- Replaced the Temple ending text, an OSS placeholder, with a three-line ending.

**VERIFIED SOURCE FINDING (re-checked against the commits):** in every changed map, the JSON parses, all non-dialogue map data is identical, and retained dialogues are unchanged apart from the `svenshan` and `task9` name fixes. Large diff line counts come from deleting whole serialized dialogue objects. **RUNTIME-VERIFIED (maintainer QA):** after a full data rebuild, `svenclimb`, `svenheaven`, `svenchasm`, `sventhrone` and `sventemple2` each loaded directly and played normally. The stale Shan dialogue no longer appeared, and `sventemple2`'s Sven/Silb conversation still played.

---

## CI-1: macOS CI Fails During Dependency Setup

**Status:** Open
**Priority:** Low
**Evidence:** CI observation

### Location

```text
GitHub Actions macOS workflow
Homebrew dependency installation step
```

### Observed Behavior

The inherited macOS workflow attempts to install an x86_64 Homebrew environment on an Apple Silicon runner.

The workflow fails before Lugaru source compilation begins.

Observed error:

```text
Homebrew on macOS is only supported on Apple Silicon processors!
```

### Impact

macOS CI does not currently provide a usable native-build reference.

There is no current evidence from this failure that the Lugaru source itself fails to compile on macOS.

### Expected Direction

Update the workflow to use an architecture-compatible dependency setup or an appropriate runner.

Native CI restoration remains lower priority than browser correctness issues.

### Current state (2026-10-04)

**VERIFIED SOURCE FINDING (inherited state):** before the v1.0 release-preparation cleanup, `.github/workflows/` was unchanged since Loupgarenne commit `1db2761`. It built native Linux, macOS and Windows (MinGW cross-compile) targets under `lugaru-loupgarenne-*` names, while the inherited release workflow triggered on `*-loupgarenne` tags.

**CURRENT STATE (2026-10-04):** native comparison artifacts now use `lugaru-web-native-*`, and the inherited tag-driven native release workflow has been removed because v1.0.0 is a Web-only release. The retained workflow still builds only native targets; no workflow currently builds or tests the Web target, so CI provides no evidence about the Web release. The v1.0 Web build is produced locally; see [build provenance](build-provenance.md#current-v10-release-build). Whether the macOS failure above still reproduces was not re-checked for v1.0.

---

## Adding New Issues

When a new porting issue is confirmed, add a stable ID and include:

```text
ID
Summary
Status
Priority
Evidence
Relevant source location
Observed or inferred behavior
Impact
Reproduction steps when applicable
Expected direction
Introduced-by commit when known
Resolution commit once fixed
```

Avoid creating issue entries for speculative concerns without either source evidence or an observable symptom.

Do not describe source-inferred behavior as runtime-confirmed until it has been reproduced.

---

## Resolving Issues

When an issue is fixed:

1. Change its status to **Fixed**.
2. Record the fixing commit.
3. Describe the final behavior briefly.
4. Preserve the original symptom and investigation notes.
5. Update the high-level issue table in `PROJECT.md` if its status changes.

When an issue originates from a known inherited commit, preserve that provenance after resolution.

This file should preserve the reasoning behind fixes rather than only showing the current state.
