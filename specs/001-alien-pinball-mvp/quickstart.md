# Quickstart and Validation Guide

Phase 1 now supplies the C++ project, framework Blueprints, input assets and empty
`L_PhysicsPrototype` map. See [setup validation](validation/setup.md) for executed commands
and evidence. Later milestone commands/scenarios below remain prospective until their tasks
are implemented. The empty foundation does not yet provide menus or ball gameplay.

## Prerequisites and foundation

- Unreal Engine 5.8.2; local discovered installation is `C:/Program Files/Epic Games/UE_5.8`.
- Supported Visual Studio/MSVC and Windows SDK from [research.md](research.md). Verify a clean
  build at M1 before gameplay work; use UnrealBuildTool's installed SDK rules if patch versions
  differ. Enable Game Development with C++.
- M1 creates `PinballBattle.uproject` as a blank C++ desktop project in the existing repository,
  preserving `.specify`, `.agents`, `.codex` and `specs`. Enable Enhanced Input and Niagara;
  development test support uses Automation/FunctionalTesting. Do not add unrelated templates.
- Phase 1 configures `/Game/Tests/Maps/L_PhysicsPrototype` as the startup map with the project
  GameMode and collision channels, following the first-playable task ordering. T036 switches
  startup to `/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet`. M5 adds transition test maps;
  M6–M8 add production minigame maps. T092 configures shipping cook exclusions for `/Game/Tests`.
- M1 configures Git LFS for .uasset/.umap, if available in the team's Git setup, before adding
  binary content; generated Binaries/Intermediate/Saved/DerivedDataCache remain ignored.

Run later commands from the project repository in PowerShell:

```powershell
$pinballRepo = 'D:/Media/Projects/Unreal/PinballBattle'
$pinballEngine = 'C:/Program Files/Epic Games/UE_5.8'
$pinballProject = Join-Path $pinballRepo 'PinballBattle.uproject'
& "$pinballEngine/Engine/Build/BatchFiles/Build.bat" PinballBattleEditor Win64 Development "-Project=$pinballProject" -WaitMutex
```

Expected: successful Development Editor build, no missing module/plugin or compiler errors.
Do not substitute a successful documentation check for this build evidence.

## Playable checkpoints

| Milestone | Entry point after implementation | What to do / expected result |
| --- | --- | --- |
| M1 | /Game/Tests/Maps/L_PhysicsPrototype | Launch empty shell; verify GameMode/controller/pawn and loaded input contexts; BOOT state, no menus yet |
| M2 | /Game/Tests/Maps/L_PhysicsPrototype | Short/full plunger launch, both flippers, repeat at 30/60/120 FPS; no tunneling |
| M3 | Graybox L_AlienCabinet practice loop | Hit all object categories, traverse two lanes, drain/reset and recover trapped ball |
| M4 | L_AlienCabinet | Three-ball score session, pause, game over, clean restart and Quit |
| M5 | /Game/Tests/Maps/L_TransitionTest | Trigger test stub from real table; return with one award and unchanged ball count |
| M6 | L_TransitionTest selecting Asteroid definition | Rotate/thrust/fire, timeout and life exhaustion; return to table |
| M7 | Same harness selecting Defense definition | Mouse aim/Space intercept, colony loss, full timer even with zero colonies |
| M8 | Same harness selecting Assault definition | Horizontal fire/dodge, destruction/survival score, both end conditions |
| M9 | L_AlienCabinet | Three real objectives; full-session and failure/pause/race regression |
| M10 | Themed L_AlienCabinet | Objective/UI readability, original assets, local FX/audio and collision regression |
| M11 | Packaged game on reference desktop | All measurable outcomes and first-time playtests |

The harness uses the shipping lifecycle and transition, with editable test definition selection;
it must not contain a second simplified transition implementation. Opening an arena map alone
is an authoring inspection, not the full gameplay validation path, because its runtime root
expects the common world/session setup.

Example standalone launch after M4:

```powershell
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor.exe" $pinballProject /Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet -game -log -windowed -ResX=1920 -ResY=1080
```

Use Start, Left/Right flippers, hold/release Down to launch, and Escape to pause. During
Asteroid use Left/Right/Up/Space; Defense uses mouse/Space; Assault uses Left/Right/Space.
Instructions consume a fresh Space/Enter press before the active 30-second clock begins.

## Automated validation after tests are implemented

Planned registered test filters:

- `PinballBattle.Score`: base points, multipliers/rounding/caps, profile examples, duplicate
  event/result rejection, malformed metrics and stale sessions.
- `PinballBattle.Flow`: legal state edges, three drains, terminal score lock, reset, pause
  continuation and generation invalidation.
- `PinballBattle.Transition`: loads L_TransitionTest through a latent Automation wrapper and
  runs its APinballTransitionFunctionalTest actors; exercises real bodies, contexts and roots.

```powershell
$pinballReport = Join-Path $pinballRepo 'Saved/Automation/PinballBattle'
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $pinballProject -unattended -nop4 '-ExecCmds=Automation RunTest PinballBattle;Quit' "-ReportExportPath=$pinballReport"
```

Expected: all registered PinballBattle tests run and pass with a nonzero test count; inspect
report and logs for failures, missing maps, skipped tests or timeouts. Do not accept process
exit alone as proof of coverage. Use the rendering-capable Editor/Test Automation window for
input/camera/physics tests that need a viewport; do not use NullRHI to claim visual/feel results.
Run pure score/flow filters separately while iterating, then transition tests after changes
that affect that boundary. Epic documents command-line test filters and report export in
[Run Automation Tests](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine).

## Full pinball and minigame acceptance

1. Launch/Start ten times (SC-001). Record score zero and three balls each time.
2. Complete five three-ball sessions (SC-002). Verify 3→2→1→0, 650 points for one target,
   bumper and lane at multiplier one, correct game-over score and restart reset.
3. Perform 20 short/full launches and observe at least 100 relevant contacts (SC-003).
   Force an escape/trap in development, verify recovery without decrement or award.
4. Trigger all three minigames by ordinary play in one three-ball session (SC-004).
   Debug-trigger harness tests do not substitute for this reachability demonstration.
5. Repeat ten round trips for each game. Record before/after SessionId, BallId, score and
   remaining balls; expect only the accepted bonus to change score. Inspect dormant actors,
   timer counts and mappings after each return for leaks.
6. Test 30-second timeout and both shooter life-loss endings; Defense with no colonies still
   lasts 30 seconds. Verify three-second results within the spec's 0.25-second tolerance.
7. Audit low/high profile outcomes from [gameplay contracts](contracts/gameplay.md). Replay
   duplicate results and stale-session results; expect no extra points or animations.
8. Pause/resume three times in each mode and all transition phases. Compare body positions,
   local lives, score and remaining timers; instructions/results/guard clocks do not advance.
9. Inject each failure in the [transition evidence table](contracts/transitions.md): missing
   runtime/camera, failed start, trigger/drain races, held keys, stale loads on reset, blocked
   release and repeated cleanup. Record zero-bonus safe recovery or secured fatal-recovery UI.
10. Review include and asset references: shared pinball has no AlienInvasion dependencies;
    each minigame can initialize/play/end/cleanup with the other two definitions absent in a
    test cabinet. Shipping cabinet validation still requires all three definitions.

## Packaged build and performance acceptance

After M9, run a Development packaging pass so soft references and startup preloading are
validated outside Editor. Production map cook references must be configured first.

```powershell
$pinballPackage = Join-Path $pinballRepo 'Saved/Packages/MVP'
& "$pinballEngine/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$pinballProject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$pinballPackage"
```

Run the generated Windows executable from the archive output. Expected: all three minigame
assets load, common controller/GameMode stays alive across runs, no debug dependency, and
normal Quit exits. No packaging or executable is produced by this planning step.

Reference benchmark: Windows 11 x64, Ryzen 5 5600-class CPU, RTX 3060 12 GB-class GPU, 16 GB RAM,
SSD; 1080p High scalability, Lumen and hardware ray tracing disabled, conventional lighting,
VSync off, no FPS cap while measuring. Record actual hardware, OS, driver, build revision,
quality overrides and engine version; equivalent replacement needs a documented comparison.

Capture Unreal Insights/frame timings for at least five minutes per mode, using repeated
30-second runs for each minigame. Mark active play separately from instructions/results and
loading. Report average FPS, 99th-percentile active-play frame time and maximum transition
freeze; targets are >=60 FPS, <=20 ms and <=250 ms respectively. Include first/cold activation
and warmed repeat cases; do not hide a first-use hitch in warm-only results. Boot may load
asynchronously before Start, but its UI must remain responsive. Then cap to 60 FPS for a feel
pass and repeat M2 behavior at 30/60/120 FPS for stability rather than performance acceptance.

Recruit five first-time testers. Using only in-game instructions, at least four must launch,
identify all objectives and operate each minigame; at least four must rate ball readability
and flipper responsiveness >=4/5. Record observations and tuning changes, then rerun affected
physics/integration checks. The target hardware and testers are implementation prerequisites,
not resources confirmed available during planning.

## Evidence and completion

Keep local logs/traces/reports under Saved/ (ignored). Summarize milestone acceptance and
SC-001–010 evidence in a future tracked validation record with build/revision, machine/settings,
scenario counts, results and known defects. Do not mark the game complete from a checklist of
planned tests. Source/content edits require appropriate build and affected runtime checks;
this planning-only deliverable requires document consistency validation only.
