# Pinball / Minigame Transition Protocol

This is the normative MVP transition design. The table stays alive in the persistent world.
The flow component owns public state; the minigame world subsystem executes environment/run
operations; the controller executes input/view operations; the score component alone awards.

## Invariants

1. At most one active run/transition exists at a time; a session may contain many finalized runs.
2. A table score/drain/objective callback is accepted only for the current SessionId/BallId,
   event epoch, and enabled pinball gate. Physical freeze alone is not an event gate.
3. A minigame cannot start until the table has acknowledged suspension and its environment,
   runtime, pawn, camera, controls and instruction confirmation are ready.
4. Pinball remains suspended through transition, minigame play, results, and restoration.
5. Neither map travel, global time dilation, nor global world pause is used for entry.
6. Score award and run finalization occur once, before a three-active-second results display.
7. Return does not consume a ball, reset the session, or require a new plunger launch.
8. Native pause freezes the current mode and transaction; it does not perform minigame return.
9. Any callback arriving after restart/cancel/teardown must fail generation/identity validation.
10. Visibility, collision, tick, timers, audio and actor lifetime are separate responsibilities.

## A. Boot residency and readiness

1. GameMode registers exactly one persistent table/control pawn/camera and validates the
   cabinet Data Asset, objective associations and safe release markers.
2. UMinigameWorldSubsystem asynchronously loads all three definition assets and their level
   instances using soft world references and cabinet slot transforms. Keep hard runtime
   handles while resident; include these assets/maps in cook rules explicitly.
3. Each arena is added/shown once during BOOT. Its authored root begins dormant and registers
   with its actual streamed level instance. All authored dynamic objects, timers, collision,
   ticking, effects and audio are inactive by default; OnBeginPlay does not begin a run.
4. Readiness requires all of: streaming instance loaded AND shown; exactly one matching
   runtime root registered; valid configured camera and pawn spawn definition; definition
   assets loaded; environment/collision/dormancy validation passed. A loaded callback alone
   cannot make the run ready. Stream handles and run generation disambiguate multiple worlds.
5. The root explicitly hides dormant environment primitives and disables arena-local lights,
   post-process, audio and collision. Keep levels shown/added and resident; do not oscillate
   level visibility each run or assume hidden levels retain usable registered Actors.
6. Prewarm tiny effect/projectile classes and required UI/material resources before Start is
   enabled; destroy warmup run actors. Measure first-use hitches in the cooked build. If shader
   warmup is needed, use supported engine facilities in M11, not blocking loads during play.
7. Show a responsive loading/attract presentation while boot prepares. A definition/root
   failure or 30 seconds of unpaused loading without readiness shows a retry/quit error;
   it cannot falsely enable a complete shipping cabinet. Runtime warm-activation failure uses
   the recovery path below and does not cost a ball.

The three arenas occupy non-overlapping bounded slots away from the playfield, with explicit
collision channels and local lighting/audio. They share one physics/world clock, but only the
selected runtime can tick or spawn. No World Partition, streaming volumes or viewpoint-based
unload decisions are used. Residency is selected by cabinet data, not concrete game names.

## B. Accept and secure pinball

1. Trigger sends SessionId, BallId, ObjectiveId and EventId on the game thread. Flow accepts
   only PINBALL_PLAYING with a live ball, available objective, expired one-second return guard,
   and an observed exit/re-entry since its previous activation.
2. In one logical operation, assign RunId and increment Generation; capture the accepted
   score/multiplier/profile revision and objective; set the transition latch and close table
   score, drain and trigger gates. Increment the table event epoch. A simultaneously delivered
   second trigger or drain is rejected. If a drain was accepted first, this activation loses.
   Start a five-second unpaused watchdog now, covering suspension acknowledgments and warm
   setup through confirmation-readiness. No asynchronous prerequisite is outside this deadline.
3. Publish MINIGAME_TRANSITION with internal phase Securing. Semantically the pinball session
   is already protected at the gate closure; the physical-body barrier must complete before
   arena gameplay is possible. Disable gameplay routing; call explicit CancelActions so
   releasing a charged plunger does not fire an impulse.
4. Capture the table/body/controller snapshots before physical mutation. Do not run Actor,
   constraint or component mutation inside an async physics callback. Bridge physics events
   to the game thread, then secure at the next supported game-thread/pre-physics boundary.
   Already queued callbacks are harmless because their event epoch is obsolete.
5. UTableSessionComponent calls registered participants: preserve timer active/paused flags;
   pause only their owned active handles; disable table gameplay ticks including component
   ticks/timelines; capture ball/flipper bodies, constraint settings and local progression.
   Stop drives/forces, zero active velocities as appropriate, disable simulation and dynamic
   collision while preserving the snapshot. Sleeping a body alone is insufficient protection.
6. Ball collision is the root component; preserve world-space transforms rather than relying
   on attachment side effects of SetSimulatePhysics. Constrained flippers explicitly acknowledge
   frozen drive/body state. Static walls may remain because the arena is spatially isolated.
7. All required participants must acknowledge suspension. If a required participant is absent
   or fails, do not start the minigame. Run recovery while table gates remain closed; the
   entry watchdog detects a participant that never acknowledges. Do not block the game thread.

Paused or already-disabled timers/components remain so after restoration. An unregistered
physics/timer producer is a development validation failure; it cannot silently keep playing.

## C. Activate environment and transfer control

1. Resolve the already resident definition/streaming handle. If no valid root remains, report
   failure rather than synchronously load during gameplay; repair residency asynchronously
   after safe return. Pinball cannot begin a run against stale references.
2. Initialize the dormant runtime with a fresh FMiniGameContext. Runtime constructs its local
   pawn/enemies/objects in a nonplaying state, registers each spawned actor, and returns
   explicit readiness. Use OverrideLevel for arena membership where applicable plus an
   explicit per-run cleanup registry; AActor.Owner is not the entire lifetime policy.
3. Activate only this environment's rendering/local lighting/audio. Keep runtime motion,
   damage, scoring, spawn timers and active clock closed until StartMiniGame. No sibling arena
   is awakened and no global world settings are changed.
4. Controller snapshots its persistent pinball pawn, camera, owned contexts, cursor and focus.
   Remove the outgoing gameplay context; common Escape/menu remains. Possess the run pawn,
   choose the registered camera explicitly, and complete a short active-time fade/cut.
   Use no blend through the spatial gap between arenas.
5. Display instructions including controls and fresh Space/Enter confirmation. Camera and
   input readiness must be acknowledged before instructions can confirm. Release gating
   prevents keys held at activation from confirming or controlling the new mode. The five-
   second technical readiness watchdog ends here; waiting for a human confirmation has no
   timeout and does not consume the 30-second gameplay duration.
6. On fresh confirmation, consume that event. Install the owned minigame mapping context with
   immediate rebuild and ignore-held-Boolean options; apply explicit neutral gating for axes.
   Verify pawn/view/context validity, then atomically enter MINIGAME_PLAYING, open local input,
   and command StartMiniGame exactly once. Set elapsed active gameplay time to zero here.
7. If Initialize/Start fails or a required target disappears at any step, close all run gates
   and invoke failure recovery. A disabled input context or missing camera must never leave
   the player controlling the frozen table or an invisible arena.

Readiness is an AND barrier, not a fixed Delay. A per-run transition record records which
steps completed, so rollback can handle partial initialization and partial controller swaps.

## D. End, award and present results

1. Runtime's base owns the active elapsed clock and 30-second limit; per-game lives rules may
   end early. Clamp the final duration to the limit. On the game thread, the first accepted
   terminal condition latches Ended; queued later damage/timeout notifications are ignored.
   Only hits admitted before that terminal boundary contribute to the immutable result.
2. Before OnMiniGameEnded is emitted, stop local input, movement, collision damage, spawning
   and gameplay timers. Keep only static result presentation visible. Flow checks IDs,
   generation, expected lifecycle and schema before accepting the result.
3. Enter MINIGAME_RESULTS; remove mode gameplay input and preserve common pause. Flow calls
   EvaluateAndAwardMiniGame on the scoring component. Score validation plus run-ledger commit
   is atomic on the game thread; a zero award is still finalized. A duplicate notification
   cannot replay either the award or result presentation.
4. Results widget displays the accepted run summary and the actual returned award. Hold for
   three seconds of unpaused time, owned by flow (not the widget). Pause freezes this clock;
   removing/recreating the widget cannot re-award. Failed performance after losing lives still
   shows earned bonus. Invalid/aborted/start-failed runs take zero-award recovery instead.
5. After three active seconds, enter MINIGAME_TRANSITION with Returning phase. Keep table
   events closed and invoke the ordered restore below. No scoring event is accepted merely
   because a HUD switched to pinball.

## E. Deactivate and safely restore pinball

1. Start a five-second unpaused return watchdog covering cleanup and every restoration
   acknowledgment. Disable outgoing gameplay input and cancel held commands; stop remaining run-owned
   timers, callbacks, audio, FX, projectiles and movement. Detach presentation listeners.
2. Possess the saved persistent pinball control pawn and set the table view target while the
   outgoing run pawn/camera still exist. Apply a short fade/cut. If snapshots are invalid,
   resolve the explicitly registered table defaults; never guess by Actor names.
3. Call Cleanup once (safe if called again). Explicitly destroy all run Actors, clear delegate
   and timer handles, reset root local data, and hide/deactivate arena environment components.
   Keep the authored level/root resident and dormant for the next fresh run. Do not unload or
   destroy the camera before the controller has released it.
4. Prepare restored participant state while bodies, timers and all event gates remain held.
   Put flippers and plunger in neutral; stage their collision/constraint settings. Discard
   pre-transition held forces. Record which timers will resume and their remaining time;
   do not resume them yet. Failure or expiry of the return watchdog keeps the table secured
   and shows Retry/Restart/Quit; it must not recursively retry the same failing restore forever.
5. Validate the cabinet-configured safe ball release by collision sweep/overlap. Reposition
   the original BallId into clear space and stage gravity/collision/simulation flags and the
   configured bounded release velocity without enabling simulation. No extra plunger launch.
   Use a validated backup marker
   if the preferred marker is obstructed. Recompute contact occupancy before rearming scores.
6. Restore pinball context, cursor/focus, HUD and camera; rebuild input with fresh-press guards.
   After all body/participant/controller acknowledgments, perform one game-thread pre-physics
   commit without a latent step: apply prepared collision/constraint flags, publish
   PINBALL_PLAYING and the new event epoch, open score/drain gates, enable bodies with prepared
   velocities, resume previously active timers/ticks, and enable fresh gameplay input. Until
   this commit none of those producers may progress. Re-entrant overlap notifications caused
   by setup are rejected as setup events; only subsequent physical events use the new epoch.
7. Keep special-objective gates closed for one active second; also require exiting/re-entering
   the relevant trigger. A timer expiring while the ball remains inside does not trigger a run.
   Pause freezes protection. Ordinary scoring is available once pinball is genuinely resumed.
8. Release transition-only snapshots and references; retain finalized RunId in the score ledger
   until session reset. The objective completion indicator is updated on normal completion,
   including lives-exhausted runs, not failed start or cancellation.

If original ball is destroyed unexpectedly, recreate exactly one physical Actor for the same
ball entitlement/ID using the validated safe release; do not decrement balls. If neither
release marker is safe or table/controller cannot be reconstructed, keep pinball secured and
show a recoverable error with Retry/Restart/Quit. Do not silently resume into a drain or a
corrupt session. Restart is explicit user recovery, not an automatic loss of preserved progress.

## F. Failure, cancellation and teardown

All failure branches converge on one idempotent recovery operation keyed by SessionId/RunId/
Generation. Invalidate outstanding run callbacks first, close run gates, finalize zero award
if not already finalized, and show a two-active-second failure notice. Then use E to restore.
A failed cleanup after a legitimately awarded result must not zero or re-award that result;
retain the ledger and continue cleanup/retry.

Use a new generation for recovery so late original initialization callbacks cannot re-enable
the arena. If a late streaming/root callback arrives, keep it dormant and unregister/destroy
any stale run resources. Runtime-spawned Actors belong to a recorded run and level; timers
capture weak references plus identities. Clear all delegate bindings on world teardown.

- Restart (normally from GAME_OVER, also available in fatal recovery UI) cancels/invalidate
  first, clears runtime actors and award ledgers, then creates new SessionId and one ball.
- Quit/EndPlay cancels without trying to restore a controller into a destroyed world.
- A single failed activation never decrements balls or changes the shared score.
- A retry starts from current validated ownership; it does not resurrect a partial old run.

## G. Pause semantics

Escape uses the common action with pause execution enabled. Save public flow and exact
transaction phase, enter PAUSED, and use native world pause for gameplay/physics. Also gate
score/input/transition commits and explicitly freeze fade, result, watchdog and UI timers
that might use real time or tick while paused. No gameplay Actor is configured to tick while
paused. Menu UI and Escape remain responsive; camera cuts/fades do not advance.

Streaming/asset work may finish while paused. Callbacks can update readiness flags only;
continuations wait for resume and must revalidate generation/references. On resume, restore
underlying state once, release native pause, and allow one queued valid continuation. Do not
call Initialize/Start/award twice. Do not clear or restart the run clock. Pinball body transforms
stay exact across user pause; normal minigame return's safe-release policy is not invoked.

## H. Required transition evidence

| Test | Required observable result |
| --- | --- |
| Moving ball with active bumper/flipper/timers enters each game | Table transforms and remaining timer times unchanged during minigame; no table score/drain |
| Trigger-trigger and trigger-drain in both acceptance orders | One terminal decision; no second run and no double ball decrement |
| Duplicate substep contacts and duplicate result | One event award; one result award/presentation |
| Hold Left/Right/Down/Space across entry and return | No latent rotation, thrust, fire, flipper or plunger action until fresh input |
| Missing level/root/camera/pawn or failed initialize/start | Responsive notice, zero bonus, safe same-entitlement return |
| Escape in Securing/Preparing/Confirmation/Playing/Results/Returning | No active time or body progress; one correct continuation after resume |
| Pause exactly before timeout/last-life hit | Deterministic accepted terminal boundary, one result |
| Cancel/restart while callbacks are pending | Old generations cannot change new score/ball/control |
| Return while trigger still overlapped | No repeat until guard expires AND leave/re-entry occurs |
| Preferred release obstructed | Valid backup used; both blocked shows secured recovery UI |
| Ten cycles per game plus repeated game restart | No leaked active run Actors, timers, mappings, delegates or audio |
| Cooked cold start and first real activation | All soft-referenced levels/assets included; no >250 ms transition freeze |

Log state changes with session/ball/run IDs, generation, phase, accepted/rejected reason and
readiness timings using a project log category. Development widgets may display these fields;
shipping player UI shows only controls, status and clear recovery notices. Instrument first,
then verify packaged behavior; Editor success alone does not prove streaming/cook correctness.
