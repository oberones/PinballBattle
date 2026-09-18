# Phase 6 — Asteroid Field

**Closeout**: 2026-09-18. T067–T074 are complete, including follow-up aiming and steering
fixes. The user accepted the gameplay and requested commit/PR handoff. All current Phase 6
work is ready for review; remaining MVP tasks T075–T110 retain their open status in tasks.md.

## 2026-09-18 steering correction

Removed the inverted yaw sign in ship movement: Right now adds yaw and Left subtracts it,
matching clockwise/counterclockwise rotation in the arena view. The rendered input probe
now presses and checks both directions separately, rather than checking rotation magnitude
alone. Development Editor build and the full rendered two-round-trip acceptance passed
(`PINBALL_ASTEROID_PASS`), including both directional checks, thrust, firing and same-ball return.

## 2026-09-18 aiming correction

Corrected `BP_AsteroidShip`'s visual rotation from roll=-90 to pitch=-90. Python's positional
`Rotator` argument order had overridden the correct native default, pointing the cone nose
sideways relative to thrust and projectile velocity. The authoring script now uses named
pitch/yaw/roll arguments, and the saved-asset validator checks the nose alignment.
Fresh-process asset validation and the rendered two-round-trip acceptance both passed.
The new gameplay screenshot was inspected to confirm the nose follows the bullet stream;
the timeout and life-loss runs retained their 57/21 kills and correct bonuses/returns.

Implemented and validated T067–T074 on 2026-09-16. C++ owns local rules and the existing common lifecycle
owns duration, terminal admission, bonus conversion and return. Blueprint defaults supply
input, projectile/obstacle classes, original primitive visuals, emissive materials and an
original synthesized 0.12-second destruction chirp. Every added/modified function has a
declaration/implementation comment or Python docstring.

## Play and configuration

- Default map: `/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet`.
- Independent harness: `/Game/Tests/Maps/L_AsteroidTest`, using `DA_AsteroidTestCabinet` with
  only `DA_MG_AsteroidField`. No Defense, Assault or framework stub definition is selected.
- Start, hold/release Down, enter the circular objective, release keys, then confirm with
  Space/Enter. Left/Right rotate, Up thrust, hold Space fires at a 0.14-second interval.
- Three local lives; two active seconds of protected respawn; 30 active seconds to timeout.
  Ship and rocks use swept planar motion and rebound within the 800 × 430 half-extents.
  Twelve initial rocks plus 0.65-second spawn pacing provide enough targets for the high bonus.
- Each unique destruction contributes 100 informational local points and one
  `ObjectsDestroyed` metric. The central profile converts two objects to 1,000 and twenty
  to 10,000 base bonus. Life exhaustion retains earned performance; pinball lives are untouched.
- The production cabinet remains explicitly in development configuration while Phases 7/8
  supply the other two required games. Final three-game validation was not weakened.

## Executed checks

Environment: UE 5.8.2, Development Editor Win64, MSVC 14.50.35738 and Windows SDK
10.0.22621.0. Windows 11 23H2, Ryzen 7 7800X3D, Radeon RX 7900 XTX, 64 GB RAM.
Rendered acceptance uses D3D12, 1280 × 720, 60 FPS cap, VSync off and project quality defaults.

- Development Editor build succeeded, including Unreal Header Tool and all four gameplay
  classes, the acceptance probe and rules test.
- `PinballBattle.Asteroid+PinballBattle.MiniGame+PinballBattle.Score`: **4 passed, 0 failed,
  0 not run**. Covers unique/stale hits, target availability, low/high conversion, damage
  protection, retained score on life loss, timeout clamping, cleanup/replay, possession ownership
  regression and existing lifecycle/scoring cases. Report: `Saved/Automation/Phase6/index.json`.
- Both asset authoring and fresh-process asset validation completed without errors/warnings.
  Validator checks actual packages, pawn actions, sound/classes, definition, independent
  cabinet, scoring weight, single arena root and harness objective association.
  Log marker: `PINBALL_PHASE6_ASSET_VALIDATION_PASSED` in `Saved/Logs/Phase6Assets.log`.
- A rendered two-round-trip run passed controls, ship boundary rebound, pause clock,
  actual projectile kills, swept rock–ship damage, protected respawn, terminal state, bonus
  accounting and same-ball/session return. The final strengthened run also passed with
  `PINBALL_ASTEROID_PASS` in `Saved/Logs/Phase6Acceptance.log`.
  Timeout: 57 kills, three lives, 30.00 active seconds, 10,000 bonus. Early life exhaustion:
  21 kills, zero local lives, 7.00 active seconds, 10,000 earned bonus.
- `Saved/Screenshots/WindowsEditor/Phase6Playing.png` was visually inspected: arena boundaries,
  ship, shots, objects and the Asteroid Field HUD are readable, including lives/time/local score.

The rendered run caught and fixed a real possession defect: Unreal assigns a pawn's Actor Owner
to its controller. The ship now retains an explicit weak runtime reference and immutable run ID.
The score observer captures its baseline after table gates close, so ordinary entry contact
points cannot be mistaken for a duplicate minigame bonus.

## Reproduce

Build with the command in [quickstart](../quickstart.md), then run
`./Scripts/Run-Phase6Validation.ps1`. The script requires an explicit `PINBALL_ASTEROID_PASS`
marker, a zero process exit and a clean Unreal exit. It stops its own process on a bounded timeout.
Authoring and validation scripts are `Scripts/Editor/create_phase6_assets.py` and
`Scripts/Editor/validate_phase6_assets.py`; execute through Unreal's Python commandlet.

## Limits

The rendered probe uses assisted aim and controlled collision placement; it proves real input,
projectile throughput and integration, not first-time human difficulty or an unassisted score.
Visual feedback and original audio wiring are implemented; no human listening or feel study
is claimed. Packaging, reference-hardware performance, ten-cycle per-game regression, ordinary
table-objective reachability study and five-person usability acceptance remain in their later
planned phases. Engine ToolsetRegistry emits its existing `PythonTestRunner` startup warning
in standalone mode; the game and explicit acceptance checks continue normally.
