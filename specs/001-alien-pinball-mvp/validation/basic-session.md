# Phase 4 basic-session validation

Implemented 2026-09-16: T029–T036. The default cabinet now provides Start, delegate-bound
score/balls/multiplier HUD, three-ball accounting and a locked Game Over screen. Physics and
interaction producers remain shared with practice content. `BP_AlienGameMode` references
`DA_AlienCabinet`; the cabinet explicitly declares development operation without minigames.
The final three-minigame production validation remains Phase 9 work.

## Build and assets

- UE 5.8.2 Development Editor build passed using MSVC 14.50.35738 and Windows SDK 10.0.22621.0.
- Real packages authored by `Scripts/Editor/create_phase4_assets.py`: four widget Blueprints,
  scoring/cabinet Data Assets, cabinet GameMode and the updated cabinet map/controller.
- `validate_phase4_assets.py` ran in a fresh process: `PINBALL_PHASE4_ASSET_VALIDATION_PASSED`,
  zero commandlet errors/warnings. It reads the saved scoring values, references, screen kinds,
  three-ball/timing constants, pause action and map GameMode configuration.
- Git LFS attributes verified for the new `.uasset`/`.umap` packages. Existing generated-file
  ignore rules already cover this C++/Unreal/Python workflow; no ignore changes were necessary.

## Deterministic checks

`PinballBattle` Automation: **6 passed, 0 failed**. Report:
`Saved/Automation/Phase4Tests/index.json`; log: `Saved/Logs/Phase4Automation.log`.

| Test | Checked behavior |
| --- | --- |
| Score.TableAwards | 100+50+500=650; x2=1300; floor once (100.75x2=201); duplicate event/episode rejection; stale session/ball/epoch; paused gate; invalid multiplier; captured profile; duplicate/negative/NaN/infinite/unbounded profile values; missing profile; int64 conversion/addition overflow |
| Flow.EdgesAndPause | Legal/illegal edges; one saved underlying phase; no nested pause or repeated resume; no launch/drain while paused; atomic lost-state gate; terminal and restart edges |
| Flow.RestartScoreLock | Two resets, score zero, default balls/multiplier, rejected prior-session callbacks, immutable final score |
| Interactions.ContactEpisodes | Existing contact deduplication/re-arm regression |
| Interactions.DirectedLane | Existing ordered traversal/reverse/incomplete-path regression |
| Practice.TuningValidation | Existing physical tuning and invalid-value regression |

The test fixture initially double-initialized an Unreal world and later violated TArray's
self-reference guard while constructing duplicate input. Both fixture defects were corrected;
the six-test result above is the completed rerun, not the interrupted attempts.

Practice regression also passed at 60 FPS (`Run-Phase3Validation.ps1 -Rates 60
-CyclesPerRate 2 -RunLabel Phase4Regression`): two natural cycles, 19 typed events, all
controlled target/bumper/lane fixtures and four trap/escape/blocked-path recoveries. Evidence:
`Saved/Logs/Phase3Phase4Regression60.log`, marker `PINBALL_INTERACTIONS_PASS`.

## Rendered acceptance

Run `./Scripts/Run-Phase4Validation.ps1`. It launches D3D12 rendering at 1280x720, capped 60 FPS,
using the actual cabinet map, controller, Enhanced Input, Chaos ball, producers, widgets and
GameMode. Hardware: Ryzen 7 7800X3D, Radeon RX 7900 XTX, Windows 11 23H2. This is functional
acceptance, not the later reference-hardware performance benchmark.

`ABasicSessionProbe` is disabled unless `-PinballSessionProbe` is explicitly supplied, and is
always disabled in Shipping. It performs three natural scored games (nine mapped plunger
launches and nine physical drains), observes 3→2→1→0 in each, rejects repeated drain requests,
checks final score lock, performs two restarts and invokes production Quit. No teleport or
forced drain substitutes for those nine physical drains. Pause and reset details are in
[pause.md](pause.md) and [restart.md](restart.md).

Final run result: `PINBALL_SESSION_PASS`, process exit 0 and clean `LogExit: Exiting.`
Three final scores were 2,150 / 2,550 / 2,550; all nine drains were counted once. The extra
ready-state charged-plunger cancellation and all three fresh-input resume checks also passed.

Evidence: `Saved/Logs/Phase4Session.log`, `Saved/Automation/Phase4/Session.txt`, and rendered
`Start.png`, `Ready.png`, `Playing.png`, `Pause.png`, `GameOver.png` in that same evidence
directory. Start, Pause and Game Over captures were visually inspected: readable text/buttons,
visible score/ball count, and the menu column does not cover the playfield.

This is automated playable acceptance with rendered-image inspection, not a human playtest or
the Phase 11 usability study. Standalone `-game` logs contain an engine experimental
ToolsetRegistry Python startup error (`PythonTestRunner` is unavailable in game mode); this
does not prevent gameplay or clean Quit. No engine/plugin files were modified.

## Reproduction

```powershell
$engine = 'C:/Program Files/Epic Games/UE_5.8'
$project = "$PWD/PinballBattle.uproject"
& "$engine/Engine/Build/BatchFiles/Build.bat" PinballBattleEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE "-Log=$PWD/Saved/Logs/Phase4Build.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/create_phase4_assets.py" -unattended -nop4 -nullrhi
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/validate_phase4_assets.py" -unattended -nop4 -nullrhi
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -unattended -nop4 -nullrhi '-ExecCmds=Automation RunTests PinballBattle' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$PWD/Saved/Automation/Phase4Tests"
./Scripts/Run-Phase4Validation.ps1
```

NullRHI is used only for authoring/read-back and deterministic Automation; the session harness
renders with the real graphics backend. Previous practice remains in `L_PhysicsPrototype` and
the preserved `L_TableInteractions`. `Run-Phase3Validation.ps1` now selects the latter so its
unlimited-ball acceptance cannot accidentally change the production cabinet's lifecycle.
