# Phase 2 — first playable pinball

Completed 2026-09-16. Scope: **T009–T020**, repeat-ball practice only.
Validated the Phase 2 working-tree changes on base revision `30876dc`; no commit was created.

The prototype launches, flips, rebounds, drains and automatically prepares one new ball.
Score, three-ball accounting, pause menus, minigames and recovery remain in their later phases.

## Play

Open `Content/Tests/Maps/L_PhysicsPrototype.umap` and press Play, or run from the repository:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' "$PWD/PinballBattle.uproject" /Game/Tests/Maps/L_PhysicsPrototype -game -windowed -ResX=1280 -ResY=720
```

- Hold **Down Arrow** to charge, then release to launch. Charge saturates at 1.25 seconds.
- **Left Arrow** and **Right Arrow** operate the corresponding screen-side flippers.
- Let the ball drain; one replacement appears automatically. Launch again with a fresh press.
- The fixed camera shows the whole playfield. Instructions and launch status stay visible.
- Close the standalone window with Alt+F4. Escape/pause is a Phase 4 feature.

The prototype has a lane on the left of the player view. Returning down that lane reaches the
same drain as the main playfield; it cannot leave an active ball stuck behind the launcher.

## Implementation and configuration

| Tasks | Delivered behavior |
| --- | --- |
| T009 | `UPinballTuningData`, positive/finite validation, material and impulse bounds; preserved 10-second trap window and launch/capture exemptions for Phase 3 |
| T010 | Explicit table, actuator, camera, spawn and bumper references; one current ball; body registry without world/name searches |
| T011 | Chaos sphere collision root, CCD, physical material, bounded impulses and post-physics speed clamp |
| T012 | Two physical flippers with one constrained twist axis, Twist/Swing drive, neutral/held targets and configurable drive strength/damping/torque |
| T013 | Bounded plunger charge, distinct short/full launch and cancellation that returns no launch impulse |
| T014 | Enhanced Input controller-to-pawn forwarding, ready-only launch and explicit fixed table view |
| T015 | Bounded bumper response and once-only drain admission without scoring dependencies |
| T016 | GameMode/flow practice lifecycle, old-ball removal before replacement and cancellation of held actuator commands |
| T017 | Saved ball/flipper/plunger/bumper/drain Blueprints, tuning Data Asset and physical material |
| T018 | Saved prototype table/map, three bumpers, walls, return guides, spawn, camera and controls widget |
| T019 | Tuned geometry, collision, drive direction, ball visibility and project substepping |
| T020 | Rendered 30/60/120 FPS checks below, screenshots, lifecycle adversarial checks and reproducible commands |

Initial tuning: 6-degree incline, 12 cm ball radius, 0.08 kg mass, 1,800 cm/s maximum speed,
65–105 kg·cm/s launch impulse, 55-degree flipper travel, 45 kg·cm/s bumper impulse and 0.1-second
physical bumper cooldown. `PM_Pinball` uses friction 0.12 and restitution 0.65.
Substepping is configured globally at 1/120 second, maximum eight substeps, in DefaultEngine.ini.

`BP_PracticeGameMode` lives under `/Game/Tests` and enables the shared GameMode's explicit
practice configuration. The reusable GameMode default remains disabled. There is no second
manager or parallel rules implementation. The table exposes a stable body manifest for the
later suspension contract; suspension itself is not claimed here.

Layout/materials are authored by `Scripts/Editor/create_phase2_assets.py` as real Unreal
packages. Reruns update prototype assets and replace only actors tagged by that script.
`validate_phase2_assets.py` checks the saved packages in a separate process, including class
references, input actions, sphere root/CCD, table inventory, incline, custom-channel wall
blocking, practice isolation and absence of test-content dependencies in reusable assets.
All meshes are Unreal basic primitives; the simple materials and instructions are original.

## Executed validation

Development Editor build: **PASS**, UE 5.8.2, MSVC 14.50.35738, Windows SDK 10.0.22621.0.
Unreal Automation: **1 test passed, 0 failures, 0 warnings**,
`PinballBattle.Practice.TuningValidation` (10 assertions covering defaults, short/full bounds,
zero mass, infinite drive, inverted impulse range, NaN damping/charge and recovery exclusions).
Final asset creation and separate read-back: **PASS**, each with zero errors/warnings in its
commandlet summary. `git diff --check` passed.

### Rendered practice acceptance

`AFirstPlayableProbe` is placed only in the test map and is inactive unless launched with
`-PinballPracticeProbe`; its tick is disabled in Shipping. It injects normal arrow-key events
through PlayerController, Enhanced Input, controller and pawn. Chaos moves the actual ball
and flippers in the saved map. The 21 cycles below use **no ball teleports or forced drains**.
Each presses both flippers for the first ten seconds, then releases them to allow a natural
drain. Alternate short/full charge produces 12 short and 9 full launches.

| FPS cap / observed steady FPS | Natural cycles | Separated side contacts | Bumper / flipper contacts | Latest short / full launch speed (cm/s) | Peak speed (cm/s) | Play duration range (s) |
| --- | --- | --- | --- | --- | --- | --- |
| 30 / 30.0 | 7 | 128 | 25 / 25 | 862.2 / 1308.2 | 1754.2 | 17.88–27.64 |
| 60 / 60.0 | 7 | 100 | 12 / 22 | 863.8 / 1310.2 | 1793.9 | 8.14–23.26 |
| 120 / 120.0 | 7 | 86 | 5 / 23 | 861.5 / 1311.3 | 1620.8 | 13.13–21.17 |

Total: **21 complete cycles, 314 separated contacts, 42 bumper contacts and 70 flipper contacts**.
Persistent playfield support and repeated substep notifications are excluded from those
contact counts; a side surface rearms only after measurable ball separation. Each rate
observed both flippers moving approximately 54.6–54.7 degrees and returning to neutral.
Flipper contacts returned balls upfield; launch strength remained distinct at all three rates.
All launches reached the upper playfield, all drains reset without editor intervention, and
no tested ball crossed the escape bounds or penetrated the floor. Trajectories vary with
frame rate; deterministic simulation is not claimed.

Every run also checks: no more than one live ball; no accumulated registry entries; correct
camera; no launch after cancellation; rejection of ready-ball drains and double launches.
After its seven natural cycles, each run injects one additional drain and immediately repeats
that request. Only the first succeeds, followed by exactly one ready ball and three registered
bodies (ball plus flippers). These **three adversarial drains are excluded** from the natural
cycle count above.

Rendered screenshots were inspected for table coverage, screen-side flippers, readable
instructions and contrasting ball. Example: `Saved/Screenshots/WindowsEditor/PracticePlaying00006.png`.
The final material-only cleanup moved the ball material into Framework to remove a test-content
dependency; the layout and physical settings used by the acceptance run are unchanged.

### Evidence and reproduction

Run from the repository in PowerShell:

```powershell
$engine = 'C:/Program Files/Epic Games/UE_5.8'
$project = "$PWD/PinballBattle.uproject"
& "$engine/Engine/Build/BatchFiles/Build.bat" PinballBattleEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoPCH "-Log=$PWD/Saved/Logs/Phase2Build.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/validate_phase2_assets.py" -unattended -nop4 -nullrhi -nosplash "-abslog=$PWD/Saved/Logs/Phase2AssetValidation.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -unattended -nop4 -nullrhi -nosplash '-ExecCmds=Automation RunTests PinballBattle.Practice;Quit' "-ReportExportPath=$PWD/Saved/Automation/Practice/Unit" "-abslog=$PWD/Saved/Logs/Phase2Automation.log"
& ./Scripts/Run-Phase2Validation.ps1
```

The runner starts three independent rendered standalone processes at 1280×720 with VSync off.
It requires actual `PINBALL_PRACTICE_PASS` markers, not just a process exit code. Unreal's normal
shutdown returned zero even on earlier failed probes, so process exit alone is insufficient.
NullRHI is used only for data tests and saved-asset inspection, never for physics-view evidence.

Local evidence (generated and ignored):

- `Saved/Logs/Phase2Build.log`: successful final native build.
- `Saved/Logs/Phase2AssetCreation.log` and `Phase2AssetValidation.log`: saved asset checks.
- `Saved/Automation/Practice/Unit/index.json`: one successful Automation test.
- `Saved/Logs/Phase2Practice30.log`, `Phase2Practice60.log`, `Phase2Practice120.log`: positions,
  state transitions, charge/speed/duration per cycle and final pass markers.
- `Saved/Automation/Practice/Practice30.txt`, `Practice60.txt`, `Practice120.txt`: run summaries.

Machine: Ryzen 7 7800X3D, Radeon RX 7900 XTX (driver 26.8.1), 64 GB RAM, Windows 11 23H2.
D3D12/SM5, project default scalability, 1280×720, Lumen/ray tracing disabled. The three tests
ran concurrently. This checks physics behavior at the observed frame rates; it is not the
later packaged reference-hardware performance benchmark or a human usability/feel study.

## Fixes and remaining limits

Runtime iteration corrected the custom-channel floor/wall blocking, inverted flipper drive,
too-narrow flipper gap, return-guide end-cap pockets and launch-lane ball trap. The camera was
oriented for a bottom-of-screen flipper view and the ball received a contrasting material.

The local 2.49 GB shared precompiled header stalled the initial build; `-NoUBA -NoPCH` completed
the native checks. No compiler or engine installation changes were needed. Earlier authoring
iterations corrected reflected constructor initialization, Blueprint default persistence and
Python rotation/enum bindings before the successful saved-asset checks.

Standalone still emits the pre-existing engine EditorDataStorageUI registration warnings and
an `r.MotionVectorSimulation` render-thread warning. The final physics runs contain no project
errors, ensures or fatal errors. Optional profiling DLLs and non-Windows platform SDKs are not
installed, as in Phase 1.

Ten-second trap/escape recovery belongs to T025 and is not implemented by this phase. The
automated trajectories are finite coverage, not a guarantee against every possible trap.
Pause, score, three-ball sessions and minigames are intentionally not part of this checkpoint.
No constitutional conflict was identified; shared native pinball contains no cabinet theme or
specific minigame dependency. No implementation extension hooks are registered.
