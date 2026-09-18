# Phase 5 — Generic minigame round trip

Implemented T043–T066 on 2026-09-16 with UE 5.8.2. The independent fixture is
`/Game/Tests/Maps/L_TransitionTest`; the production cabinet remains the Phase 4 three-ball
table until the real minigame associations arrive in later phases.

## Play the fixture

Open `L_TransitionTest`, choose Start, hold/release Down and use Left/Right flippers.
The round objective is the circular insert near the middle/upper table. Enter it with the
active ball. Release held controls, then use a fresh Space or Enter to confirm instructions.
Space lights the arena beacon and increments local score. The round lasts 30 active seconds;
Escape pauses/resumes. Results show the actual central bonus for three active seconds, then
return the same ball entitlement to a collision-checked table marker.

The test definition, pawn, runtime, mapping and cabinet are real saved Unreal assets. Its
one-game development flag does not waive the final cabinet's three-definition validation.
The asset authoring/read-back scripts are `Scripts/Editor/create_phase5_assets.py` and
`Scripts/Editor/validate_phase5_assets.py`.

## Implementation and ownership

- The flow component alone owns public state, acceptance, phase clocks, terminal acceptance,
  recovery notices and ordered return. Pre-physics commits keep setup overlaps gated.
- The world subsystem asynchronously preloads soft references and shown level instances,
  accepts exactly one registered root per instance and owns at most one active run.
  Boot has a 30-second deadline; warm entry and return each have five-second deadlines.
  Human confirmation has no timeout. Failed residency can be explicitly retried.
- The runtime base guards initialization/start/end/cleanup, clamps the active clock, validates
  identity/schema and owns run actors/timers. Spawns use `OverrideLevel`; cancellation stops
  producers and notifies flow once. Paused cancellation continuation waits for resume.
- The table manifest captures bodies, angular velocities in radians, actor/component ticks,
  participant hooks and explicit timer handles. Only previously active timers resume.
  Flipper drives and plunger charge are neutralized; lanes cannot score a return teleport.
- Controller snapshots retain weak pawn/view/context/focus references and generation.
  Enhanced Input keeps common Escape, suppresses held Boolean actions, consumes confirmation,
  and exposes a neutral-input gate for future pawn axis handlers.
- The scoring owner snapshots profile data, evaluates bounded metrics and normalized rating,
  applies multipliers once, floors once, checks int64 overflow and finalizes zero awards too.
  Bonus notification follows the atomic ledger write. Results UI cannot award again.
- A missing ball actor is recreated only at a validated release with the same SessionId/BallId.
  Both blocked markers or an unacknowledged restore keep gameplay secured with Retry/Restart/Quit.
  Later cleanup failure preserves an already finalized bonus. Teardown does not restore control.

Every added/modified function has an explanatory declaration or implementation comment.
One existing remediation fixture also needed `.Get()` when extracting a `TObjectPtr` into
an `auto*`; the full rebuild exposed that compile error.

## Executed validation

Final full-suite report: **10 tests completed, 0 failures, 0 skipped** (9 clean successes and
1 success with the engine render-thread warning), exported at `2026.09.16-23.14.47` UTC.
The final PIE fixture emitted `PINBALL_TRANSITION_PASS cycles=20`; the process exited 0.
`git diff --check` passed and the new binary packages resolve to the repository's LFS filter.

Reference machine: Ryzen 7 7800X3D, Radeon RX 7900 XTX, 64 GB RAM, Windows 11 23H2.
Standalone checks rendered at 1280×720 with a 60 FPS cap. PIE Automation also used a real
rendering viewport. NullRHI was used only for content authoring/read-back and initial unit tests.

| Check | Evidence |
| --- | --- |
| Development Editor build | `Saved/Logs/Phase5Build.log`, `Result: Succeeded` |
| Saved asset wiring | `Saved/Logs/Phase5AssetValidation.log`, `PINBALL_PHASE5_ASSET_VALIDATION_PASSED`; zero commandlet errors/warnings |
| Initial ten round trips | `Saved/Logs/Phase5RoundTrip.log`, explicit pass marker |
| Nineteen physical/input/failure scenarios | `Saved/Logs/Phase5InputAndTimers.log`, `PINBALL_TRANSITION_PASS cycles=19` |
| Full Automation suite and twenty-scenario PIE wrapper | `Saved/Automation/Phase5Final/index.json` and `Saved/Logs/Phase5FinalAutomation.log` |
| Visual inspection | `Saved/Screenshots/WindowsEditor/Phase5Instructions.png` and `Phase5Results.png`: readable controls and actual 1,000-point bonus |

The functional fixture uses actual physical objective occupancy, real table bodies, streaming,
possession, input contexts and the shipping flow. It injects controlled terminal results in
short cycles and also runs one full 30-active-second timeout. It verifies:

- Ten normal round trips, stable session/ball/ball count, once-only 1,000-point results and
  duplicate terminal rejection. No run pawn or active subsystem record remains after return.
- Exact table-body transforms and disabled simulation during play/results; active and already
  paused table timers retain their remaining time and correct post-return state.
- Pause in Securing, Preparing, confirmation, play, results and Returning, plus pause just
  before the 30-second timeout. Phase and gameplay clocks do not advance during pause.
- Held Left/Right/Down/Space across entry, consumed confirmation, one action from a fresh
  Enhanced Input Space press, and neutral flippers/plunger after held-key return.
- Objective protection lasting one active second and physical exit/reentry; remaining inside
  after the protection expires does not retrigger.
- Failed start and partial initialization, missing root/camera/pawn, zero-award recovery,
  blocked preferred release, both releases blocked, stalled restore acknowledgment, explicit
  Retry and preserved earned bonus, and destroyed-ball same-entitlement replacement.
- Explicit Restart from secured recovery resets score/balls and invalidates old terminal
  notifications. Trigger-first and drain-first acceptance reject the competing decision.

Automation additionally covers all three planned metric reward profiles, low/high examples,
monotonicity, malformed/nonfinite metrics, bounds, multipliers, stale IDs, overflow, partial
initialization, repeated cleanup, cancellation and the existing Phase 2–4 deterministic tests.

Two fixture defects were corrected during validation: calling `StartTest` instead of `RunTest`
did not start Unreal's functional-test lifecycle; separately, calling `FStartPIECommand` after
`AutomationOpenMap` created a duplicate PIE session and an abort error. The final wrapper
uses the map loader's existing PIE startup and reports the real fixture result.

The engine may emit its existing `r.MotionVectorSimulation` render-thread warning. Standalone
startup also logs an unrelated experimental ToolsetRegistry PythonTestRunner error before
gameplay; saved-asset commandlets complete with zero errors. No packaged performance or
250 ms transition-freeze claim is made here; cooked integration and performance measurement
remain Phase 9/11 acceptance work. Placement and injected terminal scenarios are controlled
integration checks, not a claim of ten unaided human playthroughs or final table tuning.

## Repeat

After building, run `./Scripts/Run-Phase5Validation.ps1` for rendered standalone acceptance.
`-RecoveryOnly` selects the final secured Restart/stale-callback scenario for focused iteration.
The script requires an explicit pass marker and clean process exit, with a five-minute bound.
In the Editor Automation window run `PinballBattle` for the complete suite, or
`PinballBattle.Transition.RoundTrips` for the latent PIE fixture.

Constitution review: shared pinball/flow/scoring import no concrete minigame implementation;
the stub lives in test content, owns local progress only and uses the same lifecycle as future
games. World pause is exclusively the user's pause overlay. No paid assets, global score
pointer in context, separate minigame score owner, or constitution exemption was introduced.
