# Release Notes

This file contains Lugaru-Web release notes, followed by the inherited release
notes of the OSS Lugaru project.

Issue IDs such as B-13 refer to entries in
[`Docs/porting-notes.md`](Docs/porting-notes.md); IDs such as A11 refer to
[`Docs/research-backlog.md`](Docs/research-backlog.md). Those documents hold
the evidence, provenance and validation limits for each change.

## Lugaru-Web v1.0.0 (2026-10-13)

As of 2026-10-13, the final v1.0.0 Web artifacts, built from source commit
`1b81e1d`, are live at https://loganwalkerdev.github.io/lugaru-web/, and the
public deployment has been verified (see `Docs/build-provenance.md`).

Lugaru-Web v1.0.0 is a preservation and browser-stabilization release built
on Ryan Deering's Loupgarenne WebAssembly baseline
(`baseline/wasm-b340204`). It concentrates on browser reliability, historical
behavior, campaign and content repairs, performance, persistence, controls and
documentation rather than on new gameplay.

The in-game/console version string is
`Lugaru-Web v1.0.0 (Lugaru 1.3)`, explicitly separating the Lugaru-Web
release number from the inherited Lugaru engine version.

### Highlights

- Self-contained Web build: `lugaru.data` is served from the same origin as the
  page. The inherited CDN option and the redirect to Ryan Deering's server
  were removed (`36407bd`).
- Built with Emscripten 6.0.9; see `Docs/build-provenance.md`.
- Browser saves and settings persist through IndexedDB (IDBFS), with safe
  migration of older `localStorage` saves.
- Restored historical character turning, body tilt, head tracking and jump
  timing.
- Major Web performance fixes for dense fire effects and dense foliage,
  without reducing historical content.
- Campaign progression and map/clothing content repairs for Empire and
  Temple, with provenance recorded.
- Controller support for gameplay, camera, menus, campaign/world map and
  challenges.

### Detailed changes

**Browser platform (Web-specific):**

- Saved resolution and VSync settings are no longer overwritten on load;
  selectable 16:9 Web render resolutions (B-1, `abb81d7`)
- Main-loop timing initialized after loop creation; animation-frame
  scheduling when VSync is on (B-2, `abb81d7`)
- IDBFS persistence with conservative legacy migration (B-3, `3d747d3`)
- Motion blur shown as `Unavailable on Web` instead of a misleading toggle;
  native unchanged (B-6, `fad80ac`)
- Key and mouse rebinding fixed with non-blocking Web capture (B-7, `a7bfd51`)
- NPOT textures requesting mipmaps no longer render black on Web (B-14,
  `eea54f4`)
- Dense fire/smoke sprite rendering optimized on Web (B-15, `592e158`)
- Dense foliage rendering optimized on Web with 4x MSAA alpha-to-coverage and
  a small pinned-GL4ES patch (B-18, `df72be9`, `9012637`)

**Shared engine fixes (also affect native builds):**

- Animation frame bounds checking restored (B-4, `b222ddc`)
- Out-of-bounds crash after sprite deletion fixed (B-12, `7d4349b`)
- Historical yaw, body tilt and head behavior restored (B-13, `0305947`)
- Delayed spatial exits no longer lose campaign progression; campaign death
  reloads use the current node (B-16, `97121db`)
- Time-scaled jump boost restored under the fixed physics timestep (B-19,
  `21efae8`)
- PNG/JPEG images too large for the fixed image buffer now fail to load
  instead of overflowing it, and clothing that does not match the skin size
  is skipped instead of overrunning the skin buffer (A11, `babb5d0`,
  `117b2ec`)

**Game content:**

- Empire Sky Palace: removed two converted `Fire.jpg` clothing layers that
  corrupted NPC appearance, and the conversion rule that created them (B-17,
  `6142c73`, `bf94eca`)
- Empire Sky Palace exit trigger widened and Throne Room Emperor dialogue
  trigger made reachable from the floor (map-local polish; `ab3f0a0`,
  `2ae4c5e`, `79f4bfc`)
- Temple: tinted fur no longer renders with RGB striping, via a
  pixel-equivalent `Fur.png` (B-20, `85477f5`)
- Temple: licensed `FurBW.jpg` appearance restored in `svenclimb`,
  `svenheaven` and `svenroad` (`fef0364`)

**Controller support (new):**

- SDL GameController support for movement, gameplay actions, camera look,
  pause, and pointer-based menu navigation including campaign/world map and
  challenge flows (B-8)
- Most extensively physically tested with a wired DualSense in Firefox.
  Wired DualSense input has also been observed working in current Chrome.
- Tutorial Triangle advance and tutorial stage-3 right-stick recognition are
  implemented and validated by source review and automated harnesses; they
  have not yet been reported as physically verified.

**Release QA fixes (shared, also affect native builds):**

- Crash when catching a thrown knife fixed (`e95844f`)
- Grounded knife finisher hit detection made more reliable; the original test
  still runs first (`8f25ff1`)
- Long-ten Giant encounter (Seven Tasks `task9`) triggers reliably; long
  speaker names fit the dialogue box (`d997ab3`)
- Fight music no longer continues after Pause → End Game, and resume restores
  the correct track (`d07fea4`)
- Campaign map shows the reset state after completing a campaign (`3363eae`)
- Cutscene helper actors no longer appear on the minimap (`f23faa2`)

**Release QA fixes (game content):**

- Empire `jendraz11` throne dialogue triggers reliably (`d51f9d5`)
- Stale copied dialogue removed from Temple maps; one wrong name fixed
  (`20be11c`, `744c7de`, `488aae6`)
- Duplicate giant dialogue removed from `task9` (`d997ab3`)
- Campaign start labels normalized; Temple final label is `Ice_Cave`
  (`744c7de`, `4d27256`)
- New Temple ending text written for Lugaru-Web, replacing an OSS placeholder;
  this is not recovered original text (`744c7de`)

**Release QA fixes (Web-specific):**

- Fullscreen setting stays in sync when the browser exits fullscreen
  (`09cea07`)
- The game always starts windowed in the browser (`501e2ef`)
- Web defaults: 1920×1080, High detail, Blood low, VSync on, Unlock FPS off
  (`6e51b9d`)
- Quit removed from the Web main menu; End Game remains (`e1cfb2e`)

**Project:**

- Project, attribution and licensing documentation for public release:
  `README.md`, `PROJECT.md`, `Docs/`, `COPYING.txt`, `AUTHORS`,
  `CONTENT-LICENSE.txt`, `THIRD-PARTY-NOTICES.txt`
- Shell title, window title and startup banner renamed to Lugaru-Web
  (`36407bd`)

### Known limitations

- Motion blur is unavailable in the Web build (B-6).
- Level loading is synchronous and can briefly block the browser (B-10).
- The Web build always starts windowed; fullscreen must be entered after the
  page loads.
- The Web main menu has no Quit button; close the browser tab to exit.
- A default `"Player"` account is auto-created in all builds, a behavior
  inherited from the WebAssembly baseline (B-11).
- Controller support has not been exhaustively tested across browsers,
  operating systems, Bluetooth and controller models; controller rebinding,
  prompts and sensitivity settings are not implemented, and account-name
  entry requires a keyboard (B-8).
- Native builds are comparison targets only and were not release-validated.
  The inherited CI workflows build native targets only.

# OSS Lugaru Release Notes

The following are the release notes for each version of Lugaru as released by
the OSS Lugaru team, in reverse chronological order.

The numbers preceded by a hash (e.g. #9) reference GitLab issue numbers on
the [OSS Lugaru repository](https://gitlab.com/osslugaru/lugaru).

## Version 1.2 - 11 February 2017

Second release of the open source Lugaru project! It continues the refactoring
work to make the old Lugaru code more modular and easier to maintain and
extend. A particular attention was given to the development tools (in game
editor) to improve the modability of the game.

### Highlights

- Keep track of and display the game's version number in the console and
  main menu.
- Many fixes and improvements to development tools, most of them should now
  work as described in the documentation.
- New monospace font for the console, more readable.
- Configurable campaign end message.
- Various fixes to mod levels and campaigns.
- A lot of behind-the-scenes refactoring that should help bring more
  interesting changes in future releases.

### Detailed changes

**Engine:**

- Fix displaying the localized key names instead of the QWERTY ones (#62)
- Fixed bushes appearing above ground on first level loaded
- Add a monospace font for the console text to increase legibility (#23)
- Devtools: improve handling of `map` command to avoid crashes (#23)
- Devtools: prevent the `clothes` command from saving invalid paths (#59)
  * Added `clotheslist` command to inspect player clothes
- Devtools: reviewed and fixed conflicting hotkeys and crashes (#73, #89)
- Devtools: fix regression from 1.1 in changing player type (wolf/rabbit) (#88)
  * Some bindings were changed, see `Docs/DEVTOOLS.txt` for the updated list.
- Campaign end message now configurable in the campaign definition file (#60)
- Fix long dialog lines printed out of the dialog area (#86)
- Lots of code refactoring and modernisation, fixed warnings
- Many bug fixes

**Game content:**

- Fix incorrect number of levels in the Seven Tasks campaign file (caused a
  crash on Win32) (#70)
- Fix crash in "Temple of the Wind" (sventemple) and "Snowy Grotto"
  (svengrotto) levels (#76, #79)
- Add and fix missing black fur texture for Ash in the Empire campaign (#84)

**Project:**

- Applied `clang-format` code formatting to the source, using a custom
  Mozilla-based style (#18, !14)
- Start properly versioning the game with version number displayed in the menu
  and console (#82, !15)


## Version 1.1 - 14 December 2016

First release of the open source Lugaru project! It contains most commits
made from various contributors since the open sourcing in 2010, and is
therefore more advanced than the preceding Lugaru HD release by Wolfire.

### Highlights

- Multiple campaigns support, making it easier to install and play mods!
- Those Lugaru mods are included by default as alternative to the Turner
  official campaign:
  * Temple, by Silb
  * Empire, by Jendraz
  * The Seven Tasks, by Philtron R.
- All assets (animations, textures, models, maps, sounds, etc.) of Lugaru HD
  by Wolfire are under a free and open source license (CC-BY-SA 3.0)!
- Window and input management ported from SDL 1.2 to SDL 2.0, improving the
  support for modern screen resolutions and fullscreen mode, as well as
  better input handling.

### Detailed changes

**Project:**

- Migrated the project from Bitbucket to GitLab.com, the latter being more
  interesting feature-wise and based on an open source core
- Migrated the project from mercurial to git as the latter is better known,
  and necessary as part of the GitLab migration
- Set up continuous integration (CI) and binaries deployment with gitlab-ci:
  Linux GCC, Linux Clang, Windows MinGW32, Windows MinGW64
- Removed obsolete bundled dependencies, both binaries and source code

**Engine:**

- Added support for installing and playing multiple campaigns
  * The current campaign can be changed in the level selection menu
- Migrated from SDL 1.2 to SDL 2 (#2), with in particular:
  * improved screen resolution detection and handling, especially in
    fullscreen
  * switched input method from keycodes to scancodes
- Dehardcoded game data loading, making proper system-wide installs possible
  on Linux (#21)
- Renamed debug mode to devtools and improved the corresponding documentation
  Devtools are toggled with `--devtools` switch or adding a `Devtools: 1`
  entry to the config.txt (#32)
- Added automatic progress saving upon level completion, and not only when
  closing the game.
- Use standard directories to store game config and user data (#22):
  * Linux: XDG_CONFIG_HOME (default `~/.config/lugaru`) and XDG_DATA_HOME
    (default `~/.local/share/lugaru`)
  * OSX: `~/Library/Application Support/Lugaru`
  * Windows: `%APPDATA%\Lugaru`, i.e. `C:\Users\<you>\AppData\Roaming\Lugaru`
- Add Linux installation instructions, together with desktop file, metainfo,
  icon and man page (#15, #27, #28)
- Improved command line options parsing and added `--help` output (#14)
- Fixed `nomousegrab`option (#5)
- Added Fullscreen parameter in the options GUI, and Alt+Return as hotkey to
  toggle fullscreen mode
- Increased default resolution from 640x480 to 1024x768 and bumped default
  visual details to the maximum level (#15)
- Added "stereo" anaglyph 3D display mode to play with 3D glasses
- Lots of code refactoring and modernisation, fixed warnings
- Lots of bug fixes
- Fixed compilation against modern compilers (GCC 5/6 - including MinGW - and
  Clang 3.8/3.9 so far)

**Game content:**

- All Wolfire assets are now licensed under CC-BY-SA 3.0 by David Rosen,
  making Lugaru fully free (relicensed in commit 51203b5)
- Added the Temple campaign by Silb to the main repo (licensed under CC-BY-SA
  3.0, http://forums.wolfire.com/viewtopic.php?f=7&t=1066)
- Fixed erroneous hardcoded paths to textures in some maps
