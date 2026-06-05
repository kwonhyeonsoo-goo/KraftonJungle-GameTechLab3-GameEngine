# Game Jam Feature Patch Plan - Week14

## Purpose

Use Week12_JSEngine as a reference, but do not bulk-port it. Week14 has different
runtime boundaries, especially around `UGameViewportClient`, RmlUi widgets, FMOD,
reflection serialization, and project/world settings. The goal is to add small,
game-jam-friendly features on top of the current Week14 architecture and keep the
build verifiable after each batch.

## Implementation Status

- Batch 1 applied: Lua `Input` mode/cursor/capture API is bound through
  `UGameViewportClient`.
- Batch 2 applied: JSON `.prefab` save/spawn core exists via `FPrefabManager`,
  reusing `SceneSaveManager` actor/component serialization helpers.
- Batch 3 applied: Project/World default pawn prefab paths are serialized,
  exposed in settings UI, and consumed by `AGameModeBase` after existing pawn
  auto-possess fallback.
- Batch 4 partially applied: runtime UI now has Lua widget capture setters,
  element text/visible/enabled helpers, `data-action` event collection, and
  `UI.PollActionEvents()`. The important keyboard/text path is now covered by
  Batch 11; only advanced IME composition UI remains.
- Batch 5 applied: `UActorComponent` now has duplicate-safe `FName` component
  tags, scene/prefab round-trip support, Lua component tag API, and actor-local
  single-tag plus all-tags component lookup.
- Batch 6 applied: `FAudioManager::PlaySFX(pathOrKey, volumeScale)` and Lua
  `Audio.PlaySFX(pathOrKey, volumeScale)` exist.
- Batch 7 applied to a practical full-copy level: Project Settings now stores
  build/package validation options, the editor exposes a Packaging section,
  startup scene and default pawn prefab validation exist, and the build scripts
  route through `Scripts/PackageGame.ps1` for full-copy packaging, dry-run diff,
  `PackageManifest.json` generation, post-copy package smoke verification,
  package size reporting, and an optional launch smoke test. A cook/prune
  optimizer is still not implemented.
- Batch 8 applied: Lua now exposes `Json`, `Save`, and `Random` utility APIs,
  plus `Engine.Json`, `Engine.Save`, and `Engine.Random` aliases. Save paths are
  restricted to the project `Saves/` directory.
- Batch 9 applied: Lua now exposes read-only `Asset` query APIs, plus
  `Engine.Asset`, backed by Week14 `FAssetRegistry`.
- Batch 10 applied: runtime UI now has value, class, attribute, style, focus,
  blur, and synthetic click helpers on both `UUserWidget` and global `UI`.
- Batch 11 applied: Win32 completed text input is queued from `WM_CHAR` /
  `WM_UNICHAR`, Lua exposes `Input.ConsumeTextInput()`, and RmlUi receives
  keyboard plus text input through `ProcessKeyDown`, `ProcessKeyUp`, and
  `ProcessTextInput`.
- Input policy hardening applied: game scripts and `PlayerController` now read
  the policy-filtered `UGameViewportClient` `GameInputSnapshot`, invalid numeric
  virtual-key reads fail closed, invalid numeric `UInputComponent` mappings are
  ignored with a log, and focused RmlUi form controls automatically own text
  input plus block gameplay keyboard input. PIE ejected mode now stops the F8
  transition frame at the editor router and routes `PlayerController` input only
  while PIE is possessed.
- Scene API applied: Lua now exposes `Scene.Open`, `Scene.Load`,
  `Scene.TransitionTo`, `Scene.Reload`, `Scene.IsOpenPending`,
  `Scene.GetCurrentPath`, and `Scene.GetPendingPath`, plus `Engine.Scene`.
  Runtime transitions are queued through `UGameEngine` and missing files no
  longer destroy the active world first.
- Audio handle/group/3D API applied: `FAudioManager` now tracks SFX handles,
  separates BGM/SFX channel groups, exposes listener state and 3D SFX playback,
  and lets Lua stop/query/update handle-based sounds.
- Application/Debug Lua convenience applied: `Application.QuitGame`,
  `Application.Exit`, viewport/world-type queries, and `Debug.Log` /
  `Debug.Warn` / `Debug.Error` / `Debug.Assert` are available, plus
  `Engine.Application` and `Engine.Debug`.
- Actor Sequence runtime core applied: `PF_Animatable` reflection metadata,
  safe scalar channel read/write helpers, curve playback evaluation,
  `UActorSequence`, `UActorSequencePlayer`, and `UActorSequenceComponent`
  exist.
- Actor Sequence Batch 16 partially applied: actor-local component persistent
  GUIDs are saved on `UActorComponent`, Actor Sequence component bindings now
  resolve by persistent GUID first and component name second, `SequenceDataJson`
  refreshes binding caches before export, duplicate/player-owner lifecycle was
  tightened, and Lua exposes `ActorSequence`, `ActorSequencePlayer`, and
  `ActorSequenceComponent`. Actor JSON/prefab round-trip diagnostics now exist;
  dedicated editor UX, actual Lua-session self-test execution, and full scene
  save/load hostile QA remain next-batch work.

## Verified

- `Debug|x64` MSBuild succeeded after the package launch-smoke/size-report
  patch.
- `Game|x64` MSBuild succeeded after the package launch-smoke/size-report
  patch.
- `Scripts\python\python.exe Scripts\GenerateHeaders.py --self-test` succeeded
  after Actor Sequence reflection support.
- `Scripts\python\python.exe Scripts\GenerateHeaders.py --root KraftonEngine
  --dry-run` succeeded and detected the new Actor Sequence generated headers.
- `Debug|x64` and `Game|x64` MSBuild succeeded after the Actor Sequence runtime
  patch. Game still reports the existing PhysX/Vehicle static-library PDB
  `LNK4099` warnings.
- `Debug|x64` and `Game|x64` MSBuild succeeded after Actor Sequence persistent
  binding and Lua API wiring. Existing PhysX/Vehicle `LNK4099` PDB warnings
  remain.
- `Debug|x64` and `Game|x64` MSBuild succeeded after adding Actor Sequence
  round-trip diagnostics and the Lua debug entry point. Existing PhysX/Vehicle
  `LNK4099` PDB warnings remain.
- `Debug|x64` and `Game|x64` MSBuild succeeded after the input policy hardening
  patch. Game still reports the existing PhysX/Vehicle `LNK4099` PDB warnings.
- `Scripts/PackageGame.ps1 -Configuration Game -DryRun` succeeded.
- A real package smoke test to a temp directory succeeded and verified the
  manifest, file hashes, file sizes, required directories, and
  `Bin/KraftonEngine.exe`.
- Packaging now reports total size, top-level size breakdown, and largest files
  after a real package write.
- `Scripts/PackageGame.ps1 -LaunchSmokeTest` is available for optional packaged
  executable startup checks.
- A temp package with `-LaunchSmokeTest -LaunchSmokeTimeoutSeconds 1` succeeded;
  the packaged executable survived the timeout and was then stopped.
- The current full-copy Game package reports about `217.91 MB`, with `Content`
  at about `158.97 MB` and `Bin` at about `58.76 MB`.
- Existing warnings remain: PhysX/NvCloth PDB/link warnings. No new compile
  errors were observed.
- On this machine, MSBuild must be launched with duplicate `PATH`/`Path`
  environment variables removed, otherwise `CL.exe` fails before compilation.
- Build scripts pass `/nr:false` to MSBuild to avoid leaving reusable build
  nodes behind.

## Current Week14 Verdict

### Already Strong Enough For A Game Jam

- Lua input snapshot API plus input mode/cursor/capture controls.
- Lua `Input.ConsumeTextInput()` for completed UTF-8 text input.
- `UInputComponent` action/axis mapping and binding.
- Actor Tags in C++/Lua/Lua Blueprint/GameplayStatics.
- Component Tags for actor-local semantic lookup, including all-tags filtering.
- Actor/component tag editing uses a chip-style editor with add, remove, clear,
  and comma-paste support while preserving the existing serialized string field.
- Runtime RmlUi rendering, mouse/click/keyboard/text handling, widget capture
  flags, text/value setting, element state changes, class/attribute/style/focus
  helpers, and action event polling.
- FMOD-backed audio loading/playback/BGM/loop/master volume plus quick SFX play.
- Audio handles, `Audio.PlaySFX3D`, BGM/SFX group volumes, listener state, and
  stop/query/update helpers for one-shot sounds.
- Explicit SFX playback policy helpers for max concurrent sounds, cooldown,
  priority, and stop-oldest behavior.
- JSON prefab save/spawn core and Lua `World.SpawnActorFromPrefab(path)`.
- Project/World default pawn prefab override.
- Existing `GameBuild.bat`, `ReleaseBuild.bat`, and `PackageRelease.bat`.
- Editor Project Settings Packaging section with package validation and
  one-click script launch plus dry-run launch.
- Full-copy package manifest generation for `Bin`, `Shaders`, `Content`, and
  `Settings`.
- Post-copy package smoke verification for manifest consistency, hashes, sizes,
  required directories, and executable presence.
- Optional launch smoke test for packaged executable startup checks.
- Package size report with top-level breakdown and largest packaged files.
- Lua `Json.Encode` / `Json.Decode`, `Save.WriteText` / `ReadText` /
  `WriteJson` / `ReadJson`, and deterministic `Random` helpers.
- Lua `Asset.List(typeName)`, `Asset.GetPaths(typeName)`, typed asset path
  helpers, `Asset.Find(typeName, nameOrPath)`, and `Asset.Exists(...)`.
- Lua `Scene.Open(pathOrName)`, `Scene.Reload()`, and pending/current scene
  queries for runtime map flow.
- Lua `Application` and `Debug` convenience tables for quit, viewport/world-type
  queries, logging, and non-throwing assertions.
- Actor Sequence runtime playback core exists for owner-actor and actor-local
  component property animation, but it still needs editor/Lua/persistent-guid
  polish before it is a comfortable jam authoring workflow.

### Thin But Usable

- Runtime UI action events are simple string events from `data-action` or
  `action` attributes. They are useful for menus, but not a full reactive UI
  binding system.
- Packaging is script-backed and now has dry-run, manifest support, size
  reporting, package smoke checks, and optional launch smoke. It is suitable for
  game-jam distribution, but it is still a full-copy package rather than a
  dependency-cooked/pruned package.

### Still Missing

- Advanced IME composition/candidate positioning. Completed IME text input via
  `WM_CHAR` is now queued, but composition-window management is not implemented.
- Week12-style per-event RmlUi input consumption is not fully ported. Week14
  currently separates UI and game input through `UGameViewportClient` using
  widget capture/block flags plus RmlUi form-control focus, but RmlUi return
  values from mouse/key events are not yet fed back into a same-frame
  `InputRouter`/`GameInputBridge` policy.
- Actor Sequencer editor UX. Week14 now has an Actor Sequence runtime core, but
  not the dedicated editor workflow that Week12 had for authoring tracks, keys,
  sections, preview, and embedded curves.
- Global world-wide component tag search. This is intentionally deferred.
- Advanced audio mixing policy such as BGM ducking, category routing presets,
  and designer-authored sound banks.
- Rich packaging optimizer: asset dependency cook, stale asset pruning rules,
  and package-size trimming rules. Size reporting and optional launch smoke are
  now available.
- Advanced Week12-style runtime helpers are now mostly covered. See
  `Docs/LuaAPIBacklogPlan.md` for the smaller remaining policy/packaging items.

## Actor Tags And Component Tags Decision

Actor Tags are already present and useful. They should remain actor-level
classification: enemy, player, pickup, boss, objective, damageable.

Component Tags are worth adding, but only as actor-local lookup for now. They are
best used for semantic parts inside a prefab: `Hitbox`, `Muzzle`, `WeakPoint`,
`InteractPrompt`, `CameraSocket`, `FootstepSource`.

Do not add global component tag search in the first pass. It invites per-frame
world scans and ambiguous ownership. If global lookup becomes necessary, add it
later with explicit indexing or a narrow gameplay subsystem.

## Week12 Reference Gap

Week12 has richer systems in these areas:

- UI: `FRmlUiSystem` supports element value/class/attribute/style/focus helpers,
  keyboard events, text input, and `PollActionEvents`.
- Audio: `AudioSystem` supports `PlaySFX`, `PlaySFX3D`, BGM/SFX volume groups,
  and audio handles. Week14 now covers this core surface, but not higher-level
  concurrency policy.
- Packaging: `FGameBuildSettings`, `FGamePackager`, packaging UI, startup scene
  checks, and package copy/cook logic exist. Week14 now has the UI/settings
  wrapper, validation, full-copy package script, dry-run diff, manifest
  generation, size reporting, package smoke checks, and optional launch smoke,
  but not dependency cooking/pruning.
- Tags: component tags and actor-local single-tag/all-tags lookup helpers exist.
- Input: runtime input policy work and controller-facing cursor/input-mode
  helpers exist.
- Utility API: Week12 exposes `Json`, `Save`, `Random`, `Asset`, `Scene`,
  `Application`, and debug helpers through a modular `Engine` API surface.
  Week14 now covers this core set.

Week14 now covers the most immediately useful subset: prefab spawn, default pawn
prefab, component tags, quick SFX, Lua input controls, runtime UI manipulation
and text input, script-backed packaging UX, and Lua save/json/random/asset/scene
helpers. Week14 is still behind Week12 in dependency-cooked packaging, audio
routing policy, Actor Sequence tooling, and advanced IME polish.

## Actor Sequence Next Batch Candidate

Reference files inspected from `Week12_JSEngine`:

- `Source/Engine/Animation/ActorSequence.h/.cpp`
- `Source/Engine/Animation/CurvePlayback.h`
- `Source/Engine/Animation/TimelinePlayer.h/.cpp`
- `Source/Engine/Component/ActorSequenceComponent.h/.cpp`
- `Source/Editor/UI/EditorActorSequencerWidget.*`
- `Source/Editor/UI/EditorActorSequenceEditModel.*`
- `Source/Editor/UI/EditorActorSequenceDetails.*`
- `Source/Editor/UI/EditorActorSequenceTimeUtils.h`
- `Source/Editor/UI/EditorCurveEditorWidget.*`
- Lua definitions for `ActorSequenceComponent`, `ActorSequence`, and
  `ActorSequencePlayer`.

### What Week12 Actually Has

- `UActorSequenceComponent` is a spawnable actor component that owns an embedded
  `UActorSequence` plus runtime and editor-preview `UActorSequencePlayer`
  objects.
- `UActorSequence` stores bindings to the owning actor or its components,
  tracks, sections, channels, curve asset paths, and inline curve key data.
- `UActorSequencePlayer` resolves bindings by persistent component guid first,
  then component name, caches base property values, evaluates curve playback,
  and writes animatable scalar property channels.
- Playback supports autoplay, looping, play rate, pause-at-end, start offset,
  play/pause/stop, preview play/pause/stop, and scrubbing by time.
- Lua exposes `ActorSequenceComponent:Play/Pause/Stop`,
  `GetSequence`, `GetSequencePlayer`, `AddFloatTrack`, and player
  `Play/Pause/Stop/SetCurrentTime/GetCurrentTime/IsPlaying`.
- Editor support is not trivial: the property panel embeds sequence details,
  Actor Sequencer opens as a document/tab, curve editor can edit embedded
  sequence curves, and timeline UI supports property track/key editing.
- The dedicated editor is split into several responsibilities:
  `FEditorActorSequencerWidget` owns the document/tab UI and timeline
  interactions, `FEditorActorSequenceEditModel` owns target/property/key/undo
  operations, `FEditorActorSequenceDetails` owns component-level details, and
  `FEditorCurveEditorWidget` provides embedded-curve editing plus reference
  preview support.
- `FEditorActorSequencerWidget` is featureful: open/reset target, toolbar,
  add-track popup, add-property popup, pinned components, selected tracks,
  selected keys, playback range dragging, section start/end dragging, key
  dragging, context menus, timeline scrolling, and scrub/play preview controls.
- `FEditorActorSequenceEditModel` is the important safety layer. It validates
  that the sequence component is live, resolves owner/component bindings,
  collects only animatable scalar properties, maps property type to channel
  names, creates tracks/curves, adds keys from current property values, moves
  and deletes keys, deletes tracks, changes apply/time-mapping modes, resizes
  sections/playback range, and captures sequence undo state.
- `FEditorActorSequenceDetails` wraps the details-panel workflow: begin edit
  undo, mark edited, commit edit undo, and render sequence component controls.
- Week12's `FEditorCurveEditorWidget` has extra Actor Sequence behaviors that
  Week14's current `FFloatCurveEditorWidget` does not fully cover yet:
  opening embedded sequence curves, embedded rendering, actor-sequence source
  labels, reference preview targets, sequence-reference detection, dirty/save
  callbacks for embedded curves, and curve undo capture/commit/cancel.
- Reference inspection confirms that this is a dedicated component-sequence
  editor, not a standalone asset editor. It should open from an
  `UActorSequenceComponent`, keep editing the embedded `UActorSequence`, and
  use the selected actor as its preview/transaction source.
- Week12 and Week14 data names do not match one-to-one. Week12 editor code uses
  names such as `TargetObjectGuid`, `PropertyPath`, `EActorSequenceTrackType::Vec3`,
  `Color`, and `Transform`; Week14 currently uses `TargetComponentGuid`,
  `PropertyName`, and `Scalar/Vector3/Rotator/Vector4`. Porting must include a
  deliberate compatibility pass instead of direct copy/paste.

### Week14 Fit Check

- Week14 already has persistent component guids, prefab/scene component
  serialization, property panels, reflection property metadata, animation
  timelines, Lua property get/set helpers, an asset-editor document manager,
  and a `FFloatCurveEditorWidget` registered for curve assets.
- Week14 now has `FProperty::IsSequencerScalar`, `ReadScalarChannelValue`, and
  `WriteScalarChannelValue`. Future editor/Lua work should go through these
  helpers rather than introducing ad-hoc property writes.
- Week14's prefab system must serialize the embedded sequence object through
  the component's existing reflection/archive path, not through a second custom
  prefab format.
- Do not bulk-copy Week12 editor UI blindly. Port the data/runtime layer first,
  then adapt the editor UI to Week14's current panel/tab/property architecture.
  However, the dedicated Actor Sequencer editor is not optional if Actor
  Sequence is expected to be designer-usable during the jam.
- Prefer extending Week14's existing `FFloatCurveEditorWidget` for embedded
  actor-sequence curve editing if the extension stays clean. Only introduce a
  separate Week12-style `FEditorCurveEditorWidget` if the existing asset editor
  cannot support embedded sequence curves, reference preview, and source-aware
  undo without becoming tangled.

### Batch 15 - Actor Sequence Runtime Core Applied

Applied:

- Added curve playback descriptors/evaluator.
- Extended `FProperty` with a safe animatable scalar channel API:
  `IsSequencerScalar`, `ReadScalarChannelValue`, and
  `WriteScalarChannelValue`.
- Added `PF_Animatable` reflection support and `Animatable` metadata parsing in
  `Scripts/GenerateHeaders.py`.
- Marked actor transform/visibility and scene-component relative
  location/scale/rotation edit fields as animatable.
- Added `UActorSequence`, `UActorSequencePlayer`, and
  `UActorSequenceComponent`.
- Added owner-actor and actor-local component bindings. The first runtime pass
  started name-based; Batch 16 now upgrades component bindings to persistent
  GUID first, component name second.
- Added inline float-curve JSON export/import on the component's
  `SequenceDataJson` property so reflection scene/prefab serialization has a
  single component-owned data source.
- Added runtime and preview players, auto-play, loop, play-rate, start-offset,
  pause-at-end, current-time, play/pause/stop, base-value restore on explicit
  stop, and stale-target invalidation guards.
- Added the new files to `KraftonEngine.vcxproj` and filters.

Original task checklist:

- Add curve playback descriptors/evaluator if Week14 lacks an equivalent
  reusable layer.
- Extend `FProperty` with a safe animatable scalar channel API:
  `IsSequencerScalar`, `ReadScalarChannelValue`, and
  `WriteScalarChannelValue`.
- Add an explicit `Animatable` metadata/flag path or metadata fallback for
  properties that should be sequence-editable.
- Add `UActorSequence`, `UActorSequencePlayer`, and
  `UActorSequenceComponent`.
- Support owner-actor and actor-local component bindings only. Keep world/global
  bindings out of the first pass.
- Preserve base values on stop and when resolved targets disappear.
- Ensure runtime playback ticks through the normal component tick path and
  editor preview tick stays separate from game playback.

Validation status:

- Done: `Debug|x64` and `Game|x64` build.
- Done: header generator self-test and dry-run.
- Still needs editor/runtime QA: add an `ActorSequenceComponent` to an actor and
  play/pause/stop without crashes.
- Still needs editor/runtime QA: animate at least one float property on an
  actor-local component.
- Still needs editor/runtime QA: stop restores the cached base value.
- Still needs editor/runtime QA: destroying/removing a target component
  invalidates the track safely instead of dereferencing stale objects.

### Batch 16 - Actor Sequence Persistent Binding And Lua Partially Applied

Applied:

- Added a saved `PersistentGuid` field to `UActorComponent`, generated on
  save/load for older or newly created components.
- Added `TargetComponentGuid` to Actor Sequence component bindings.
- Runtime resolution now looks up actor-local component bindings by persistent
  GUID first and component name second.
- Sequence export refreshes owner/component binding caches before writing
  `SequenceDataJson`, so legacy name-only bindings can migrate when the target
  component is present.
- `UActorSequenceComponent` now refreshes player owner state on load, duplicate,
  runtime play, and preview play/scrub paths to avoid stale/null-owner players.
- Lua now exposes:
  `ActorSequence:GetDuration/SetDuration/Clear/ExportToJsonString/ImportFromJsonString`,
  `ActorSequencePlayer:Play/Pause/Stop/SetCurrentTime/GetCurrentTime/IsPlaying/IsPaused`,
  and `ActorSequenceComponent:Play/Pause/Stop/GetSequence/GetSequencePlayer/AddFloatTrack`.
- Lua `ActorComponent:GetPersistentGuid()`,
  `Object:AsActorSequenceComponent()`, and
  `Actor:GetActorSequenceComponent()` are available.
- `ActorSequenceComponent:AddFloatTrack(desc)` accepts compact script keys such
  as `target`, `property`, `channel`, `start`, `duration`, and `curve`, plus
  C++-style key names. `target` may be `"Owner"`, a component name, or a
  component persistent GUID.
- Guarded `LuaScriptManager.cpp` against the Win32 `GetCurrentTime` macro so
  `ActorSequencePlayer:GetCurrentTime()` binds to the engine method rather than
  `GetTickCount`.
- Added `FActorSequenceDiagnostics::RunRoundTripSelfTest()` and Lua
  `Debug.RunActorSequenceRoundTripSelfTest()`. The self-test creates a source
  actor, adds a component-bound `Location.X` sequence with inline curve keys,
  serializes actor JSON, spawns from serialized actor, saves/spawns a prefab,
  validates persistent component GUID binding, applies playback, and verifies
  stop/base-value restore.

Remaining tasks:

- Run the new Lua diagnostics entry in an actual editor/game session and record
  returned `Passed`, `ChecksRun`, and `Message`.
- Extend diagnostics from actor JSON/prefab round-trip to full scene save/load
  round-trip once the scene test entry point is chosen.
- Verify loop/start offset/pause-at-end flags from a real saved scene or prefab
  authored through the future editor UI.
- Add Lua definitions/examples after the C++ binding surface is stable.

Validation:

- `local r = Debug.RunActorSequenceRoundTripSelfTest()` returns
  `r.Passed == true`.
- Save a scene containing an actor sequence, reload it, and play the sequence.
- Save that actor as a prefab, spawn the prefab, and play the sequence.
- Lua can trigger a sequence on a spawned prefab.
- Invalid Lua descriptors return `false`/`nil` and log a useful warning instead
  of crashing.

### Proposed Batch 17 - Actor Sequencer Editor UX And Polish

Tasks:

- Add a Week14-native dedicated Actor Sequencer editor that opens from
  `UActorSequenceComponent` details and can live as a document/tab, matching the
  current editor document workflow.
- Port/adapt the Week12 editor model as a first-class layer, not as UI glue:
  `IsSequenceComponentLive`, `GetLiveOwner`, `CollectAnimatableScalarProperties`,
  binding resolution, component labels, property-to-track/channel mapping,
  add-track, add-key-from-current-value, move/delete key, delete track,
  section/range resize, apply mode, time mapping mode, curve creation, and undo
  notification.
- Add a details-panel section for `UActorSequenceComponent`: open sequencer,
  runtime play/pause/stop, preview play/pause/stop, current time, duration,
  playback range, auto play, pause at end, loop, play rate, start offset, and
  clear/reset sequence actions.
- Port the dedicated sequencer UI interactions: toolbar play/pause/stop,
  scrubber, current time display, duration/range display, add-track popup,
  add-property popup, owner actor row, actor-local component rows, pinned
  components, timeline ruler, sections, keys, selected track/key state, section
  start/end drag, playback range drag, key drag, delete key/track context menus,
  add key at current time, zoom/view range, and vertical track scrolling.
- Add property picker filtering that shows only `PF_Animatable` properties
  accepted by `FProperty::IsSequencerScalar()`. Avoid presenting read-only,
  transient, object-reference, string, array, or unsupported struct properties.
- Add channel picker support for scalar/vector-like properties: `Value`,
  `X/Y/Z`, `Pitch/Yaw/Roll`, and `R/G/B/A` where the runtime scalar channel API
  supports them.
- Integrate curve editing for embedded sequence curves. First try extending
  Week14's `FFloatCurveEditorWidget` with an embedded-source mode; otherwise
  port the Week12 `FEditorCurveEditorWidget` responsibilities needed for
  Actor Sequence: open embedded curve, render embedded, source label,
  reference preview, dirty state, save callback, reload, key list, canvas,
  key/tangent dragging, add/delete key, interpolation/tangent mode editing, and
  fit-to-keys.
- Connect curve edits back to the sequence component so scene/prefab dirty
  state and undo state are captured once per edit gesture, not every frame.
- Add editor preview isolation: preview playback/scrubbing must use the
  component's preview player and must not start runtime playback or permanently
  leave animated values on the actor when the editor is closed.
- Add close/target-invalid handling: if the selected actor/component is deleted,
  hidden by scene reload, or the component loses its owner, the editor clears
  its target and stops preview without crashing.
- Reuse Week14 undo/property-change paths where they already exist. If the
  transaction path cannot safely serialize sequence data yet, gate undo behind
  a minimal actor-state capture like Week12 and document the limitation.
- Add small UI polish: stable panel widths, no text clipping, disabled states
  for invalid selections, tooltips for apply/time-mapping modes, visible
  selected-track/key feedback, and clear empty states for "no sequence",
  "no animatable properties", and "no keys".

Dedicated editor port checklist:

- `FEditorActorSequenceEditModel` first:
  adapt the Week12 safety layer to Week14's current runtime names. It must use
  `TargetComponentGuid`, `PropertyName`, `EActorSequenceTrackType::Scalar`,
  `Vector3`, `Rotator`, and `Vector4`, and it must call
  `FProperty::ReadScalarChannelValue/WriteScalarChannelValue` for validation.
- `FEditorActorSequenceDetails` second:
  embed component controls in the existing Week14 property panel, with undo
  begin/commit around autoplay, loop, pause-at-end, play rate, start offset,
  duration, current time, clear sequence, preview, and open-sequencer changes.
- `FEditorActorSequencerWidget` third:
  port the timeline UI after the model compiles. Keep Week12 interactions:
  toolbar icons, add-track and add-property popups, owner/component target rows,
  pinned components, ruler, playback range handles, sections, keys, selection,
  drag/drop edits, context menus, scrub, preview play/pause/stop, and scroll.
- Curve editor integration fourth:
  try extending Week14 `FFloatCurveEditorWidget` with an embedded source context
  before introducing a parallel Week12 `FEditorCurveEditorWidget`. Required
  source context fields are component pointer, binding/track/section/channel
  handle, label, preview target, dirty callback, undo begin/commit/cancel, and
  restore-on-close behavior.
- Document/tab integration:
  use Week14 `FEditorDocumentTabManager` style behavior so the sequencer can be
  reopened, focused, closed, and invalidated without duplicate windows or stale
  actor/component pointers.
- Transaction policy:
  reuse Week14 undo if it can serialize the owning actor plus embedded
  `SequenceDataJson`. If not, port Week12's actor-state snapshot fallback and
  make the UI honest about any edit type that cannot undo yet.
- Preview policy:
  preview and scrub must use `GetPreviewSequencePlayer()`/`SetPreviewTime()` and
  must stop/restore on close, target deletion, scene reload, and component
  removal.
- Polishing target:
  no clipped labels in track rows, disabled buttons for invalid selections,
  clear empty states, deterministic row heights, visible selected key/section
  states, helpful tooltips for Absolute/Additive/Multiply and Seconds/Normalized
  mapping, and stable behavior when the edited actor is duplicated or prefab
  spawned.

Validation:

- Existing property panel still edits ordinary components.
- Actor Sequencer opens from `UActorSequenceComponent` details as a document/tab
  and can be reopened without duplicating stale editor instances.
- Add an animatable float track on the owner actor and on an actor-local
  component.
- Add vector/rotator/color channel tracks where supported by the scalar channel
  API.
- Add key, drag key, delete key, resize section, resize playback range, change
  apply mode, change time-mapping mode, scrub, preview play, pause, and stop.
- Curve editor opens for an embedded sequence curve; add/delete key, drag key,
  edit tangents/interpolation, save/apply, reload/cancel where supported, and
  return to the sequencer without losing selection.
- Preview stop/editor close restores cached base values.
- Scene/prefab save after editor sequence edits persists exactly the edited
  data, including inline curve keys and component persistent-guid bindings.
- Undo/redo either works for sequence edits or is explicitly disabled for the
  first pass with no false UI affordance.
- Deleting the target component while the sequencer is open clears the editor
  target safely.
- Detached/docked editor window behavior remains unchanged.

Suggested split if Batch 17 is too large:

- Batch 17A: Details-panel entry point plus `FEditorActorSequenceEditModel`
  port/adaptation, with no complex timeline drawing yet.
- Batch 17B: Dedicated sequencer document/tab, add-track/property/key workflow,
  playback controls, scrubbing, selection, context menus, and section/key drag.
- Batch 17C: Embedded curve editor integration, reference preview, source-aware
  dirty/save/undo, and curve key/tangent polish.
- Batch 17D: UX polish and hostile editor QA: invalid target handling, empty
  states, disabled controls, docking/reopen behavior, undo/redo verification,
  scene/prefab round-trip from editor-authored data, and build verification.

### Component Multi-Tag Lookup Applied

Added:

- `AActor::FindComponentByTags(...)` and `AActor::FindComponentsByTags(...)`.
- Lua `actor:FindComponentByTags(...)` / `actor:GetComponentByTags(...)`.
- Lua `actor:FindComponentsByTags(...)` / `actor:GetComponentsByTags(...)`.
- Lua calls accept either `actor:FindComponentsByTags("Hitbox", "WeakPoint")`
  or `actor:FindComponentsByTags({ "Hitbox", "WeakPoint" })`.

Policy:

- Multi-tag lookup means all requested tags must be present on the component.
- Lookup remains actor-local. Do not add global component tag scans without an
  explicit indexed gameplay use case.

### Tag-List Editor Polish Applied

Added:

- Actor and component `Tags` properties render as a chip-style list in the
  property panel.
- Existing tags can be removed by clicking their chip.
- New tags can be added one at a time or pasted as a comma-separated list.
- `Clear` removes the whole tag list.

Implementation note:

- This intentionally keeps the existing `PendingTagsString` storage path, so
  scene/prefab serialization and `PostEditProperty` tag synchronization stay
  unchanged.

## Remaining Batches

### Batch 4 Remainder - UI Keyboard/Text Applied

Added:

- Platform completed-text input queue using `WM_CHAR` / `WM_UNICHAR`.
- `InputSystem::ConsumeTextInput()` for RmlUi and
  `InputSystem::ConsumeScriptTextInput()` for Lua.
- Lua `Input.ConsumeTextInput()` returning UTF-8 text.
- RmlUi `ProcessKeyDown`, `ProcessKeyUp`, and `ProcessTextInput` forwarding.
- Lua helpers for value/class/attribute/style/focus.

Still avoid:

- Fake Korean/IME support by composing keycodes manually.
- A second input router separate from `UGameViewportClient`.

### Batch 7 - Packaging Pipeline Applied

Added:

- `Scripts/PackageGame.ps1` as the shared full-copy packager.
- `GameBuild.bat`, `ReleaseBuild.bat`, and `PackageRelease.bat` now call the
  shared packager after building.
- Build scripts pass `/nr:false` to MSBuild to avoid persistent node reuse.
- Dry-run diff mode reports added/updated/unchanged/deleted package files.
- `PackageManifest.json` records relative path, source, size, and SHA-256.
- `Play.bat` and `BuildInfo.txt` are generated by the shared packager.
- Editor Packaging section can launch package dry-run.
- Non-dry-run packages run a smoke verification over the manifest, required
  directories, file sizes, hashes, and packaged executable.
- Non-dry-run packages print total package size, top-level breakdown, and the
  ten largest packaged files.
- `-LaunchSmokeTest` starts the packaged executable and fails if it exits early
  with a non-zero code. Surviving the timeout is considered a startup pass.
- Project Settings can pass `--launch-smoke` and `--launch-smoke-timeout` to
  `PackageRelease.bat`.

Still later:

- Asset dependency cook/prune rules.
- Package-size trimming.
- A C++ `FGamePackager` only if script-backed packaging becomes limiting.

### Audio Handle/Group/3D Applied

Added:

- Handle-returning one-shot playback with `Audio.PlaySFXHandle(...)`.
- 3D one-shot playback with `Audio.PlaySFX3D(...)`.
- `Audio.SetListener(position, forward, up)`.
- BGM/SFX group volume setters and getters.
- Handle control: `StopSound`, `StopAllSounds`, `IsSoundPlaying`,
  `SetSoundVolume`, `SetSoundPitch`, and `SetSoundPosition`.

### Audio Playback Policy Applied

Added:

- `Audio.SetSFXPolicy(pathOrKey, maxConcurrent, cooldownSeconds, priority,
  stopOldest)`.
- `Audio.ClearSFXPolicy(pathOrKey)` and `Audio.ClearAllSFXPolicies()`.
- `Audio.GetActiveSoundCount(pathOrKey)`.
- Policy is opt-in per sound key/path. Sounds without a policy keep the old
  fire-and-forget behavior.

Policy behavior:

- `maxConcurrent <= 0` means unlimited.
- `cooldownSeconds` is checked against the last successful playback of that
  sound.
- When the concurrent cap is full, `stopOldest=true` replaces the oldest active
  sound whose priority is less than or equal to the new request.

Still avoid:

- Pretending this is a full mixer authoring layer. BGM ducking, bus routing
  presets, and designer-authored sound banks should stay separate.

### Application/Debug Convenience Applied

Added:

- `Application.QuitGame()` / `Application.Exit()`.
- `Application.GetViewportSize()`, `Application.GetWorldType()`,
  `Application.IsGame()`, and `Application.IsEditor()`.
- `Debug.Log(...)`, `Debug.Warn(...)`, `Debug.Error(...)`, and
  `Debug.Assert(condition, message)`.

Still avoid:

- A separate logging subsystem just for Lua. These wrappers intentionally route
  into the existing engine logger.

### Batch 8/9 Remainder - Lua API Backlog

See `Docs/LuaAPIBacklogPlan.md` for the detailed Week12 comparison and proposed
batch order. Short version:

- Add `Scene` API only after Week14 has a safe runtime scene-transition request
- `Scene` API is now applied on top of Week14's queued runtime transition path.
- Add richer `UI` APIs and `Input.ConsumeTextInput()` together, because text
  fields need both RmlUi keyboard forwarding and OS text/IME events.
- Deeper `Audio` handle/group/listener APIs are now represented in Week14.

## Recommended Next Work

1. Build the game prototype using the newly available prefab, default pawn,
   component tag, UI action, and SFX APIs.
2. If menus or text fields become central, implement the Batch 4 text input path.
3. Use `PackageRelease.bat <VersionName> --dry-run` before final packaging to
   inspect package changes without writing files.
4. Use `PackageRelease.bat <VersionName> --launch-smoke --launch-smoke-timeout 5`
   when you want the final package to prove the exe survives startup.
5. Use `Save.WriteJson` / `Save.ReadJson` for prototype settings, progress, and
   high-score data.
6. Use `Asset.GetRmlDocumentPaths()` / `Asset.GetSoundPaths()` for data-driven
   menus and random content selection.
7. Use `Scene.Open("Stage01")` / `Scene.Reload()` for level flow instead of
   destroying or loading worlds directly from gameplay callbacks.
8. Use `Audio.PlaySFX3D(...)` for spatial gameplay cues, and keep
   `Audio.PlaySFX(...)` for fire-and-forget UI/arcade sounds.
9. Use `Debug.Assert(...)` in prototype Lua scripts for bad data checks that
   should be visible but should not crash the whole run.
10. Keep global component tag search deferred until there is an indexed gameplay
   system that actually needs it.

## Quick Lua Examples

```lua
Input.SetInputModeGameAndUI()
Input.SetCursorVisible(true)

local controller = World.GetFirstPlayerController()
if controller then
    controller:SetInputModeGameAndUI()
end

local enemy = World.SpawnActorFromPrefab("Content/Prefab/Enemy.prefab")
local hitbox = enemy and enemy:FindComponentByTag("Hitbox")
local weakHitboxes = enemy and enemy:FindComponentsByTags("Hitbox", "WeakPoint")

Audio.PlaySFX("Explosion.wav", 0.8)
Audio.SetSFXPolicy("Explosion.wav", 4, 0.05, 0, true)
local sfx = Audio.PlaySFX3D("Explosion.wav", enemy:GetActorLocation(), 1.0)
Audio.SetSFXVolume(0.75)

Save.WriteJson("player_state.json", { hp = 100, coins = 3 })
local state = Save.ReadJson("player_state.json")
local roll = Random.RandomInt(1, 6)
local menuDocs = Asset.GetRmlDocumentPaths()
Scene.Open("Stage01")
Debug.Log("world:", Application.GetWorldType())

local menu = UI.CreateWidget("Content/UI/MainMenu.rml")
menu:AddToViewport()
menu:SetWantsMouse(true)
menu:SetBlocksGameInput(true)
menu:SetActionEvent("startButton", "StartGame")
menu:SetValue("nameInput", "Player")
menu:SetClass("startButton", "highlight", true)
UI.SetStyle("healthBar", "width", "75%")

for _, eventName in ipairs(UI.PollActionEvents()) do
    if eventName == "StartGame" then
        menu:RemoveFromParent()
        Input.SetInputModeGameOnly()
    end
end
```

## Guardrails

- Do not bulk-port Week12.
- Do not replace Week14 serialization, material, asset package, or renderer
  formats as part of this feature patch.
- Keep input source of truth in `UGameViewportClient`/`InputSystem`.
- Keep prefab format as JSON `.prefab` for now.
- Keep component tag lookup actor-local until a real indexed global use case
  appears.
