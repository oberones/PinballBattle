# Phase 7 — Planetary Defense

Implemented T075–T082 on 2026-09-18. The six independent native gameplay classes use the
existing lifecycle, streamed arena, controller, central scoring and same-ball return.
Real Blueprint, map, input, HUD, material and sound packages were authored and saved through
Unreal's Python commandlet. The requirements checklist remained read-only: 16 checked,
zero unchecked. No `.specify/extensions.yml` or implementation hooks were present.

## Play

- Default cabinet: `/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet`. Start, hold/release
  Down to launch, then enter the green Defense objective on the left of the table.
- Independent harness: `/Game/Tests/Maps/L_DefenseTest`, configured with
  `DA_DefenseTestCabinet` and only `DA_MG_PlanetaryDefense`.
- Release held keys and confirm with Space/Enter. Move the mouse to aim, hold Space to
  launch, and use Escape to pause/resume. The confirmation press never launches a shot.
- Three colonies each withstand one impact. Losing the final colony does **not** end the
  run or disable the launcher. Both outcomes finish after 30 active seconds.
- Each interception earns 100 local points; each survivor adds 500 local points at timeout.
  The shared score profile awards `min(10000, 250 × ThreatsDestroyed + 1000 × StructuresSurviving)`.
  Four threats and zero colonies produce 1,000; 28 threats and three colonies produce 10,000.
- Results display through the existing three-second flow before the same pinball resumes.

## Gameplay and control implementation

`PlanetaryDefenseRuntime` owns three colony spawn anchors, creates the run's three colonies,
paces three initial threats plus one every 0.55 active seconds, and applies a 0.22-second
fire interval without an ammunition limit. The 30-second ceiling belongs to the common
runtime. Success requires at least one surviving colony; the final result carries both
performance metrics. No gameplay class converts metrics to the global bonus or changes
pinball ball accounting.

`DefenseAimPawn` intersects the mouse's deprojected ray with the transformed arena plane,
clamps it inside the useful target band, and retains its last valid target on projection
failure. Initial pointer placement projects a valid world target after camera readiness.
The reticle and launcher barrel show the actual clamped destination. Enhanced Input polls
held fire without a latched Boolean; the controller's release barrier remains authoritative
on entry and resume. The pawn's runtime link survives possession changing its Actor Owner.

Interceptors sweep to their captured destination and detonate on an accepted threat hit
or exact arrival. Threats also sweep through blasts and colonies, processing contacts in
travel order. Runtime membership, run IDs and a resolved latch reject stale hits and prevent
impact/destruction or overlapping blasts from crediting the same threat twice. Blast disks
and collision radii agree, shrink during the last quarter of their 1.4-second lifetime,
and expire using active time. All run actors and interception audio are explicitly cleaned.

The shared definition gained a mouse-aim presentation option; the shared controller uses
that option to retain the cursor and viewport focus during gameplay/resume. Each definition's
HUD is now selected independently, with the cabinet HUD as fallback. Defense uses a narrow
status column beside its arena; existing keyboard-only games keep their previous input mode.
There are no imports of, or content dependencies on, Asteroid Field or Alien Assault.

## Executed validation

Environment: UE 5.8.2, Development Editor Win64, MSVC 14.50.35738, Windows SDK 10.0.22621.0;
Windows 11 23H2, Ryzen 7 7800X3D, Radeon RX 7900 XTX, 64 GB RAM. Rendered runs use D3D12,
1280 × 720, VSync off, project quality defaults, with separate 30 and 60 FPS caps.

- Development Editor build passed, including UHT, all six gameplay classes and the test probe.
- Unreal Automation: **7 passed, 0 failed, 0 warnings, 0 not run**. Filters cover Defense,
  Asteroid, lifecycle, score, flow/pause and restart score locking. Defense assertions include
  transformed ray intersection, invalid/behind/parallel rays, aim clamping, pre-start and
  terminal fire rejection, rate limiting, real overlapping blast queries, stale/duplicate
  hits, one-hit colony loss, continued fire at zero colonies, finite blasts, target supply,
  both 30-second endings, clean replay, local survivor points and monotonic bonus arithmetic.
  Report: `Saved/Automation/Phase7/index.json`; log: `Saved/Logs/Phase7Tests.log`.
- Fresh-process asset validation passed for all packages, explicit camera/anchors, input,
  sound, HUD, metrics, score weights, isolated test cabinet, both production objectives and
  absence of sibling minigame dependencies. Marker: `PINBALL_PHASE7_ASSET_VALIDATION_PASSED`
  in `Saved/Logs/Phase7Assets.log`. The existing Asteroid asset validator also passed with
  the production cabinet now containing both definitions.
- Rendered 30/60 FPS round trips passed actual mouse projection/deprojection, Space through
  Enhanced Input, entry/confirmation held-key suppression, native pause, fresh fire on resume,
  real interceptor/blast kills, colony impacts, zero-colony continued interception, scoring,
  actor cleanup, cursor restoration and unchanged SessionId/BallId/three pinball balls.
- The final 60 FPS run also pressed/released the actual Escape binding for pause/resume
  in each round and observed exactly one emitted result per run: 30 seconds and success
  with survivors, 30 seconds and failure with zero survivors. The observer captures the
  accepted pause boundary after keyboard processing so a simultaneously admitted shot
  before Escape is not mistaken for activity during pause.
- Rendered Asteroid regression passed after the shared controller/HUD changes:
  `PINBALL_ASTEROID_PASS` in `Saved/Logs/Phase6Acceptance.log`. The timeout run retained
  57 kills/three lives/30 seconds; the life-loss run retained 21 kills/zero lives/seven
  seconds. Both awarded 10,000 and returned the same ball with rotation, thrust, firing,
  rebound and pause checks passing.

| Frame cap | Scenario | Spawned | Interceptions | Colonies | Active duration | Bonus |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 60 FPS | Defended | 57 | 55 | 3 | 30.00 s | 10,000 |
| 60 FPS | Initially undefended, then intercepting | 57 | 52 | 0 | 30.00 s | 10,000 |
| 30 FPS | Defended | 57 | 55 | 3 | 30.00 s | 10,000 |
| 30 FPS | Initially undefended, then intercepting | 57 | 52 | 0 | 30.00 s | 10,000 |

The rendered probe assists mouse targeting but uses production input and collision code.
It does not directly increment metrics or call the firing function. Unprotected colony
loss occurs through descending threat sweeps. It requires an explicit pass marker, zero
process exit and clean engine shutdown. Logs are `Saved/Logs/Phase7Acceptance30.log` and
`Saved/Logs/Phase7Acceptance60.log`.

Visual review found and corrected inverted screen travel and undersized orthographic framing.
The final camera shows threats descending toward bottom colonies, reserves space for the HUD,
and includes the arena plane in its clip volume. The probe checks screen direction and mouse
projection to catch those errors. `Saved/Screenshots/WindowsEditor/Phase7Playing.png` and
`Phase7NoColonies.png` capture the rendered gameplay; both images were visually inspected.
Original primitive/emissive assets and the synthesized interception chirp are documented in
`Content/Minigames/PlanetaryDefense/Art/ASSET_PROVENANCE.md`.

## Follow-up: paddle control after return

The reported loss of paddle control was reproduced with a new rendered regression check:
the real Left Arrow binding set the paddle's held state, but its body moved **0.00 degrees**
after the first minigame return. Simulation was enabled and the constraint handle reported
valid, so the earlier state-only checks missed a physical hinge restoration failure.
The failing log is `Saved/Logs/FlipperReturnBeforeFix.log`.

Return now rebuilds each flipper's constraint against its restored Chaos body before
reenabling the motor. Table restoration also preserves the neutral pose staged by the
participant instead of overwriting it with the captured pose before initializing the joint.

Both real-minigame probes now share `FFlipperReturnCheck`: after each return it injects
fresh Left/Right presses independently through the controller, measures body rotation,
checks the other paddle remains neutral, and verifies release returns both paddles to rest.
The strengthened Planetary Defense run passed both endings and both returns at 60 FPS,
measuring **54.38 degrees** of travel for each paddle on each return. Asteroid Field's
timeout and life-exhaustion runs also passed both returned-paddle checks at **54.38 degrees**.
The Editor build also passed. See `FLIPPER_RETURN_PASS`, `PINBALL_DEFENSE_PASS` and
`PINBALL_ASTEROID_PASS` in `Saved/Logs/Phase7Acceptance60.log` and
`Saved/Logs/Phase6Acceptance.log`.

The shared transition suite also passed all **20 scenarios**, including repeated round
trips, timeout, phase pause, held-input suppression, frozen bodies/timers, start failures,
blocked returns, retry, recovery restart and stale callbacks. Marker:
`PINBALL_TRANSITION_PASS cycles=20` in `Saved/Logs/Phase5Acceptance.log`.

The subsequent center-gap ball-feed report and its natural-trajectory regression checks
are recorded in [playable ball-return validation](ball-return.md).

## Reproduce

Build using the command in [quickstart](../quickstart.md). Then run:

```powershell
./Scripts/Run-Phase7Validation.ps1
./Scripts/Run-Phase7Validation.ps1 -FrameRate 30
```

Author or validate actual packages with UnrealEditor-Cmd using `-run=pythonscript` and
`-script="D:/Media/Projects/Unreal/PinballBattle/Scripts/Editor/create_phase7_assets.py"`
or `validate_phase7_assets.py`, plus the project path, `-unattended -nop4 -nullrhi`.
The authoring script preserves existing cabinet associations and can be rerun.

## Limits

These are controlled rendered acceptance runs, not a first-time-player difficulty study.
The synthesized sound asset and playback path are verified; no human listening assessment is
claimed. Ordinary-play objective reachability across all three games, ten-round regression,
packaging, reference-hardware performance and first-time-player acceptance remain assigned to
later phases. The production cabinet retains its explicit development flag pending Phase 8.
The engine's existing standalone ToolsetRegistry `PythonTestRunner` startup error still appears;
it does not prevent gameplay, the acceptance marker or clean exit. There were no asset-script
errors in the final validation runs.
