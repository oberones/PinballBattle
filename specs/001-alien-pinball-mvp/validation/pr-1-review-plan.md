# PR #1 review assessment and follow-up plan

Reviewed 2026-09-16 against PR head `977809a3152c9af3cbe87d48b59858e93decb524`, which matches the local checkout.

Source: [feat: implement phase 4 - pause and restart](https://github.com/oberones/PinballBattle/pull/1). The review contains two inline comments and five suppressed comments in its [review summary](https://github.com/oberones/PinballBattle/pull/1#pullrequestreview-5227329724). All seven are assessed below.

This is a plan, not implementation or new runtime acceptance evidence. Assessment used the current code, feature contracts and tasks, plus arithmetic checks. No Unreal build, asset regeneration or gameplay run was performed for this review.

## Dispositions

| # | Comment | Assessment | Planned action |
| --- | --- | --- | --- |
| 1 | Restart partially resets before ball spawning succeeds | Valid; highest priority | Make start/restart failure preserve a coherent terminal state and score, report failure, and permit retry. |
| 2 | Generation is logged rather than asserted | Valid coverage gap | Assert production generations 1, 2, 3 in the normal acceptance run; cover invalidation separately in failure tests. |
| 3 | Return-guide script moves the guide again on rerun | False positive | Keep the current geometry calculation; document the algebra below. |
| 4 | Bumper impulse cooldown survives restart | Valid reset omission; low likelihood with the default 0.1-second cooldown | Explicitly reset bumper actuator cooldown and verify the first new-session hit. |
| 5 | Harness and runner deadlines are too short | Valid validation defect | Derive both deadlines from one explicit scenario budget and process grace periods. |
| 6 | Menu button bindings and keyboard routing are bypassed | Valid coverage gap, not evidence of a broken menu | Exercise actual widget input for Start, Resume, Restart and Quit. |
| 7 | One-second cabinet return protection is unused | Correct observation, but proposed recovery wiring targets the wrong behavior | Clarify Phase 5 ownership; consume this value in special-objective rearming, not ordinary trap recovery. |

## 1. Make start/restart failure consistent

[Inline comment](https://github.com/oberones/PinballBattle/pull/1#discussion_r4029924992).

Evidence: [RequestNewSession](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Framework/PinballGameModeBase.cpp:133) mutates session fields and resets the table before calling score reset and ball spawn. [ResetSession](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Framework/PinballScoringComponent.cpp:19) immediately clears the score and broadcasts. [SpawnReadyBall](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Pinball/PinballTable.cpp:112) can fail, including through collision rejection. [RequestStartIntent](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Framework/PinballPlayerController.cpp:191) ignores the failure. The existing Game Over widget can therefore show score zero and three balls without a playable new session.

Planned changes:

1. Validate required owners, cabinet/profile and legal flow before destructive work. Keep the existing rule that only ATTRACT and GAME_OVER accept this intent.
2. Separate preparation from publication within the existing lifecycle owners. Advance generation and invalidate pending work before cancelling timers or resetting actors, as required by T040 and the data model. Keep gameplay gates closed throughout preparation. Never roll generation backward on failure.
3. Stage the candidate identity, validated score configuration and one ready ball before committing the new session. Preserve the previous terminal session projection and authoritative final score until preparation succeeds. Avoid resetting the ledger or broadcasting zero score before spawning is known to have succeeded.
4. Commit the matching session/ball identities, score zero, three balls, multiplier one and READY state together, with notifications only after observers can read a consistent result. Keep the scoring component as the sole score owner; do not add a second mutable score total.
5. On failure, dispose of candidate resources, keep ATTRACT or GAME_OVER coherent, preserve the completed score, keep old callbacks invalidated, and return an actionable failure result. Have the controller display a concise retry message on the existing menu; a repeated Start/Restart attempts preparation again. Ensure failure feedback works even when flow has not changed, because presentation refresh currently skips an unchanged state.

Acceptance:

- Exercise the real GameMode path with a blocked/failed spawn both from ATTRACT and from a scored GAME_OVER. Use a controlled world fixture or a narrow test seam; do not manually assign the expected post-reset session.
- Failure preserves the terminal flow and final score, leaves no candidate ball, and does not publish a successful fresh session or enable gameplay.
- Removing the failure and retrying creates exactly one ready ball with matching IDs and a clean score/ledger. Generation remains monotonic, including failed attempts.
- Late score/drain/replacement work from the previous generation cannot change score, spawn an extra ball or consume entitlement.
- Observe score/session notifications and verify every published snapshot is internally consistent; repeated input cannot commit twice.

## 2. Assert production generation progression

[Inline comment](https://github.com/oberones/PinballBattle/pull/1#discussion_r4029925044).

Evidence: [BasicSessionProbe](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Tests/BasicSessionProbe.cpp:108) checks fresh score, entitlement, actor count and SessionId but only logs Generation. [PinballRestartTests](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Tests/PinballRestartTests.cpp:20) assigns Generation on a local record and tests scoring, not production restart orchestration.

Add `Snapshot.Generation == Sessions + 1` before incrementing `Sessions` in the ordinary three-session probe, with expected/actual values in its failure message. Retain the scoring unit test for its actual purpose. Add the production failure/retry tests from item 1 rather than claiming the scoring unit test covers GameMode generation advancement.

The natural successful run must assert 1, 2, 3. Keep injected failures in a separate scenario: failed attempts may legitimately consume generations, so that scenario must check monotonic invalidation rather than successful-session count. Update the restart evidence only after these checks pass.

## 3. Keep the return-guide calculation

Evidence: [fix_return_guide.py](D:/Media/Projects/Unreal/PinballBattle/Scripts/Editor/fix_return_guide.py:15).

For center `P`, forward vector `F` and old X scale `s`, the new center is:

`P' = P + F * (50 * s - 125)`

The first run at scale 3.2 shifts the center by `35 * F`. Every subsequent run at scale 2.5 shifts it by `(125 - 125) * F = 0`. The script preserves the flipper-side endpoint and checks the outer bound after the edit. The alleged additional 125-unit movement does not occur.

No geometry or asset change is warranted for this comment. The arithmetic was checked directly; the editor script was not rerun during this review.

## 4. Reset bumper cooldown with the session

Evidence: [OnQualifiedContact](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Pinball/BumperResponseComponent.cpp:18) retains `LastImpulseTime`, while [table reset](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Pinball/PinballTable.cpp:151) only clears bumper feedback. The default cooldown is 0.1 seconds, so normal drain/menu/launch timing will usually mask this; a valid longer tuning value exposes the inherited cooldown.

Add a documented bumper reset method that makes its first qualified contact eligible immediately, regardless of the previous session's hit time. Call it for each valid registered bumper from table session reset, alongside feedback cleanup. Use an explicit unused-cooldown state or a tuning-independent expired value. Do not rerun component initialization merely to clear cooldown, because initialization also changes event producer identity and bindings.

Test a hit, an immediate session reset and a new-session hit before the old cooldown expires, using a deliberately long valid cooldown. Verify a new impulse and one award. Verify ordinary same-session repeat contacts still respect the configured impulse cooldown and independent contact-based scoring.

## 5. Align validation deadlines

Evidence: [probe deadline](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Tests/BasicSessionProbe.cpp:80), [per-ball deadline](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/Tests/BasicSessionProbe.cpp:178) and [runner timeout](D:/Media/Projects/Unreal/PinballBattle/Scripts/Run-Phase4Validation.ps1:17).

Nine balls at the permitted 65 seconds already require 585 seconds. That exceeds both the 420-second probe limit and 480-second process limit before launch/menu overhead.

Define the scenario budget explicitly: `sessions * ballsPerSession * perBallSeconds + setupAndMenuAllowance`. Compute it in the runner and pass the scenario counts, per-ball limit and total budget to the probe as command-line options; retain equivalent safe defaults for direct probe launches. Derive the process timeout from that same total plus explicit engine-startup and shutdown grace. Log the resolved limits. Keep real-time deadlines for detecting hangs; native world time freezes during pause.

The current per-ball timer includes moving pauses. Document whether that remains the intended measurement and budget any pauses outside a launched-ball interval in the overhead allowance. Avoid replacing one unrelated magic timeout with another.

Verify budget arithmetic at the nine-ball boundary and expiry behavior with controlled elapsed-time values or shortened test settings. Preserve nonzero failure exit, PASS-marker verification and clean Quit checks. A full 585-second wait is not necessary to test the calculation.

## 6. Cover actual menu input

Evidence: [widget bindings and routing](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Private/UI/PinballPresentationWidget.cpp:59) exist, but the probe invokes controller Start, Resume and Quit methods directly. Its comment about exercising menu Resume overstates that coverage.

Extend rendered acceptance to activate the visible Start, Resume, Restart and Quit buttons through Slate/UMG pointer input. Also exercise Enter while a menu has focus, and retain mapped Escape coverage. Give controls stable identifiers if the test needs them. The input helper must reach widget hit testing/event routing; it must not call controller intent methods or `PrimaryIntent` directly.

Verify one state change per activation, correct cursor/focus, Resume preserving session/score and ready-versus-playing state, Restart resetting exactly once, and fresh gameplay input after closing a menu. Exercise repeated input as part of restart consistency. Use the final Game Over Quit button to exit the standalone acceptance process and require its normal exit. If Quit is checked from additional screens, use separate processes because each successful check terminates the game.

Record the actual click and keyboard paths in acceptance evidence. Screenshots or widget existence alone do not prove the bindings work.

## 7. Clarify return protection and retain Phase 5 ownership

Evidence: [cabinet field](D:/Media/Projects/Unreal/PinballBattle/Source/PinballBattle/Public/Data/CabinetDefinition.h:28) is validated but unused. However, [transition contract E.7](D:/Media/Projects/Unreal/PinballBattle/specs/001-alien-pinball-mvp/contracts/transitions.md:168) defines one active second of **special-objective** protection plus physical exit/re-entry after minigame return, while ordinary scoring resumes immediately. [T035](D:/Media/Projects/Unreal/PinballBattle/specs/001-alien-pinball-mvp/tasks.md:108) introduces the configuration in Phase 4; [T052](D:/Media/Projects/Unreal/PinballBattle/specs/001-alien-pinball-mvp/tasks.md:145) and [T059](D:/Media/Projects/Unreal/PinballBattle/specs/001-alien-pinball-mvp/tasks.md:152) implement the relevant behavior in Phase 5.

The current 0.15-second `EventProtectionUntil` belongs to trap/escape recovery and gates ordinary table events. Replacing it with the cabinet's one-second value would conflate two different protections. Removing the cabinet field would also undo the explicit T035 configuration contract. MVP validation deliberately fixes the value to one second; changing that acceptance value requires a reviewed spec update.

For this follow-up, add a descriptive property comment/tooltip identifying its minigame-objective purpose and Phase 5 consumer. Clarify T052/T059 to read the cabinet value rather than introducing another hard-coded duration. Keep runtime wiring in Phase 5. Its acceptance must demonstrate one active second AND exit/re-entry, protection frozen by pause, and ordinary score/drain availability immediately after committed return.

## Implementation order and completion checks

1. Implement item 1 and its failure/retry regression tests; add item 2's successful-generation assertion.
2. Implement item 4's bumper reset and focused regression check.
3. Implement item 5's shared validation budget, then item 6's menu-input scenarios before the next full rendered run.
4. Add item 7's scope clarification to the property and existing Phase 5 tasks. Make no item 3 geometry change.
5. Build the Editor target, run the relevant scoring/flow/restart automation including new failure cases, and run the revised three-session rendered acceptance. Confirm natural 3→2→1→0 accounting, generations 1→2→3, pause behavior, two restarts and clean Quit. Run the existing Phase 3 interaction validation because the bumper reset touches shared table behavior.
6. Update basic-session, pause and restart evidence with the new run's actual checks and artifacts. Preserve the distinction between unit tests, rendered input coverage and future Phase 5 work.

Add descriptive comments to every new function, including test helpers. Regenerate assets only if implementation changes their authored properties or interfaces.

## Implementation progress

Source changes for items 1, 2, 4, 5, 6 and the Phase 5 clarification in item 7 are drafted. The return-guide geometry is unchanged. New functions have descriptive comments.

Static checks pass: whitespace validation, PowerShell parsing, and the runner's default, four-session and remediation budget calculations. The default rendered scenario allows 645 seconds plus 120 seconds of process startup/shutdown grace. Overhead scales at 20 seconds per session.

Build and runtime validation remain pending while the PinballBattle editor is open. No new gameplay acceptance result is claimed. Next checks: Development Editor build, `PinballBattle` automation, `Run-Phase4Validation.ps1 -Remediation`, normal `Run-Phase4Validation.ps1`, and the Phase 3 interaction regression. Update the evidence documents only after those checks succeed.
