# Phase 4 pause validation

Scope: T037–T039, pinball only. Cross-minigame and transaction pause coverage remains scheduled
in Phases 5 and 9.

The GameMode coordinates native Unreal pause with the flow component's single saved state.
Gameplay input mappings are removed; common Escape remains available. Table, score and drain
gates reject events while paused. Resume restores the saved phase and re-adds the pinball
mapping with held-key suppression. Widgets display state and send intents; they own no clocks.

`Scripts/Run-Phase4Validation.ps1` performs three moving-ball pause/resume cycles via actual
Escape press/release events. Each compares snapshots across 0.6 wall-clock seconds:

- Ball and both constrained flipper transforms unchanged within 0.001 tolerance.
- Ball velocity unchanged within 0.001 tolerance.
- Central score and Unreal world time unchanged exactly.
- Stale score/drain and launch attempts rejected.
- Resume returns to PINBALL_PLAYING, old held flipper/plunger commands stay cancelled, and
  fresh left/right presses operate both flippers.

An additional ready-state scenario pauses a charged plunger, releases Down while paused,
uses the Resume menu intent, and verifies PINBALL_READY with no accidental launch. A later
fresh hold/release successfully launches the same ball.

The initial runtime check caught Escape immediately retriggering after resume when a global
`RebuildWithFlush` reset the still-held common action. The fix rebuilds the changed gameplay
context while retaining the common action's press state. The rerun passes all three moving
cycles. Deterministic `PinballBattle.Flow.EdgesAndPause` also passes nested pause, repeated
resume, forbidden transitions and exact saved-state restoration checks.

Evidence: `Saved/Logs/Phase4Session.log` (`PHASE4 pause=1/2/3` and ready-pause markers),
`Saved/Automation/Phase4/Pause.png`, and `Saved/Automation/Phase4Tests/index.json`.
These are automated rendered gameplay checks; no human playtest is claimed.
