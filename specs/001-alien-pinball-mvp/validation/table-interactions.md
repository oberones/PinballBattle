# Phase 3 table interactions

Completed 2026-09-16. Scope: T021–T028, repeat-ball table practice. Scoring totals, three-ball accounting,
pause menus and minigame activation remain in their scheduled phases.

## Play

Open `/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet` and press Play. Hold/release **Down**
to launch; **Left/Right** operate the corresponding screen-side flippers. The temporary
HUD shows event counts, not points. A fresh ball appears after a drain. The map explicitly
uses the existing test-only practice GameMode; the reusable GameMode remains production-disabled
until Phase 4. The original physics prototype remains the project's default map.

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' "$PWD/PinballBattle.uproject" /Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet -game -windowed -ResX=1280 -ResY=720
```

## Behavior and physics decisions

- `FBallHandle` identifies the world-local session and each new ball entitlement. Recovery
  repositions the same actor and preserves both IDs; a drain creates a fresh BallId.
- Targets and bumpers emit `FScoringEvent` with category, source/event IDs, sequence, observed
  phase and physical epoch. No producer supplies point values. `FScoreAward` defines the
  future score owner's output; there is no second mutable score implementation here.
- Contact scoring latches at the first side-face/ring collision and rearms only when a
  closest-point query measures separation greater than sphere radius plus 1 cm. Invalid
  collision queries cannot rearm. Landing on a top cap remains a passive support collision.
- Powered bumpers add one bounded radial coil impulse per contact, with an independent
  minimum coil interval. Passive rebounds and rolling/spin remain Chaos-owned. Impulse limiting
  solves the directional velocity budget rather than rescaling away tangential velocity.
- The sphere remains a 3D CCD body and flippers retain physical hinges. Table scale is the
  existing enlarged readable graybox, not a claim of regulation dimensions. Cabinet material
  values separate low-bounce playfield (restitution 0.08), ball (0.30), target faces (0.40)
  and rubber (0.65), with explicit combine modes. Mass is 0.08 kg; speed limit is 1,800 cm/s.
- Lanes evaluate successive sphere-centre positions against local entry and exit planes,
  with radius-aware corridor clearance. Ordered forward entry then exit emits once;
  reverse or side departure cannot complete a traversal. Full departure rearms it.
  A straight swept segment can cross both gates within one rendered frame.
- Escape bounds are oriented with the table. Low motion must persist for ten active seconds,
  with launch and explicitly registered capture areas exempt. Recovery checks a slightly
  expanded sphere and its outgoing path against solids and drain/trigger channels at the
  primary marker, then the backup. If both are blocked, the same ball stays secured with
  event gates closed and retries. Successful return clears spin and old contact/lane progress,
  releases at 220 cm/s toward the flippers, and rejects queued contact callbacks briefly.
- Four targets, two directed lanes, three bumpers, one drain, two return markers and three
  labeled nonblocking objective previews are saved as real Unreal assets. Feedback flashes
  and original chimes follow accepted producer events and never mutate score UI.
- Function contracts are documented at native declarations; Python authoring/validation
  functions include descriptive docstrings. Ignore patterns and binary LFS rules were verified.

The collision-episode design accounts for Epic's documented queued/repeated substep callbacks:
[Physics Sub-Stepping](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine).

## Executed checks

- UE 5.8.2 Development Editor native build passed with MSVC 14.50.35738 and Windows SDK
  10.0.22621.0 (`-NoUBA -NoPCH`).
- Unreal Automation: **3 passed, 0 failed, 0 warnings**. Contact episodes cover 100 sustained
  callbacks, hysteresis, invalid geometry and a new identity. Directed lane tests cover
  ordered/incomplete/reverse/side/high-speed/above-lane travel and departure. Existing tuning
  validation also passes. Report: `Saved/Automation/Interactions/Unit/index.json`.
- Asset authoring and a separate fresh-process read-back both passed with **0 errors and
  0 warnings**, including inventory, explicit references, original SoundWaves, material/tuning
  references, practice isolation, and absence of cabinet/test dependencies in shared assets.

### Rendered acceptance

`ATableInteractionProbe` is opt-in and disabled in Shipping. It drives ordinary mapped
keyboard input for natural cycles, then clearly logs each deliberate shot/recovery fixture.
No teleports or forced drains occur during the 24 natural cycles below. No automatic
escape/trap recovery was needed during those final natural cycles.

| FPS cap | Natural cycles | Natural target / bumper / lane events | Total distinct events including shot fixtures | Natural sources reached |
| --- | --- | --- | --- | --- |
| 30 | 8 | 21 / 24 / 9 | 61 | 8 of 9 |
| 60 | 8 | 15 / 11 / 9 | 42 | 7 of 9 |
| 120 | 8 | 23 / 20 / 11 | 61 | 9 of 9 |

All **four targets, three bumpers and two lanes** were reached through ordinary play;
the 120 FPS run reached every element within four cycles. The finite 30/60 FPS trajectories
did not hit every element, which is recorded rather than treated as deterministic replay.
There were **143 natural typed events plus 21 controlled physical strikes**, each with a
unique EventId, valid SessionId/BallId/epoch/category and increasing per-source sequence.
Each controlled target/bumper strike emitted exactly once and activated its visible lamp.

Every rate also passed these fixtures, retaining exactly one owned ball and three registered
moving bodies throughout:

- Forced stationary trap: no early recovery; one return at 10.000–10.001 active seconds.
- Forced escape: same actor/SessionId/BallId, no scoring event, no drain/replacement.
- Primary marker inside solid playfield: backup marker selected instead.
- Both markers blocked: simulation secured, events/drains closed, no replacement; restoring
  the safe marker resumes the same ball. Each run has four successful forced recoveries.
- Stationary ball in launch and separate capture volumes: no recovery after more than
  ten active seconds in either exemption.
- Stale pre-recovery drain epoch rejected; intentional valid drain accepted once; repeated
  request rejected; next ready ball has a fresh BallId and the same SessionId.

Saved summaries: `Saved/Automation/Interactions/Interactions30.txt`, `Interactions60.txt`,
`Interactions120.txt`; detailed logs: `Saved/Logs/Phase3Interactions30.log`,
`Phase3Interactions60.log`, `Phase3Interactions120.log`. The runner requires the explicit
`PINBALL_INTERACTIONS_PASS` marker; Unreal exit code alone is insufficient.

The original Phase 2 map also passed a fresh two-cycle rendered regression at 60 FPS:
30 separated contacts, 6 bumper contacts, 3 flipper contacts, peak speed 1,743.1 cm/s,
both flippers traveling approximately 54.5–54.7 degrees and correct once-only replacement.
Evidence: `Saved/Logs/Phase2Smoke60.log` and `Saved/Automation/Practice/Smoke60.txt`.

The initial controlled-shot probe needed a completed launch physics step before teleporting
its fixture, and its final drain assertion was corrected to expect a new entitlement ID.
These were test-fixture corrections; all final rates passed the corrected assertions.
Final presentation cleanup aligns text to the inclined playfield. This changes no collision
geometry, tuning, input or event rules used in the acceptance runs.
The saved final presentation was inspected in `Saved/Screenshots/WindowsEditor/Phase3Ready00006.png`;
all labels and lane arrows face the player. A further rendered fixture run passed after that
cleanup (`Saved/Logs/Phase3FinalVisual60.log`), and the final saved-asset read-back also passed.
`git diff --check` passed for source, scripts and feature documentation.

Standalone runs log an engine ToolsetRegistry Python startup error (`PythonTestRunner` absent
in game mode) with the already-enabled ModelContextProtocol plugin, plus pre-existing
EditorDataStorageUI and `r.MotionVectorSimulation` warnings. The user's plugin configuration
was preserved. There were no project runtime errors, ensures or fatal errors in the final
acceptance runs. A sandboxed editor attempt could not access its DDC; approved runs used
the installed cache. Final asset authoring/read-back commandlets report zero errors/warnings.

No constitutional conflict was identified. Shared pinball assets/code have no cabinet theme
or specific minigame dependency. No implementation extension hooks are registered.

## Reproduction

```powershell
$engine = 'C:/Program Files/Epic Games/UE_5.8'
$project = "$PWD/PinballBattle.uproject"
& "$engine/Engine/Build/BatchFiles/Build.bat" PinballBattleEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoPCH "-Log=$PWD/Saved/Logs/Phase3Build.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/create_phase3_assets.py" -unattended -nop4 -nullrhi -nosplash "-abslog=$PWD/Saved/Logs/Phase3AssetCreation.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -run=pythonscript "-script=$PWD/Scripts/Editor/validate_phase3_assets.py" -unattended -nop4 -nullrhi -nosplash "-abslog=$PWD/Saved/Logs/Phase3AssetValidation.log"
& "$engine/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project /Engine/Maps/Entry -unattended -nop4 -nullrhi -nosplash '-ExecCmds=Automation RunTests PinballBattle.;Quit' "-ReportExportPath=$PWD/Saved/Automation/Interactions/Unit" "-abslog=$PWD/Saved/Logs/Phase3Automation.log"
& ./Scripts/Run-Phase3Validation.ps1
```

Validation runs use rendered standalone 1280×720, D3D12/SM5, project scalability, VSync off,
Lumen/ray tracing disabled on Ryzen 7 7800X3D, Radeon RX 7900 XTX and 64 GB RAM, Windows 11 23H2.
This is physics/interaction acceptance, not the later packaged hardware benchmark or human
usability study. Authoring scripts replace only their own tagged actors. No commit is created.

## Lighting readability follow-up — 2026-09-16

Player feedback identified deep angled shadows hiding the ball. The cabinet now uses a
near-overhead key (84 degrees relative to the playfield, intensity 2.5, shadow amount 0.35)
and four shadowless interior Rect Lights (40 lumens each, 5000 K). Reduced ambient occlusion
keeps shallow contact shading around obstacles. The local fill approximates reflected
cabinet illumination without requiring indirect lighting from Lumen; movable Rect Lights
and their shadow controls are described in Epic's
[Rect Light documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/rectangular-area-lights-in-unreal-engine).

`Scripts/Editor/cabinet_lighting.py` is shared by full authoring and the targeted
`update_cabinet_lighting.py` updater. The targeted update preserves gameplay actors,
collision, tuning and explicit references. The saved map and authoring recipe both include
the revision. The successful lighting commandlet reported zero errors/warnings.

The final rendered screenshot `Saved/Screenshots/WindowsEditor/Phase3Playing00007.png`
was inspected for reduced shadow length/depth and the lit ball beside a target. The final
60 FPS probe passed two natural cycles plus all existing strike/recovery/drain fixtures
(`Saved/Logs/Phase3Lighting60.log`, `Saved/Automation/Interactions/Lighting60.txt`). No new
gameplay code or physics tuning was introduced by the lighting change.
