# Phase 1 setup validation

Completed 2026-09-15 America/New_York (build/launch logs use 2026-09-16 UTC).

Scope: T001–T008 only. Phase 1 creates an empty prototype and framework/input foundations.
The initial flow remains BOOT. Start/pause menus, ball play, scoring and minigames belong to
later tasks; this checkpoint does not claim a playable pinball game.

## Installed tools

- Unreal Engine 5.8.2, changelist 56702186, at `C:/Program Files/Epic Games/UE_5.8`.
- Visual Studio Build Tools 2026 18.10.1 (installation version 18.10.12210.168), at
  `C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools`.
- UnrealBuildTool selected MSVC **14.50.35738**, compiler file version 19.50.35738.0,
  in the installer family directory `VC/Tools/MSVC/14.50.35717`.
  The directory name is not the compiler patch version; 35738 clears UE's ban through 35722.
- Windows SDK and UCRT **10.0.22621.0**.
- .NET Framework **4.8 SDK and targeting pack**, required by the Editor's SwarmInterface.
- Unreal's bundled .NET 10.0 runtime is used by UnrealBuildTool.
- Git LFS **3.6.1**, with clean/smudge/process filters already configured.

The compiler and SDKs were absent at initial inspection and installed with user-approved
commands. `.vsconfig` records the required components. Build Tools is sufficient for command-line
compilation; a Visual Studio IDE/extension is not required for this phase.

## Commands

From the repository root in PowerShell:

```powershell
$pinballEngine = 'C:/Program Files/Epic Games/UE_5.8'
$pinballProject = (Resolve-Path './PinballBattle.uproject').Path
& "$pinballEngine/Engine/Build/BatchFiles/Build.bat" PinballBattleEditor Win64 Development "-Project=$pinballProject" -WaitMutex -NoHotReloadFromIDE "-Log=$PWD/Saved/Logs/Phase1Build.log"
```

After the native build, author any missing foundation assets, then validate saved content:

```powershell
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $pinballProject /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/create_phase1_assets.py" -unattended -nop4 -nullrhi -nosplash "-abslog=$PWD/Saved/Logs/Phase1AssetCreation.log"
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $pinballProject /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/validate_phase1_assets.py" -unattended -nop4 -nullrhi -nosplash "-abslog=$PWD/Saved/Logs/Phase1AssetValidation.log"
```

The authoring script preserves existing assets. The validator reads their actual saved
properties in a separate Editor process and fails on wiring or binding mismatches.
NullRHI is used only for content authoring/read-back, not the rendered standalone check:

```powershell
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $pinballProject /Game/Tests/Maps/L_PhysicsPrototype -game -windowed -RenderOffscreen -ResX=1280 -ResY=720 -seconds=15 -unattended -nop4 -nosplash '-ExecCmds=HighResShot 1280x720' "-abslog=$PWD/Saved/Logs/Phase1Standalone.log"
```

To open the prototype interactively, run the following without the automated timeout:

```powershell
& "$pinballEngine/Engine/Binaries/Win64/UnrealEditor.exe" $pinballProject /Game/Tests/Maps/L_PhysicsPrototype -game -windowed -ResX=1280 -ResY=720
```

## Verification status

- T001: engine files, compiler file version, SDK directories and UBT-selected toolchain verified.
- T002–T005: project, module, framework classes and flow declarations compiled successfully.
- T003: `git check-attr` confirms LFS and `-text` on `.uasset`/`.umap` paths before asset creation.
  `git check-ignore` confirms generated build/cache/log/IDE files and local secrets are ignored.
- T006–T007: nine `.uasset` files and one non-partitioned `.umap` authored and saved by Unreal.
  The standalone read-back validator confirms the GameMode/controller/pawn/GameState class
  references, one PlayerStart, no extra placed pawn, both controller mapping references,
  exact key mappings and Boolean action types. Pause alone permits execution while paused.
  Plunger has no trigger modifiers that would suppress ordinary press/hold/release phases.
- T008: Development Editor build passed (13 actions, 65.29 seconds, exit 0). Saved asset
  authoring and validation each exited 0 with zero errors/warnings in the commandlet summary.
  Standalone ran for its 15-second engine time limit, rendered a 1280×720 frame and exited 0.

| Evidence | Observed result |
| --- | --- |
| `Saved/Logs/Phase1Build.log` | `Result: Succeeded`; MSVC 14.50.35738 / SDK 10.0.22621.0 |
| `Saved/Logs/Phase1AssetCreation.log` | `PINBALL_PHASE1_ASSETS_CREATED` |
| `Saved/Logs/Phase1AssetValidation.log` | `PINBALL_PHASE1_ASSET_VALIDATION_PASSED` |
| `Saved/Logs/Phase1Standalone.log` | `Game class is 'BP_PinballGameMode_C'`; foundation flow BOOT |
| Standalone input readiness log | `Common=IMC_Common (1 mappings), Pinball=IMC_Pinball (3 mappings), Pawn=BP_PinballControlPawn_C_0, View=BP_PinballControlPawn_C_0` |
| Standalone shutdown log | Normal timed `RequestExitWithStatus(0, 0, ...)`, followed by `LogExit: Exiting.` |
| `Saved/Screenshots/WindowsEditor/HighresScreenshot00000.png` | Inspected: blank frame, expected for the empty map without geometry or UI |

Rendering used D3D12/SM5 on AMD Radeon RX 7900 XTX, driver 26.8.1, with Ryzen 7 7800X3D
and Windows 11 23H2. This smoke check is not the later reference-hardware performance test.
The standalone log contains no errors, ensures or fatal errors. It does contain engine
warnings about EditorDataStorageUI widget registration and `r.MotionVectorSimulation` render-
thread access. These did not prevent startup/rendering/shutdown and remain recorded limitations.
Platform discovery also reports missing SDKs for non-Windows platforms, outside this scope.

Initial attempts: the sandboxed build stalled before logging and was stopped. The first
unsandboxed build exposed the missing .NET Framework SDK; that dependency was installed and
the build retried. UBT warns that the optional Visual Studio SDK is absent and editor/IDE
integration is disabled. This does not mean the C++ compiler or Windows SDK is missing.
An initial asset-authoring attempt exposed a Python `FKey` constructor mismatch; the script
now uses the reflected `key_name` property, and creation plus independent read-back passed.

## Architecture review

One runtime module uses C++20 and engine-native GameMode/GameState/PlayerController/Pawn
classes. GameMode owns the flow component; GameState exposes a read-only flow projection.
The controller owns only its two configured input contexts and explicitly selects the pawn's
foundation camera. Blueprint defaults provide asset references; core C++ has no cabinet or
minigame asset paths, name-based Actor searches, scoring service or minigame subsystem.
No constitutional conflict was identified for this scoped foundation.
