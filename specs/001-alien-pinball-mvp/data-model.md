# Data Model: Alien Invasion Pinball MVP

These are planned reflected C++ types and Data Assets, not implementation code. All runtime
objects belong to one world/session. Definition assets are immutable while a run is active.
Use explicit references and typed IDs, never Actor or level object names for identity.

## Ownership and identity

| Record | Owner / lifetime | Fields and relationships |
| --- | --- | --- |
| FSessionState | APinballGameStateBase, until restart | SessionId (FGuid), BallsRemaining (0..3), CurrentBallId, FlowState, ResumeState when paused, Multiplier (positive bounded integer, default 1), ObjectiveStates, LatestAward. Only GameMode/flow mutate session fields |
| Score total and ledger | UPinballScoringComponent on GameState, same session | TotalScore (int64 >=0), accepted event IDs, finalized/awarded RunIds, immutable scoring settings revision. UI snapshot combines this with FSessionState; no second writable total |
| FBallHandle | GameMode/table | SessionId, BallId, weak ball reference, disposition (Ready/Active/Suspended/Drained/Recovering). New spawned entitlement gets a new ID; recover/return preserves ID |
| FObjectiveState | Session state, keyed by ObjectiveId | Associated MiniGameId, available/re-arm phase, completed-at-least-once, last RunId. Definition association is separate from live status |
| FTransitionRecord | UGameFlowComponent with subsystem handles | SessionId, BallId, RunId, Generation, ObjectiveId, direction/phase, accepted terminal latch, saved table/controller state, pending readiness, deadlines and award. Never a second global game state |
| FActiveMiniGameRun | UMinigameWorldSubsystem | RunId, SessionId, MiniGameId, generation, definition, streaming handle, runtime/pawn references, lifecycle stage, context, result; at most one active record |

New session invalidates every prior run and generation before UI, timers or Actors are reset.
All deferred completions compare IDs to current owners. Weak object references are validated
again when callbacks run. Data identifiers may be FName/PrimaryAssetId values; they are not
names of level Actors and cannot be used to locate Actors by string search.

## Enumerations and state model

**EArcadeGameFlowState**: BOOT, ATTRACT, PINBALL_READY, PINBALL_PLAYING,
MINIGAME_TRANSITION, MINIGAME_PLAYING, MINIGAME_RESULTS, BALL_LOST, GAME_OVER, PAUSED.

| From | Event / guard | To |
| --- | --- | --- |
| BOOT | Cabinet/table/assets/roots validated | ATTRACT |
| ATTRACT | Start | PINBALL_READY |
| PINBALL_READY | Accepted plunger release | PINBALL_PLAYING |
| PINBALL_PLAYING | Accepted drain first | BALL_LOST |
| BALL_LOST | Decrement once; balls >0; replacement ready | PINBALL_READY |
| BALL_LOST | Balls =0 | GAME_OVER |
| PINBALL_PLAYING | Accepted objective first; event gates close | MINIGAME_TRANSITION |
| MINIGAME_TRANSITION | Secured + arena ready + controller ready + fresh confirmation | MINIGAME_PLAYING |
| MINIGAME_PLAYING | Accepted normal terminal result | MINIGAME_RESULTS |
| MINIGAME_RESULTS | Three active seconds elapsed | MINIGAME_TRANSITION (return phase) |
| MINIGAME_TRANSITION | Cleanup/restore/release complete | PINBALL_PLAYING |
| MINIGAME_TRANSITION or MINIGAME_PLAYING | Start/run failure | MINIGAME_TRANSITION (failure notice then return) |
| Ready/Playing/Transition/Results | Escape | PAUSED; remember exact previous state and transaction phase |
| PAUSED | Resume | Saved state, unless teardown invalidated the session |
| GAME_OVER | Restart | PINBALL_READY with new SessionId |
| Any | World teardown / Quit | Invalidate transaction, cleanup, no restoration into destroyed world |

BALL_LOST processing is an atomic rule update before the next frame; pause cannot interrupt
between accepting the drain and its one decrement. UI menu interactions at ATTRACT/GAME_OVER
do not run gameplay. Pause is an overlay with one saved underlying state, never a stack of
PAUSED states. Transaction phases include Securing, Preparing, AwaitingConfirmation, Returning,
Recovering; only flow writes the public state, subsystem reports readiness/completion.

**EMiniGameLifecycleState**: Dormant → Initialized → Playing → Ended → Dormant.
Cleanup is valid from every stage. Pause is controlled by the flow and preserves the lifecycle
stage. **EMiniGameEndReason**: TimedOut, LivesExhausted, StartFailed, Cancelled, RuntimeFailed.
Success is a separate boolean: shooters surviving timeout succeed; defense succeeds at timeout
with at least one colony. LivesExhausted can have nonzero earned points; failure/cancel cannot.

## Immutable definition assets

### UCabinetDefinition

- CabinetId, display title, table class/reference, persistent map reference, scoring profile.
- Pinball tuning, ball/control pawn classes, HUD classes, pinball input context.
- Objective definitions: unique ObjectiveId, associated UMiniGameDefinition, label/icon/cue,
  safe-return marker reference resolved by table registration, release velocity.
- Arena slot transforms and bounds assigned to minigame definitions; startup preload list.
- Initial balls =3; return trigger protection =1 second; results presentation =3 seconds.

Validation: exactly three required associations for this cabinet, all IDs unique, references
cookable, no overlapping arena bounds, no safe release inside solids/trigger/drain. Marker
references are assigned or registered explicitly. No dependency from shared classes back to
this AlienInvasion asset.

### UMiniGameDefinition

- MiniGameId, display name, soft UWorld reference, expected runtime subclass.
- Mode input mapping context, instructions, presentation/widget class references.
- Duration =30 active seconds, optional local lives =3, run pawn class/config and arena bounds.
- Scoring profile key, allowed result-multiplier range (default only 1), configured metric
  keys and bounds. Per-game difficulty settings reside on its Blueprint defaults or a small
  game-specific configuration asset when editing benefits from one.

Validation: one registered runtime root of the expected type per streamed instance; valid
camera, spawn location and input context; positive finite dimensions/duration; no references
to a sibling minigame's classes/content. All runtime/sublevel Actors start dormant.

### UScoringProfile

- ProfileId/revision, point categories and base values (Target=100, Bumper=50, Lane=500).
- Per-minigame metric weights, base reward cap, rating thresholds; all nonnegative.
- Allowed session multiplier range 1..10; result multiplier range set by definition (MVP 1).
- Rounding policy: floor once after multiplication; int64 checked arithmetic with overflow rejection.

Validation: duplicate categories rejected, missing requested profile rejected, finite bounded
values only, no negative weights. Snapshot revision/settings at session/run acceptance so
live editor tuning cannot change the interpretation of an in-flight result.

### UPinballTuningData

- Table units/scale/incline, ball radius/mass, physical material settings, speed bounds.
- Plunger max hold/impulse curve, flipper angles/drive strength/damping and return behavior.
- Bumper impulse/cooldown, contact-separation re-arm tolerance, lane entry/exit conditions.
- Escape bounds, low-motion threshold and 10-second trap window; exempt launch/capture areas.
- Physics stepping/CCD expectations documented with config; do not hide project-wide physics
  settings in cabinet scripts. Tuning values are starting hypotheses, verified at M2.

## Boundary records

### FMiniGameContext

SessionId, RunId, MiniGameId, Generation, definition/config and read-only reward-profile snapshot,
arena transform/bounds,
run seed, duration limit and local starting lives/objectives. No mutable session or score
service pointer. Runtime obtains its own camera/pawn through its registered root. Shared
score is deliberately absent; minigames do not need it to calculate performance.

### FMiniGameResult

SessionId, RunId, MiniGameId, Generation, EndReason, bSuccess, RawScore (int64),
PerformanceRating (enum), ObjectivesCompleted (int32), DurationSeconds (finite double),
bHasMultiplier + Multiplier, and typed metric entries (MetricId → finite nonnegative value).
Standard metric IDs: ObjectsDestroyed, ThreatsDestroyed, StructuresSurviving,
EnemiesDestroyed, LivesRemaining, SurvivalSeconds. Profiles consume only configured keys.
The shared framework contains no switch on concrete minigame names.

Validation: identity exactly matches active accepted run; one terminal result; time in
0..duration limit (allow only numerical epsilon, clamp internal clock at limit); counts and
metrics within definition bounds; raw score >=0; multiplier allowed by definition. Invalid
results are logged and converted to zero-award failure recovery, not partially accepted.
Neither the result nor runtime includes a pinball bonus chosen by the minigame.

### FScoringEvent and FScoreAward

Event: SessionId, BallId, EventId, SourceId, Category, contact/traversal sequence, observed
phase/epoch. Event carries a category, not an arbitrary UI delta. Table component emits at
most once per qualifying contact; score service independently rejects repeats and wrong phase.

Award: AwardId, SessionId, optional RunId or EventId, BasePoints, EffectiveMultiplier,
AwardedPoints, NewTotal, profile revision. Score service publishes a read-only award and latest
result presentation together. Results HUD shows awarded bonus; it cannot award on animation end.
Ledger retains accepted event IDs for the current three-ball session; bounded by actual events,
cleared on restart. No persistent backend or anti-cheat protocol is needed.

### FTableSuspendSnapshot and FBodySnapshot

Table snapshot: SessionId/BallId/transition generation, registered participant references,
body snapshots, table tick/component activation flags, paused timer handles with previous
active/paused state, lane/contact progression and logical actuator state, saved timestamp.

Body: weak primitive reference, world transform, linear velocity, angular velocity in radians,
simulation enabled, collision enabled/profile/responses, gravity, awake/asleep flag, and
attachment metadata if applicable. Constrained flippers additionally capture drive/constraint
settings through their participant. Capture before changing flags; restore only components
still belonging to this table/generation. No arbitrary reflection-based Actor serialization.

Normal minigame return overrides ball transform/velocity with safe release and resets flippers
and plunger to neutral, discarding stale force/input accumulation. Other table state/timers
resume from captured values only at the atomic pre-physics restoration commit after all
controller/table acknowledgments. Exact freeze/resume for the user's pause uses native pause
plus clock/presentation guards, not minigame safe-release behavior.

### FControllerModeSnapshot

Persistent pinball pawn, prior view target, owned mapping context references/priorities,
cursor visibility, click/hover flags, input mode and viewport focus, logical held-action state.
Only restore contexts owned by this controller's game; preserve other subsystem mappings.
Do not replay saved held actions. Reacquire registered default table camera/pawn if a saved
reference is invalid; never restore a view target that is about to be destroyed.

## Registration and lifetime invariants

- Table components register/unregister with their table's UTableSessionComponent. New actors
  cannot become active during a suspended phase. Registered movable bodies and timers are
  an explicit manifest checked by development validation, not a global GetAllActors search.
- A streamed root registers with UMinigameWorldSubsystem using its level-instance identity.
  Multiple PIE worlds get separate registries. Level loaded alone is not readiness.
- Runtime-spawned actors are tracked in a per-run registry, have appropriate Owner references,
  and are spawned into the arena level with FActorSpawnParameters.OverrideLevel where needed.
  Ownership alone is not relied on to cause destruction at cleanup/unload.
- Cleanup explicitly destroys run Actors, clears timer handles and delegates, stops audio/FX,
  resets input state, and returns authored environment to dormancy. Resident map roots survive
  ordinary cleanup and may initialize a new RunId later.
- GameState/score/flow are authoritative local-world objects. No replication, networking,
  persistent global scoreboard, or custom GameInstance state is introduced.
