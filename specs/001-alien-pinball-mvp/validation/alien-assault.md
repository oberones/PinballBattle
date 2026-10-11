# Phase 8: Alien Assault validation

Implemented on 2026-10-10 for T083–T089 / US5. The spec-quality gate passed with
16 checked items and no unchecked items. No `.specify/extensions.yml` hooks exist.

## Play

Open `/Game/Tests/Maps/L_AssaultTest` for the independent cabinet, or enter the magenta
Assault objective on the right of `/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet`.
The test cabinet contains only the Assault definition and uses the production transition.
Without `-AssaultProbe`, the test map is playable normally.

Confirm the instructions with a fresh Space/Enter press. Left/Right moves horizontally
along the arena bottom; hold Space to shoot upward; Escape pauses. Magenta saucers fire
magenta shots toward the cyan ship. Player shots are yellow. Three local lives and a
flashing two-second respawn shield are independent of the three pinball balls. Timeout
at 30 active seconds succeeds; losing all lives ends early and retains earned performance.
Each destroyed alien's moving formation slot is refilled after three active seconds.
Surviving aliens are never replaced on a wave timer.

The shared profile awards `min(10000, 250 * EnemiesDestroyed + 100 * floor(SurvivalSeconds))`.
Four kills/10 seconds gives 2,000; 28 kills/30 seconds gives 10,000. Local points are
100 per kill, with 10 per completed survival second added at the end. UI reads the shared
clock and local status; it owns neither scoring nor gameplay timers.

## Implementation

- The runtime starts twelve enemies in two rows. Each accepted destruction schedules only
  that slot to respawn three active seconds later, joining the formation's current motion.
  Pausing freezes these deadlines; cleanup discards them and ending closes respawns.
  The definition allows up to 120 kills, the theoretical ceiling for twelve slots over
  thirty seconds. The runtime owns formation membership, shot admission, local lives,
  protection, feedback and result metrics. The shared runtime owns the terminal latch,
  active clock, actor registry and cleanup.
- Enemy formation motion uses active time. Both moving enemy contacts and ship movement
  query swept collision. Player and enemy projectiles sweep their entire segment, expire
  at the arena edge or after five active seconds, and carry the accepted run identity.
  Runtime membership and one-time enemy resolution reject stale and duplicate hits.
- Every accepted destruction enlarges the saucer briefly and plays the original synthesized
  cue. Owned audio is stopped on cleanup. All run actors belong to the streamed arena.
- Real Blueprint, map, input, material, sound, HUD and definition assets are saved beneath
  `Content/Minigames/AlienAssault`. The scoring profile and cabinet now include Assault,
  preserving the other associations. The production development flag remains for Phase 9's
  full integration gate. No specific-game case was added to shared flow or scoring code.
- The only existing C++ file changed outside Assault is the Asteroid test probe: an explicit
  `Components/InputComponent.h` include fixes an incomplete type exposed without shared PCHs.

## Executed checks

Environment: Unreal Engine 5.8.2 (CL 56702186), Development Editor Win64, MSVC 14.50.35738,
Windows SDK 10.0.22621.0; Ryzen 7 7800X3D and Radeon RX 7900 XTX, Windows x64.
Rendered validation uses D3D12, 1280 × 720, VSync off and project quality defaults.

- Development Editor build passed. The existing 2.49 GB shared precompiled header stalled
  MSVC; compiling without it succeeded. The reproducible build switches are below. This
  changes no project build settings and does not remove the user's caches.
- Unreal Automation: **8 passed, 0 failed, 0 not run**. Assault, Asteroid, Defense,
  flow/pause, restart score lock, shared lifecycle and both score suites ran.
  Assault covers pre-start and terminal gates, three lives, protection, stale/duplicate
  kills, shot rate and expiry, real hostile-projectile sweeps, owned enemy contact,
  unregistered enemy rejection, wave supply, both endings, replay cleanup, local survival
  points and the low/high bonus examples (including flooring fractional survival time).
  Evidence: `Saved/Automation/Phase8/index.json`, `Saved/Logs/Phase8Tests.log`.
- Fresh-process asset validation passed (`PINBALL_PHASE8_ASSET_VALIDATION_PASSED`): saved
  packages, camera, controls, HUD, sound, metrics, weights/floor rule, Assault-only cabinet,
  three production associations and no hard/soft sibling content dependencies. The source
  include audit also found no Asteroid/Defense imports in Assault gameplay.
- The rendered screenshot `Saved/Screenshots/WindowsEditor/Phase8Playing.png` was visually
  inspected: bottom ship, upward shots, moving enemies, distinct projectile colors, arena
  boundaries and readable timer/score/lives/controls. The probe verifies screen direction,
  not merely the sign of a world-space coordinate.
- Rendered acceptance passed at both 60 and 30 FPS caps, with two round trips per cap.
  The probe exercised Enhanced Input Left/Right/Space, bounded horizontal movement,
  screen-right direction, real player/hostile projectile sweeps, observed destruction
  flashes and playing audio components, native pause, protected hits, one terminal result,
  terminal raw points and central bonuses. SessionId, BallId and three pinball balls stayed
  unchanged. Both primary and backup releases fed the ball to a paddle in 1.55–1.57 seconds;
  actual returned paddle movement measured 54.10–54.63 degrees, with independent fresh
  input and neutral release. Each runner required the explicit `PINBALL_ASSAULT_PASS`
  marker, zero exit code and clean engine shutdown.

The following results were recorded for the initial timed-wave implementation, before
the per-alien respawn update below.

| FPS cap | Ending | Kills | Lives | Active duration | Bonus |
| --- | --- | ---: | ---: | ---: | ---: |
| 60 | Timeout success | 60 | 3 | 30.00 s | 10,000 |
| 60 | Lives exhausted | 0 | 0 | 6.80 s | 600 |
| 30 | Timeout success | 59 | 3 | 30.00 s | 10,000 |
| 30 | Lives exhausted | 0 | 0 | 6.80 s | 600 |

Logs: `Saved/Logs/Phase8Acceptance60.log`, `Saved/Logs/Phase8Acceptance30.log` and
`Saved/Logs/Phase8Assets.log`. Early losses correctly retain survival points even without
kills; the automation separately covers earned destruction points on life exhaustion.
Initial probe failures revealed inadequate assisted dodging and an incorrect assumption
that only injected hostile shots would cause damage. Both were corrected; ordinary enemy
fire remains enabled. The final build also passed after assigning an Assault-specific
collision profiler label. T083–T089 are complete.

## Follow-up: individual alien replacements

The requested behavior now replaces each destroyed alien after three active seconds in
its own moving formation slot. There is no periodic wave reset or wait for a full clear.
Surviving actors keep their identity and movement phase; independent kills have independent
deadlines. The new occupant joins the formation's current position, receives a short firing
delay, and earns no points until destroyed. Native pause freezes respawn time, and the
shared terminal gate and cleanup prevent late replacements.

The saved definition and authoring scripts now allow 120 destructions, covering the maximum
possible slot replacements over thirty seconds, while retaining the existing bonus cap.
`Scripts/Editor/update_phase8_respawns.py` migrates the existing definition and runtime
Blueprint without rewriting cabinet maps. Instructions explain the three-second behavior.

Executed after this update:

- Development Editor build passed.
- `PinballBattle.Assault.Rules` passed, including survival beyond the old six-second reset,
  staggered kills, empty slot at 2.99 seconds, replacement after three seconds, correct
  moving-slot position, survivor identity, duplicate hits, pause, no respawn points,
  replay and terminal cancellation. Report: `Saved/Automation/AssaultRespawns/index.json`.
- Fresh-process asset validation passed; log: `Saved/Logs/AssaultRespawnAssetValidation.log`.
- Rendered 60 FPS-cap acceptance passed both endings and both primary/backup ball returns:
  timeout had 99 kills, three lives, 30.00 active seconds and a 10,000 bonus; life exhaustion
  had zero kills, zero lives, 6.82 seconds and a 600 survival bonus. Both returned paddles
  passed actual movement/release checks. Log: `Saved/Logs/Phase8Acceptance60.log`.
- Python compilation and whitespace checks passed. The earlier 30 FPS results above apply
  to the original timed-wave build; this follow-up used the targeted rules test and 60 FPS
  rendered acceptance.

## Reproduce

From the repository root:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' PinballBattleEditor Win64 Development '-Project=D:/Media/Projects/Unreal/PinballBattle/PinballBattle.uproject' -WaitMutex -NoUBA -UBANoDetour -NoCache -NoPCH -NoSharedPCH
./Scripts/Run-Phase8Validation.ps1
./Scripts/Run-Phase8Validation.ps1 -FrameRate 30
```

Use UnrealEditor-Cmd with the project and `-run=pythonscript`,
`-script="D:/Media/Projects/Unreal/PinballBattle/Scripts/Editor/create_phase8_assets.py"`,
`-unattended -nop4 -nullrhi` to regenerate assets; use `validate_phase8_assets.py` to inspect
saved packages. Run authoring with other Unreal processes closed because loaded packages
can hold file locks. Original asset provenance is in
`Content/Minigames/AlienAssault/Art/ASSET_PROVENANCE.md`.

## Limits

The acceptance probe uses assisted aim/dodging and controlled hostile-shot placement;
its bullets, collision, input, rewards and return use production code. It does not establish
human difficulty or ordinary-play reachability of all three cabinet objectives. Human
listening, first-time-player testing, packaging, ten round trips per game and performance
acceptance remain later-phase work. Audio playback is asserted programmatically.
The existing engine ToolsetRegistry `PythonTestRunner` startup error appears in standalone
logs; it does not prevent the gameplay checks or clean shutdown.
