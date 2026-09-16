# Research: Alien Invasion Pinball MVP

Date: 2026-09-15. Research complete; decisions below resolve the planning unknowns.
These are project design decisions, not claims that the engine automatically provides the
session guarantees. No application code, assets, build, or runtime validation was performed.

## 1. Engine and toolchain baseline

**Decision**: Target installed Unreal Engine 5.8.2, Windows x64, C++20 and Blueprints.
Use Visual Studio 2026 with Game Development with C++, MSVC 14.50.35723 or a later supported
14.50 patch, and Windows SDK 10.0.22621.0 or newer supported by this engine. M1 verifies the
installed compiler by building a blank project; do not assume toolchain installation from
engine presence alone.

**Evidence**: Local `Engine/Build/Build.version` reports 5.8.2, changelist 56702186.
`Engine/Config/Windows/Windows_SDK.json` prefers MSVC 14.50 and bans 14.50 through 35722.
Epic documents VS2026 support for UE5.8 in its
[Visual Studio setup guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine).

**Rationale**: Use the installed release and its own supported compiler configuration.
**Alternatives considered**: An older minor release adds migration and installation work;
a custom source-engine build is unnecessary.

## 2. Minigame environment architecture

**Decision**: A persistent non-World-Partition cabinet world contains the table. Each minigame
is authored as its own small streamed sublevel. Preload all three asynchronously during BOOT,
add/show them in separate isolated arena slots, validate their registered runtime roots, and
keep them resident but explicitly dormant until selected. Normal runs do not load/unload a
map or change worlds. Actors start dormant even if BeginPlay has run. The complete readiness
barrier is described in [transition protocol](contracts/transitions.md).

| Criterion | Separate map travel | Streamed sublevels, resident after boot | Spawned arena Actor hierarchy |
| --- | --- | --- | --- |
| Preserve table | Reconstruct Actors and physics state on return | Same live table and session | Same live table and session |
| Suspend physics | Table world is replaced; return still needs reconstruction | Explicit table-local suspension required | Same explicit suspension required |
| Transition speed | Map/load and reconstruction costs per trip | Boot prewarm absorbs load; runtime activation remains measurable | Fast spawn if assets preloaded; actor creation can still hitch |
| Input/camera | Recreate/rebind world-dependent references | One controller; explicit possession, view, context transaction | Same controller transaction |
| Future cabinets | Strong environment separation but duplicated restoration work | Cabinet data selects level assets and arena transforms | Cabinet selects prefab classes; authoring becomes more nested |
| Future minigames | Own world settings and GameMode | Independent maps but shared world/GameMode rules | Scene-level lighting/layout authoring less convenient |
| Cost | Persistence complexity for physical table | Dormancy, readiness, cleanup and bounded resident memory | Increasing Blueprint hierarchy and spawn ownership complexity |

**Rationale**: State preservation and predictable repeated transitions outweigh the memory
cost of three deliberately small arenas. Streamed levels are content boundaries, not separate
GameModes or physics worlds. Ambient lights, audio, and post-process effects must be isolated;
no arena owns unbounded global lighting or post-process settings. Each root explicitly owns
its environment presentation and run Actors. Measured memory pressure could justify selective
residency later without changing the lifecycle/result contract; do not build an eviction
framework for this MVP.

**Alternatives considered**: OpenLevel travel is rejected within a session; spawned-only
arenas are useful for the test stub but not the primary authoring approach; World Partition
and multiple independent UWorlds add no value at this scale.

Epic describes persistent/sublevel composition and independent loading/visibility in
[Level Streaming Overview](https://dev.epicgames.com/documentation/unreal-engine/level-streaming-overview-in-unreal-engine).
The distinction between loaded, shown, hidden and unloaded is documented in
[ULevelStreaming](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ULevelStreaming).
Using those mechanisms with explicit dormancy and a readiness barrier is our design inference.

## 3. Framework ownership

**Decision**: GameModeBase owns session rules through a flow Actor Component. GameStateBase
exposes session state, with a scoring Actor Component as the sole score owner. PlayerController
owns input, possession, camera selection and UI. A single UWorldSubsystem owns minigame level
residency and the active run. Stock GameInstance and GameUserSettings suffice; do not put the
current score or ball in application-lifetime globals. Use one runtime module with clean
folders rather than a plugin/module for each minigame.

**Rationale**: Lifetimes match Unreal's existing framework; no generic manager Actor hierarchy
or duplicate global state machine is needed. A world-scoped subsystem survives minigame
sublevel operations and is torn down with its world.
**Alternatives considered**: A GameInstance manager would retain stale world references;
GameMode alone would accumulate presentation and loading responsibilities; distinct GameModes
per streamed arena would not provide independent game rules in the shared world.

Grounded in Epic's [GameMode and GameState](https://dev.epicgames.com/documentation/unreal-engine/game-mode-and-game-state-in-unreal-engine)
and [subsystem lifetime documentation](https://dev.epicgames.com/documentation/unreal-engine/programming-subsystems-in-unreal-engine).

## 4. Physics and table suspension

**Decision**: Chaos rigid-body ball and constrained flippers, simple thick collision,
CCD on the ball, physics materials, and tunable bounded speeds. Start substepping evaluation
at 1/120-second maximum substep and eight maximum substeps; validate at 30/60/120 FPS rather
than promising deterministic simulation. Use a limited single flipper hinge axis with
Twist/Swing drive, not SLERP with locked axes. No custom physics solver.

Table suspension first closes logical score/drain/activation gates, then secures registered
bodies and pauses owned timers/ticks at a safe game-thread boundary. Neither Actor hiding,
Actor tick disabling, nor global time dilation is the freeze mechanism. Native world pause
is reserved for the player's explicit PAUSED state, never used to enter a minigame.

**Rationale**: The world must keep advancing for the selected minigame. Queued physics events
can outlive the event that activated the transition. Captured body flags, collision state,
world transforms and velocities permit safe recovery; the normal ball return uses the
cabinet's configured release instead of exact velocity restoration.
**Alternatives considered**: Kinematic rotating flippers may be evaluated only if the physical
prototype cannot meet acceptance; this requires a documented tuning decision, not a second
parallel physics implementation. Hiding the table does not suspend its gameplay.

Epic warns about queued collision notifications in
[physics substepping](https://dev.epicgames.com/documentation/unreal-engine/physics-sub-stepping-in-unreal-engine)
and documents drive limitations in
[Physics Constraint Reference](https://dev.epicgames.com/documentation/unreal-engine/physics-constraint-reference-in-unreal-engine).
[SetSimulatePhysics](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UPrimitiveComponent/SetSimulatePhysics)
can detach a simple body and does not automatically reattach on disable; make the ball collision
its root and restore world-space state explicitly.

## 5. Input, camera and clocks

**Decision**: Keep a common pause/menu context and exactly one mode-specific gameplay context
in the existing Enhanced Input Local Player Subsystem. Controller clears outgoing actuator
commands and held input, swaps owned contexts, possesses the appropriate pawn, explicitly
sets its view target, and requires release before fresh gameplay input. Camera and pawn are
separate references. Turn off implicit possession-driven camera selection in the controller.

Use Boolean actions for discrete arrows/fire/plunger, with explicit held-key suppression for
all controls. Any axis action must additionally pass a neutral/release gate. Intro waits for
fresh confirmation after showing instructions; the 30-second timer begins only when controls
and camera are ready. Pause and results use active, unpaused time. Streaming callbacks may
record readiness while paused but cannot commit gameplay state changes.

**Rationale**: Returning control safely is more than replacing a mapping asset. Shared keys
otherwise leak commands across modes; pawn possession may also reset the view unexpectedly.
**Alternatives considered**: ClearAllMappings would erase common/UI mappings; arbitrary delays
cannot certify camera/level/input readiness.

Epic documents context switching in
[Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine).
[FModifyContextOptions](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/FModifyContextOptions)
provides immediate rebuild and held-key options, with the held-key option applying to Boolean
actions; application-side release gating covers other actions.

## 6. Scoring, minigame rules and validation

**Decision**: Typed score events and immutable results feed the scoring component. Session,
ball, run and transition-generation identities reject stale callbacks. Whole-point int64
score, bounded inputs, one central multiplier application, and a run award ledger prevent
duplicate awards. Exact initial reward profiles and examples are in
[gameplay contracts](contracts/gameplay.md); tune Data Assets, not minigame-to-score calls.

All three games use small planar arenas and original content. Shared lifecycle code is C++;
per-game runtime subclasses keep rules isolated, and Blueprint descendants provide tuning
and presentation. Functional test maps use the same lifecycle as the shipping cabinet.

**Rationale**: Arithmetic and lifecycle failures can be tested early, before polishing assets.
**Alternatives considered**: Per-widget totals and minigames modifying global score obscure
ownership; testing only through manual full sessions misses repeat-callback races.

Use C++ Automation for score/flow invariants, Functional Testing for real-world transitions,
and packaged playtests plus Unreal Insights for physics feel and frame pacing. Epic documents
[Automation](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine),
[Functional Testing](https://dev.epicgames.com/documentation/en-us/unreal-engine/functional-testing-in-unreal-engine),
and [test execution](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine).

## 7. Performance reference and remaining implementation prerequisites

**Decision**: Acceptance reference target is Windows 11 x64, Ryzen 5 5600-class CPU,
RTX 3060 12 GB-class GPU, 16 GB RAM, SSD, 1920×1080, High scalability with hardware ray tracing
and Lumen disabled, conventional lighting, and uncapped VSync-off measurement. This is a chosen
benchmark target, not a claim about the current workstation. Record actual CPU/GPU/RAM,
driver, build, and settings with each result; equivalent replacement hardware needs a recorded
comparison. A separate capped 60 FPS feel pass follows measurement.

**Rationale**: This makes the spec's desktop target reproducible without production rendering
costs. Asset residency is bounded to one table and three small arenas.
**Alternatives considered**: Measuring only in Editor or on unspecified hardware gives no
reproducible acceptance result. Local hardware inventory was unavailable under sandbox access;
no performance or hardware suitability claim has been made.

All architecture questions are resolved. M1 still verifies toolchain installation; M11 needs
access to the chosen reference hardware and five first-time testers. These are explicit
execution prerequisites, not unresolved design choices.
