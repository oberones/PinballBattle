# Playable ball return — 2026-09-19

The old Alien cabinet release at local `(0, 450, 16)` with velocity `(0, -220, 0)`
sent the ball down the gap between the flippers. A rendered reproduction let the returned
ball roll naturally and pressed the nearest flipper through Enhanced Input. It passed
below both paddles at local Y=79.64 without any flipper contact. Evidence:
`PINBALL_DEFENSE_FAIL Unplayable return` in `Saved/Logs/ReturnFeedBeforeFix.log`.

The cabinet now releases at `(65, 450, 16)` over the Left Arrow flipper. Its collision-checked
backup is `(-85, 450, 16)` over the Right Arrow flipper. Both use table-local velocity
`(0, -160, 0)`. These settings belong to the cabinet Blueprint and its map instances;
shared physics, scoring, ball accounting and input behavior are unchanged by this tuning.
The same markers also improve the cabinet's existing trap/escape recovery feed.

`Scripts/Editor/cabinet_return.py` owns these content settings. The Phase 3 authoring script
applies them to newly authored tables. `configure_cabinet_return.py` updates the existing
Blueprint and five cabinet/test maps without recreating their actors. A fresh process running
`validate_cabinet_return.py` checked every saved setting, with zero commandlet errors/warnings
and marker `PINBALL_RETURN_ASSETS_VALIDATED` in `Saved/Logs/ReturnAssetsValidate.log`.

## Gameplay validation

`UReturnBallProbe` observes the actual returned ball without repositioning it, setting velocity
or adding an impulse. It releases stale arrows, waits for the approaching ball, presses one
mapped arrow, and requires an actual contact with that flipper followed by upward travel
beyond the paddle at greater than 150 cm/s. A feed that drains, misses or gives less than
0.6 seconds to respond fails. Only after this check do the probes reposition the ball to
isolate the existing independent paddle-motion checks.

Both minigame probes use the ordinary primary release for the first round and obstruct only
the primary marker for the second round, forcing the production collision check to choose
the unchanged backup marker. Their existing score, cleanup and same-ball checks remain active.

The 60 FPS Planetary Defense run passed both feeds: a Left Arrow strike after **1.55 s**
sent the ball upfield at **501.7 cm/s**; the backup Right Arrow strike after **1.55 s**
produced **501.8 cm/s**. Both recorded a physical flipper contact and passed the subsequent
independent paddle-motion checks. Log: `Saved/Logs/Phase7Acceptance60.log`.

Asteroid Field also passed both its timeout/primary and life-exhaustion/backup round trips
at 60 FPS, with a **1.55 s** approach and approximately **502 cm/s** upfield after each
flipper contact. Its subsequent independent paddle checks passed as well.
Log: `Saved/Logs/Phase6Acceptance.log`.

Planetary Defense also passed both feeds and paddle checks at 30 FPS: **1.57 s** to the
strike, then **565.9/565.8 cm/s** upfield for primary/backup. The six natural feeds across
these three runs all preserved the session and ball entitlement. Log:
`Saved/Logs/Phase7Acceptance30.log`. The Development Editor build passed.

The existing 60 FPS table-interaction fixtures passed seven physical scoring strikes,
trap and escape recovery, blocked-primary fallback, both-blocked suspension, unblocking,
capture/launch exemptions and drain identity. Four recoveries preserved the entitlement
without awarding points or consuming a ball. Its backup assertion now compares with the
authored marker instead of the old hard-coded X threshold. This run used fixtures only;
it did not repeat the unrelated natural-play cycles. Marker: `PINBALL_INTERACTIONS_PASS`
in `Saved/Logs/Phase3ReturnFeed60.log`; process exit was zero with a clean engine shutdown.

Environment remains UE 5.8.2, Development Editor Win64, D3D12, 1280 × 720, VSync off,
Ryzen 7 7800X3D and Radeon RX 7900 XTX. These controlled strikes validate a reachable return
and usable reaction time; they are not a human difficulty or feel study.

## Reproduce

Build the Editor target as described in [quickstart](../quickstart.md), then run:

```powershell
./Scripts/Run-Phase7Validation.ps1
./Scripts/Run-Phase7Validation.ps1 -FrameRate 30
./Scripts/Run-Phase6Validation.ps1
./Scripts/Run-Phase3Validation.ps1 -FixturesOnly -Rates 60 -RunLabel ReturnFeed
```

For manual play, complete either minigame, release any held arrows, then use Left Arrow
as the ball reaches the left flipper. An obstructed primary release feeds the right flipper
instead. The ball remains the same entitlement and does not require another plunger launch.
