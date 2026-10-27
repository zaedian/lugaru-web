# Lugaru-Web Agent Instructions

## Project identity

Lugaru-Web is an existing preservation, modernization, and continuation project built on a long technical lineage:

Original Lugaru → later platform ports → OSS Lugaru → Ryan Deering/Loupgarenne → Ryan's unmerged WebAssembly work → Lugaru-Web.

Redagito/Lugaru is a separate reference implementation, not an ancestor or merge base.

Do not casually attribute legacy code or behavior to David Rosen unless repository history or historical evidence supports that attribution.

## Core working principle

Understand the artifact before changing it.

Prefer:

* preservation before cleanup;
* evidence before folklore;
* narrow fixes before broad refactors;
* verified behavior before architectural assumptions;
* feature-driven seams rather than speculative rewrites.

Strange legacy behavior, magic numbers, cross-system coupling, and apparently obsolete code may encode historical gameplay behavior. Do not remove or "clean up" such behavior without understanding its role.

## Evidence discipline

For archaeology or behavioral investigations, distinguish when useful between:

* `VERIFIED SOURCE FINDING`
* `RUNTIME-VERIFIED`
* `HISTORICAL EVIDENCE`
* `INFERENCE`
* `OPEN QUESTION`

Use exact filenames, functions, variables, constants, map values, and campaign data where possible.

When existing documentation and source appear to disagree, investigate the discrepancy rather than silently choosing one.

Use git history when authorship, provenance, regressions, or behavioral lineage matters.

## Existing project knowledge

`Docs/README.md` is the index for current Lugaru-Web documentation.

Use these documents contextually:

* `Docs/architecture.md`: current engine mental model, important couplings, gameplay/source archaeology, and extension seams.
* `Docs/research-backlog.md`: investigation log of open technical and historical questions, plus records of resolved investigations.
* `Docs/history-and-lineage.md`: project ancestry and historical attribution.
* `Docs/design-direction.md`: preservation/product principles and Classic/Expanded/Arena direction.
* `Docs/preservation-and-modding.md`: historical community content, licensing, provenance, and restoration.
* `Docs/build-provenance.md`: current build ancestry/provenance.
* `Docs/porting-notes.md`: browser-port implementation and validation notes.

Cite maintained findings from the documents above.

Do not reread every document for every task. Read the documents relevant to the work being performed.

Older campaign READMEs and `DEVTOOLS.txt` are historical/upstream documentation and should not automatically override current source or Lugaru-Web documentation.

## Rulesets and compatibility

Keep these concepts separate:

* content/campaign;
* gameplay rules;
* presentation.

Current conceptual gameplay rulesets are:

* Classic
* Expanded
* Arena

These are design boundaries, not instructions to implement separate engines.

Historical content should preserve Classic behavior unless a task explicitly concerns Expanded or Arena behavior.

Platform improvements such as input support, browser persistence, high-refresh correctness, accessibility, and crash fixes are not inherently Expanded gameplay features.

## Source changes

For bug fixes:

1. investigate the failure mechanism first;
2. identify the smallest relevant change;
3. avoid unrelated refactors;
4. preserve native and Classic behavior unless the task explicitly changes it;
5. build/test relevant targets;
6. report what was verified and what still requires manual/runtime testing.

Do not expand a narrow bug fix into a broad architecture rewrite without explicit authorization.

Temporary instrumentation is acceptable only when the maintainer explicitly authorizes it for the investigation.

## Git and repository safety

Do not:

* push commits;
* rewrite git history;
* force-push;
* delete branches;
* perform destructive resets;
* install system packages;
* make broad dependency changes;

unless explicitly authorized.

The maintainer normally controls final commits and pushes.

Generated directories such as `build-native`, `build-web`, and `build-web-debug` should be excluded from broad source searches unless build output itself is relevant.

## Browser/WebAssembly work

The known-good browser baseline is preserved by:

`baseline/wasm-b340204`

Do not assume every difference from historical Lugaru is caused by WebAssembly. Regressions may originate in OSS Lugaru, Loupgarenne, the WASM branch, or Lugaru-Web.

When investigating browser behavior, compare lineage when useful:

OSS → Loupgarenne → WASM baseline → current Lugaru-Web.

## Preservation

Finding historical content and having permission to redistribute it are separate questions.

Do not infer redistribution rights from public availability.

Do not replace unusual historical behavior or assets merely because a modern alternative appears cleaner.

## Communication

For substantial investigations:

* explain the discovered failure/behavior mechanism;
* identify supporting source/data;
* note uncertainty explicitly;
* propose the smallest next action.

Do not present hypotheses as established facts.
