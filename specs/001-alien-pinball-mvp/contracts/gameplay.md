# Gameplay Contracts

These signatures describe planned project interfaces and behavior; they are not implemented
headers. Types and ownership are defined in [data-model.md](../data-model.md).

## 1. Minigame lifecycle extension contract

`UMiniGameLifecycle` / `IMiniGameLifecycle` is a Blueprint-visible Unreal interface. Concrete
runtimes derive from `AMiniGameRuntimeBase`, whose public entry points enforce identity and
lifecycle guards before calling protected BlueprintNativeEvent content hooks.

| Operation | Caller → receiver | Preconditions / observable result |
| --- | --- | --- |
| Initialize(FMiniGameContext) → status | World subsystem → registered runtime | Dormant, correct definition/root; creates a fresh local run and dormant pawn/actors. Does not start clocks or accept gameplay input |
| GetPresentationTargets() → pawn, camera, HUD data | Controller via subsystem → initialized runtime | Valid objects in the correct streamed instance; camera/pawn can be distinct |
| StartMiniGame() → status | World subsystem on flow command → runtime | Initialize succeeded and flow certified presentation/input readiness; opens local gameplay gate and starts active clock once |
| EndMiniGame(FMiniGameResult) | Runtime rule/base timeout → guarded base | Playing, matching IDs, not previously ended; close local input/hit/spawn gates and stop clocks before publishing one immutable result |
| OnMiniGameEnded(result) delegate | Runtime → subsystem → flow | Notification only; cannot modify pinball total or independently return the controller |
| Cleanup() | Subsystem → runtime | Valid from any stage; idempotent; stops timers/movement/FX, destroys tracked run Actors, unbinds delegates and returns to Dormant |

Internal content hooks may provide setup/start/end/cleanup presentation. Blueprint hooks never
set authoritative flow, score, RunId or lifecycle state. The common base owns the duration
ceiling and terminal latch, so a missed per-game timeout cannot leave an endless run.
Initialization readiness is explicit; no BeginPlay auto-start or fixed Delay as a load test.

`CancelRun(reason)` belongs to the world subsystem. It closes the run before cleanup and
notifies flow to recover with zero bonus for StartFailed/Cancelled/RuntimeFailed. It does not
pretend that cancellation is a successful gameplay completion. Repeated cancellation is safe.
Subsystem startup or teardown clears registration/callbacks for its world; it must not retain
Actor references through GameInstance.

Runtime-local actors may report hits or destroyed targets only to their own run. Shared result
metrics make the score profile data-driven; no minigame links/imports a sibling class. The
framework test stub exercises this same contract without a real game's rules.

## 2. Table and event contract

`ITableSuspendParticipant` is the opt-in boundary for table behaviors that move, tick, own
timers, or create score/drain/objective requests. Each participant is registered with its
`UTableSessionComponent` through explicit ownership/reference at creation.

| Operation | Required behavior |
| --- | --- |
| CaptureState(generation) | Record only owned state before mutation: bodies/constraint settings, timer flags, component/tick flags and gameplay progress |
| SuspendForMinigame(generation) → acknowledged | Cancel forces/held commands, freeze bodies safely, pause owned active timers/ticks, close local event emitters |
| PrepareRestore(snapshot, safeReturn) → acknowledged | Stage valid flags, timer state and safe release; reset actuators neutral; keep bodies/timers/input held and event gates closed |
| CommitRestore(generation) | After every prerequisite is ready, flow performs one pre-physics commit opening score/drain gates and resuming bodies/timers/input; no latent work or failure-prone lookup in commit |
| ResetForNewSession(sessionId) | Clear contacts/progression/owned timers, reset authored state and discard old IDs |

The table component coordinates participants. GameMode/flow independently gates incoming
events, so late physics callbacks cannot bypass a participant's suspension. Never trust an
Actor's visibility or tick flag to mean its physics/timers are suspended.

- `RequestObjectiveActivation(SessionId, BallId, ObjectiveId, EventId)` goes from trigger
  component to flow. It returns Accepted or Rejected immediately at the logical boundary.
- `RequestDrain(SessionId, BallId, EventId)` goes from drain to flow. A drained ball is terminal
  for its BallId; accepting the drain closes further events before decrementing once.
- `SubmitTableScore(FScoringEvent)` goes to the scoring component, which validates flow state,
  phase epoch, session/ball/source/event IDs and configured category before awarding.
- Contact scoring uses a per-source/ball contact episode, ending after actual separation
  beyond a small configured tolerance. A fixed cooldown alone is insufficient to count long
  contact correctly. Substep callback repeats share the same episode/EventId.
- Lane completion requires ordered entry→exit for the current ball and one completion per
  traversal; re-arm after departure. Unintended reverse entry does not award a full traversal.
- Bumper physical impulse may have a short tuning cooldown but score still follows distinct
  contact episodes. Event and force gates both close during suspension.

Game-thread accepted event order is authoritative. The first accepted drain or activation
wins; subsequent conflicting callbacks are rejected. No priority based on object names,
no queued second minigame, and no attempt to infer exact substep chronology from callback order.

## 3. Central scoring contract

`UPinballScoringComponent` is the sole score writer. Its total is read-only to UI and other
systems; its host GameState does not store an independently mutable copy.

| Operation | Rule |
| --- | --- |
| ResetSession(SessionId, profile) | Clears total/ledgers, captures validated settings, starts multiplier one; only GameMode may request |
| SubmitTableScore(event) → award/rejection | Accept only current playable pinball phase, one distinct event; lookup category points in captured profile |
| EvaluateAndAwardMiniGame(result, acceptedRun) → award/rejection | Called only by flow after accepting a valid terminal result; validates identity/schema and run ledger; converts result then commits once |
| GetScoreSnapshot() | Returns total/current multiplier/last award without mutable references |
| OnScoreChanged / OnBonusAwarded | Presentation notifications after atomic ledger+score update; subscribers cannot cause a second award |

An accepted run is finalized even if its award is zero. Repeated calls return the existing
award or an explicit AlreadyFinalized rejection; they never publish a new award animation or
increment total. Results generated by an old session cannot be accepted after restart.
Invalid nonfinite/negative/out-of-range data is rejected as a failed run, not coerced into a
positive reward. Success/failure describes performance; normal loss-of-lives results retain
earned performance. Cancel/start/runtime failure always earns zero.

### Initial configurable arithmetic

Table award = floor(category base points × session multiplier).
Minigame base = clamp(weighted sum of validated metrics, 0, 10,000).
Minigame award = floor(base × captured session multiplier × validated result multiplier).
Apply multiplication once, round down once, and use checked int64 arithmetic. Validate finite
metric inputs and small configured limits before conversion. On impossible overflow, reject
and log the invalid award rather than wrap the score. Default result multiplier is 1; the
MVP profiles only allow 1. Session multiplier is configurable in 1..10, defaults to 1, with
no new gameplay mechanic for earning it. Total additions also use checked int64 arithmetic.

| Profile | Local displayed raw score | Base bonus at multiplier one | Low/high examples |
| --- | --- | --- | --- |
| Asteroid Field | 100 × objects destroyed | min(10,000, 500 × ObjectsDestroyed) | 2 objects →1,000; 20 →10,000 |
| Planetary Defense | 100 × threats destroyed; add 500 per surviving colony at end | min(10,000, 250 × ThreatsDestroyed + 1,000 × StructuresSurviving) | 4 threats/0 colonies →1,000; 28 threats/3 colonies →10,000 |
| Alien Assault | 100 × enemies destroyed; add 10 per completed survival second at end | min(10,000, 250 × EnemiesDestroyed + 100 × floor(SurvivalSeconds)) | 4 enemies/10 seconds →2,000; 28 enemies/30 seconds →10,000 |

Metric weights, local-score values and base cap live in definitions/profiles; no gameplay
class hard-codes pinball bonus conversion. Local raw score is informational; metrics drive
these initial bonus profiles so the score component never double-counts raw score and metrics.
At multiplier two the asteroid examples become 2,000 and 20,000; the 10,000 target is the base
reward at multiplier one, not a cap applied again after multipliers.

Rating uses normalized weighted performance against the base cap: None at zero, Bronze below
one third, Silver from one third to below two thirds, Gold at/above two thirds. Shared pure
profile evaluation supplies the runtime's result rating without granting access to total;
central scoring independently validates/recomputes it. Each game reports ObjectivesCompleted
as its destroyed-object/threat/enemy count and supplies separate survival metrics.

Spawn pacing at M6–M8 must make high examples achievable within 30 seconds: at least 20
asteroids and 28 threats/enemies must be available without requiring optional mechanics.
Fire rates, damage protection, movement and spawn schedules are tuned to make both low and
high examples attainable, then validated during M11 rather than assumed from arithmetic.

## 4. Controller and presentation contract

`APinballPlayerController` is the only writer of possession, view target and owned input
contexts. `IMC_Common` retains Escape/pause and menu actions. One of `IMC_Pinball`,
`IMC_AsteroidField`, `IMC_PlanetaryDefense`, `IMC_AlienAssault` is installed for gameplay;
no gameplay context accepts commands in transition/results or while paused.

| Mode | Actions and bindings | Possession / view |
| --- | --- | --- |
| Pinball | Left, Right Boolean flippers; Down hold/release plunger | Persistent APinballControlPawn / registered table camera |
| Asteroid | Left/Right rotate, Up thrust, Space fire | AAsteroidShipPawn / runtime camera |
| Defense | Mouse absolute aim; Space fire | ADefenseAimPawn / runtime camera |
| Assault | Left/Right move, Space fire | AAssaultShipPawn / runtime camera |
| Instructions/results | Common confirm/menu/Escape only; no gameplay routing | Initialized run or restored table view as transaction requires |

Cancel outgoing logical actions without generating a release-triggered launch/fire. Flush
pressed state, remove only the old owned mode context, and add new mapping with immediate
rebuild/ignore-pressed-key options. Require a fresh release-to-neutral for any axis action
and fresh press for Boolean gameplay actions. For keyboard movement, use separate Boolean
left/right actions to avoid axis-held ambiguity; analog extensions must honor neutral gating.
Do not use ClearAllMappings. Keep Escape able to trigger during native pause.

Fresh Space/Enter confirms instructions after the new view is ready. It is consumed by the
instruction presentation and cannot fire the first gameplay shot. Display controls before
confirmation; gameplay clock starts after confirmation and gameplay input enablement. Defense
cursor is initialized at a clamped valid arena position and movement uses deprojection to the
arena plane, not window-pixel assumptions. UI focus/cursor state is restored for pinball.

`bAutoManageActiveCameraTarget` is disabled on the project controller; possession does not
silently replace explicit view selection. MVP uses a short fade and cut to the registered
camera, avoiding a camera flight through distant arena slots. Snapshot/restore view and pawn
separately; a persistent table camera is the recovery fallback.

Widgets receive read-only FSessionState, score snapshots, run progress and award data via
delegates. They send Start/Restart/Quit/Pause/Confirm intents to the controller/flow. Gameplay,
result timers and score awards never depend on widget construction or animation completion.

## 5. Contract verification before three game implementations

M5 must prove: Initialize never starts play; start twice cannot duplicate timers; duplicate
end emits one result; cleanup after partial initialization is safe; cleanup twice is safe;
world teardown cannot write score; hidden/dormant arenas generate no gameplay; contexts cannot
leak held arrows/Space; stale result and old-session callbacks are rejected. A test-only stub
is a replaceable contract client, not a fourth shipping minigame.
