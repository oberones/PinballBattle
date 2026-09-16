# Phase 4 restart and Quit validation

Scope: T040–T042. Start/Restart intent goes through the controller to GameMode. It accepts only
ATTRACT/GAME_OVER, assigns a new SessionId and increments Generation before cancelling timers,
resetting actors or notifying score/UI observers. It resets the central ledger/total, ball
entitlement, interaction counts, actuator commands and old hit flashes/chimes. Deferred
replacement callbacks compare both the captured session and generation before spawning.

`Scripts/Run-Phase4Validation.ps1` completes three naturally drained scored games and requests
Restart twice from Game Over. Each new session checks:

- Score = 0, balls = 3, multiplier = 1.
- SessionId changes and Generation advances (1, 2, 3).
- Exactly one fresh APinballBall actor exists.
- Old-session score/drain callbacks are rejected.
- Every replacement after drain has a new BallId; each drain decrements only once.
- Game Over has no ball, ignores launch/late score/drain events, and keeps the final score.
- Start/HUD/Game Over widgets correspond to the authoritative flow state.

The third Game Over invokes the production controller Quit intent. The runner requires the
scenario PASS marker, process exit code 0 and `LogExit: Exiting.`; printing a PASS marker alone
is insufficient. Screenshots include `Saved/Automation/Phase4/GameOver.png`, visually checked
for final score, zero balls, Restart and Quit.

Deterministic `PinballBattle.Flow.RestartScoreLock` passes repeated-reset and stale callback
tests independently of the rendered run. Logs/report: `Saved/Logs/Phase4Session.log`,
`Saved/Automation/Phase4/Session.txt`, `Saved/Automation/Phase4Tests/index.json`.

This milestone has no asynchronous minigame callbacks or runs. Their generation invalidation,
cleanup and world-teardown fault scenarios remain T097–T098 rather than being claimed here.
The acceptance is automated rendered gameplay; the later human study is separate.
