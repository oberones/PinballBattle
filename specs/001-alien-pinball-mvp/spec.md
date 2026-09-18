# Feature Specification: Alien Invasion Pinball — First Playable MVP

**Feature Branch**: `main` (no branch-creation hook configured)

**Feature Directory**: `specs/001-alien-pinball-mvp`

**Created**: 2026-09-15

**Status**: Implementation in progress — Phases 1–6 complete; Phase 6 closed on 2026-09-18

**Current delivery closeout**: Phase 6 / US3 (T067–T074) is implemented and validated,
including the nose-alignment and left/right steering corrections accepted during playtesting.
See [Phase 6 evidence](validation/asteroid-field.md) for build, automation, asset and rendered
gameplay results. Phases 7–11 (T075–T110) remain open; this closes the current delivery,
not the full three-minigame MVP or its final acceptance gates.

**Input**: One complete alien-themed pinball cabinet combining a three-ball game with
Asteroid Field, Planetary Defense, and Alien Assault; performance earns bonuses before
returning to the same pinball session.

## User Scenarios & Testing *(mandatory)*

Acceptance scenarios are referenced as `USn-ASm`, where `n` is the user story number and
`m` is its numbered acceptance scenario (for example, `US1-AS1`). Retain these identifiers
when linking validation evidence.

### User Story 1 - Play a Complete Pinball Game (Priority: P1)

As a player, I can start a game, launch a ball, operate both flippers, score on a recognizable
pinball table, and play all three balls to a final score.

**Why this priority**: Enjoyable pinball and a reliable complete game are the foundation of
all minigame integration.

**Independent Test**: Play from the start screen through three drains without activating a
minigame. Verify table interaction, score totals, ball accounting, and game over.

**Acceptance Scenarios**:

1. **Given** a fresh launch, **When** the start screen appears and I select Start,
   **Then** the score is zero, balls remaining is three, and one ball awaits launch.
2. **Given** a ball in the plunger lane, **When** I hold and release Down Arrow,
   **Then** it launches with strength reflecting the hold duration up to a configured maximum.
3. **Given** a launched ball, **When** I press Left or Right Arrow,
   **Then** the corresponding flipper responds and can return the ball toward scoring areas.
4. **Given** default scoring and a multiplier of one, **When** I make one standard target
   hit, one bumper hit, and one lane completion, **Then** the total increases by 650 points,
   with visible and audible feedback for each interaction.
5. **Given** balls remaining is three, **When** three successive balls drain,
   **Then** the count changes to two, one, and zero; replacement balls appear only after the
   first two drains, and the third drain displays GAME OVER and the final score.
6. **Given** normal play anywhere on the table, **When** the ball travels across the playfield,
   **Then** the camera keeps the playable area visible and allows its trajectory to be judged.
7. **Given** test scoring settings with a multiplier of two, **When** the same target, bumper,
   and lane events occur, **Then** they award 1,300 points; restoring multiplier one restores
   the 650-point total without changing the interactions.

---

### User Story 2 - Enter and Return from an Arcade Interlude (Priority: P1)

As a player, I can hit an identifiable special objective, play its minigame, receive a bonus,
and continue the same ball without losing my pinball progress.

**Why this priority**: Safe transitions demonstrate the defining product concept.

**Independent Test**: With one playable minigame available, activate its objective through
normal table play and complete a round trip. Repeat this test for each of the three games.

**Acceptance Scenarios**:

1. **Given** active pinball play, **When** the ball activates a special objective,
   **Then** the matching named minigame is announced, pinball stops progressing, and controls
   switch to that minigame without changing the score or balls remaining.
2. **Given** a minigame in progress, **When** I use its controls,
   **Then** pinball cannot move, drain, or generate table points in the background.
3. **Given** a completed run, **When** the results appear,
   **Then** I see performance and bonus, the bonus is added exactly once, and pinball resumes
   automatically after the results presentation with the same session and ball count.
4. **Given** a returned ball, **When** I resume play,
   **Then** the ball is safely returned to the playfield, pinball controls and camera are
   restored, and no previously held button causes an unintended action.
5. **Given** a previously completed objective, **When** the ball leaves and later re-enters
   its activation area after the return protection interval, **Then** another run can begin.

---

### User Story 3 - Clear the Asteroid Field (Priority: P2)

As a player, I pilot a small spacecraft, rotate, thrust, and shoot drifting objects to earn a
performance-based pinball bonus in a short bounded-arena challenge.

**Why this priority**: This supplies the multidirectional shooter experience; it is required
for the complete MVP but follows the core pinball and transition loop.

**Independent Test**: Activate only the Asteroid Field objective and complete both a timed
run and an early loss of all minigame lives. No other minigame is needed for this test.

**Acceptance Scenarios**:

1. **Given** Asteroid Field has started, **When** I use Left/Right, Up, and Space,
   **Then** the ship rotates, thrusts, and fires respectively while remaining in the arena.
2. **Given** drifting objects, **When** my shots destroy them,
   **Then** the displayed minigame score increases once for each destruction.
3. **Given** an active run, **When** 30 seconds of active play elapse or the last life is lost,
   **Then** play ends once and the result records points, destruction count, lives outcome,
   and active duration for the pinball bonus.

---

### User Story 4 - Defend the Planetary Colonies (Priority: P2)

As a player, I aim interceptors to destroy incoming threats and protect colonies, earning a
bonus that rewards both interceptions and surviving structures.

**Why this priority**: This demonstrates a distinct targeting and defense interaction using
the same pinball integration rules.

**Independent Test**: Activate only Planetary Defense and complete a 30-second round with
surviving colonies and another with none surviving.

**Acceptance Scenarios**:

1. **Given** Planetary Defense has started, **When** I move the mouse and press Space,
   **Then** the targeting cursor moves and an interceptor launches toward its position.
2. **Given** descending threats, **When** an interceptor reaches its destination,
   **Then** a temporary blast zone can destroy threats entering it while it remains active.
3. **Given** an undefended threat reaches a colony, **When** it impacts,
   **Then** that colony is lost and its displayed status updates.
4. **Given** a run reaches 30 seconds, **When** results are calculated,
   **Then** both threats destroyed and surviving colonies affect the bonus; holding one
   factor fixed, improving the other cannot reduce the bonus.
5. **Given** all colonies have been lost before timeout, **When** play continues,
   **Then** I can still intercept threats until the timer expires and receive a result with
   zero surviving colonies.

---

### User Story 5 - Repel the Alien Assault (Priority: P2)

As a player, I move a ship along the bottom of the arena and shoot hostile alien formations
while avoiding their fire to earn destruction and survival rewards.

**Why this priority**: This completes the three required minigames with a fixed-screen shooter.

**Independent Test**: Activate only Alien Assault and finish a timed run and an all-lives-lost
run without using either other minigame.

**Acceptance Scenarios**:

1. **Given** Alien Assault has started, **When** I use Left/Right and Space,
   **Then** my ship moves horizontally within the arena and fires upward.
2. **Given** enemies appear above me, **When** the encounter progresses,
   **Then** they move and fire toward my ship, and destroying them increases my score.
3. **Given** active play, **When** 30 seconds elapse or my last life is lost,
   **Then** the result includes enemies destroyed and survival performance, and pinball
   receives the corresponding bonus once.

---

### User Story 6 - Pause Without Losing Progress (Priority: P1)

As a player, I can pause pinball or a minigame without losing a ball, time, or score.

**Why this priority**: Consistent pause and input behavior protect every playable journey.

**Independent Test**: Pause and resume in pinball, each minigame, transitions, and results;
compare position, counters, and remaining time before and after the pause.

**Acceptance Scenarios**:

1. **Given** active gameplay, **When** I press Escape,
   **Then** a pause presentation appears and movement, gameplay timers, and scoring stop.
2. **Given** a paused game, **When** I resume,
   **Then** the previous mode continues with unchanged progress and correct controls.
3. **Given** a transition or result presentation, **When** I pause,
   **Then** its progression waits until resume and the pending transition completes once.

---

### User Story 7 - Restart or Quit After Game Over (Priority: P1)

As a player, I can read my final score, restart a clean game, or quit the application.

**Why this priority**: The MVP must be replayable without restarting the application manually.

**Independent Test**: Finish a session, restart twice, then choose Quit at a later game over.

**Acceptance Scenarios**:

1. **Given** game over, **When** I wait or press gameplay buttons,
   **Then** gameplay remains stopped and the displayed final score does not change.
2. **Given** game over after earning bonuses, **When** I select Restart,
   **Then** score is zero, balls remaining is three, objectives are available, multiplier is
   one, and only one fresh ball awaits launch with no previous minigame effects remaining.
3. **Given** game over, **When** I select Quit,
   **Then** the application closes normally.

### Edge Cases

- Repeated contact with one scoring surface during a single uninterrupted contact must not
  create repeated awards; a later distinct hit may score again.
- Simultaneous special objectives must start only one run; other activations are discarded,
  not queued. If a drain was already accepted, no minigame starts for that lost ball; if a
  minigame activation was accepted first, subsequent drain events cannot consume that ball.
- Repeated finish signals or a timeout and final-life loss occurring together must produce
  only one result and one bonus. Count gameplay outcomes through the accepted end time only.
- A held flipper, plunger, or fire button during a mode change must require a fresh press
  before affecting the new mode. Mouse position must remain a valid aim position on entry.
- Failure to start a minigame or an interrupted run must show a brief failure notice, award
  no bonus, and restore pinball without losing a ball; a fresh later activation may retry.
- A ball escaping the table or remaining trapped outside an intended capture/launch area
  for 10 seconds must be recovered safely without changing score or consuming a ball.
- Losing a minigame life must never decrement pinball balls remaining. Respawn must avoid
  immediate repeated damage from the same contact.
- Completing a run with no scoring actions may award zero; it must still return a valid
  result and resume pinball. Scores and bonuses cannot become negative.
- Pausing during the last second of a minigame or results must not consume the remaining
  time. Repeated pause/resume must not duplicate a transition or award.
- Restart must clear all previous gameplay, result presentations, and pending awards before
  the next ball becomes playable.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The player MUST be able to launch the desktop game into a start/attract screen
  without errors, select Start, and enter a new zero-score, three-ball session.
- **FR-002**: The cabinet MUST contain one plunger lane, one active physics-driven pinball,
  two controllable flippers, at least three pop bumpers, rebound surfaces, at least four
  standard scoring targets, at least two lanes or ramps, three distinct minigame activation
  objectives, and one drain. It MUST provide a launch route, return paths toward the
  flippers, and reachable scoring areas characteristic of a simplified pinball machine.
- **FR-003**: Holding/releasing Down Arrow MUST charge/release the plunger; Left and Right
  Arrow MUST independently operate the corresponding flippers. Release MUST return each
  flipper to rest. Short and full plunger charges MUST produce distinguishable launch strength.
- **FR-004**: Collisions MUST retain the ball within intended surfaces at supported play
  speeds, allow repeatable rebounds and flipper returns, and recover escaped or trapped
  balls as described in Edge Cases. A recoverable fault MUST NOT cost a ball.
- **FR-005**: The camera MUST show the full playable pinball area with a fixed or subtly
  dynamic perspective. The ball, flippers, targets, and activation objectives MUST remain
  distinguishable; UI and feedback MUST NOT obscure trajectory during active play.
- **FR-006**: Each accepted drain MUST consume exactly one ball. Balls remaining MUST include
  the current ball, starting at three; the first two drains MUST provide a replacement at
  the plunger and the third MUST stop play and enter game over.
- **FR-007**: The cabinet MUST expose separately identifiable Asteroid Field, Planetary
  Defense, and Alien Assault objectives using names plus distinct visual cues. Every
  objective MUST be reachable through ordinary ball/flipper play without a debug action or
  completion of another minigame.
- **FR-008**: Each valid objective activation MUST announce and start only its associated
  minigame. Activation MUST be disabled during transitions, minigames, results, pause, and
  game over. Repeat activation MUST require leaving and re-entering the zone after return
  protection expires; no automatic chain of minigames is permitted.
- **FR-009**: From accepted activation until pinball resumes, the system MUST preserve the
  session, shared score, balls remaining, multiplier, objective progress, and ball return
  information. Pinball motion, table timers, table scoring, and drains MUST be suspended.
- **FR-010**: Each minigame MUST receive a fresh run, end once, produce a structured result,
  and release its gameplay effects on exit. No minigame MAY require another specific minigame
  to be present or changed. An unavailable minigame MUST follow the failure recovery path.
- **FR-011**: Each result MUST identify its run and minigame and support raw score,
  success/failure, performance rating, objectives completed, active duration, and optional
  multiplier. Relevant lives, destruction, and survival outcomes MUST be available for bonus
  calculation. A result MUST NOT itself change the shared pinball score.
  Surviving to timeout MUST count as success for the shooters; retaining at least one colony
  at timeout MUST count as success for defense. A failed performance MAY still earn points
  for completed actions; failure to start or interruption MUST earn no bonus.
- **FR-012**: Each normally ended run MUST show a result summary and bonus earned for three
  seconds of unpaused time, award that bonus once, then automatically restore the pinball
  camera and controls. Failed/interrupted runs MUST return with a notice and zero bonus.
- **FR-013**: Pinball MUST resume with the same ball entitlement through a configured safe
  return location/direction, without a new plunger launch or an immediate forced drain.
  Special objectives MUST remain disabled for one second of active play after return.
- **FR-014**: Controls MUST affect only the current gameplay mode. On-screen instructions
  MUST identify the current controls before each minigame starts; gameplay timers MUST begin
  only when control is available. Mode changes MUST discard stale held actions.
- **FR-015**: Escape MUST pause and resume pinball, minigames, transition presentations, and
  results. Pause MUST freeze movement, scoring, and timers and preserve the previous mode.
- **FR-016**: Asteroid Field MUST provide a small spacecraft in a bounded arena, Left/Right
  rotation, Up thrust, Space fire, drifting destructible objects, and points per destruction.
  Objects and ship MUST stay in the arena through visible boundary rebounds. Splitting large
  objects into smaller ones MAY be included but is not required for MVP completion.
- **FR-017**: Asteroid Field MUST start with three local lives, decrement a life on damaging
  collision, and end after 30 seconds of active play or loss of all local lives. Remaining
  lives, score, and time MUST be visible; destruction performance MUST determine its bonus.
- **FR-018**: Planetary Defense MUST start with three colonies at the bottom of the arena,
  descending hostile projectiles, a mouse-controlled targeting cursor, and Space to launch
  an interceptor. Interceptors MUST create visible temporary blast zones at their aim point,
  destroying each intercepted threat only once. A direct colony hit MUST destroy that colony.
- **FR-019**: Planetary Defense MUST run for 30 seconds of active play, display time, score,
  and colony status, and calculate performance from threats destroyed and colonies surviving.
  All colonies being destroyed MUST NOT end the timer early. Ammunition MUST be unlimited
  for the initial MVP; ammunition scoring and resource management are not required.
- **FR-020**: Alien Assault MUST provide a ship moving horizontally along the arena bottom
  with Left/Right and firing with Space, plus moving enemy formations or waves above that
  fire toward the player. Destroying enemies MUST award local score.
- **FR-021**: Alien Assault MUST start with three local lives and end after 30 seconds of
  active play or loss of all local lives. Score, time, and lives MUST be visible, and both
  enemies destroyed and active survival duration MUST affect bonus performance.
  Enemy projectile or enemy contact MUST remove one life per accepted damaging hit; damage
  during a respawn protection interval MUST NOT remove additional lives.
- **FR-022**: The game MUST maintain one authoritative session score. Valid table events and
  centrally evaluated minigame results MUST update it; presentations MUST show that total.
  Default values MUST be 100 per standard target, 50 per bumper hit, and 500 per completed
  lane/ramp traversal, each applied once per distinct qualifying event.
- **FR-023**: Point values, performance-to-bonus rules, and multipliers MUST be configurable
  without changing gameplay rules. A new session MUST start at multiplier one; base points
  and bonus conversion MUST support configured multipliers with exactly one application.
  The MVP need not include a separate mechanic for earning higher multipliers.
- **FR-024**: Default minigame reward tuning MUST make approximately 1,000–10,000 points
  achievable per productive run at multiplier one; zero-performance or aborted runs MAY earn
  zero. Higher evaluated performance MUST NOT reduce the bonus, and identical results under
  identical settings MUST yield identical nonnegative whole-point bonuses. Tuning MUST define
  measurable low/high performance examples for each game before gameplay acceptance.
- **FR-025**: Every scoring interaction MUST provide both visible and audible feedback.
  Popups, emissive flashes, lights, particles, sounds, and notifications MAY be combined.
  Minigame activation MUST have a named notification and a distinct audiovisual cue that
  cannot be mistaken for a standard scoring hit.
- **FR-026**: The pinball display MUST continuously show current shared score, balls remaining,
  and the identity and availability/completion status of all three minigame objectives.
  Each minigame MUST show its local score and timer plus lives or colony state as applicable.
- **FR-027**: Game over MUST display GAME OVER, the final shared score, Restart, and Quit.
  No gameplay or pending event MAY change the final score after the third accepted drain.
- **FR-028**: Restart MUST create a clean session with zero score, three balls, multiplier
  one, reset objectives, correct pinball controls, and exactly one ball awaiting launch.
  Quit MUST close the application normally.
- **FR-029**: The cabinet MUST present an original retro alien/UFO science-fiction theme
  with readable play elements and prototype-level polish, using no required paid assets.
  Its art, audio, names, characters, layouts, and distinctive presentation MUST NOT reproduce
  protected content from classic arcade titles.
- **FR-030**: Shared pinball behavior MUST remain independent of the Alien Invasion theme.
  Cabinet content and scoring configuration MUST remain replaceable without rewriting the
  shared pinball rules. All three minigames MUST obey the same entry/result/return behavior
  and be independently replaceable without altering the other minigames.

### Key Entities *(include if feature involves data)*

- **Cabinet Definition**: The theme, table layout, scoring settings, and three objective-to-
  minigame associations for this cabinet.
- **Pinball Session**: One game with shared score, balls remaining, multiplier, objective
  progress, and current gameplay/pause status; ends after the third accepted drain.
- **Ball State**: Current ball availability, position/motion needed for safe return, and
  whether it is awaiting launch, active, suspended, recovering, or drained.
- **Scoring Event**: A distinct qualifying interaction, its source, point category, and
  contribution to the session score under current scoring settings.
- **Special Objective**: An identifiable table activation area, associated minigame,
  availability, and whether it has been completed this session.
- **Minigame Run**: A single activation with an identity, starting conditions, local score,
  active time, objectives/lives status, and completion status, belonging to one session.
- **Minigame Result**: The once-only outcome of a run, including the performance information
  defined in FR-011. It contains no pinball bonus and does not change the shared score.
- **Score Award**: The central scoring system's calculated award for an accepted table event
  or minigame result. The minigame results presentation combines the performance result with
  this award to show the actual bonus earned.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Ten consecutive fresh application launches and Start actions succeed without
  errors or a blocked start; each produces score zero and three balls. (FR-001)
- **SC-002**: Five complete three-ball sessions each consume exactly three balls, reach game
  over with the correct final total, and allow a clean restart; Quit closes normally.
  (FR-006, FR-022, FR-027–028)
- **SC-003**: All required table elements are present and reachable. In 20 launches spanning
  short/full charges and at least 100 observed flipper/surface contacts, no ball visibly
  crosses a solid barrier or becomes permanently trapped; forced escape/trap cases recover
  without score or ball loss. (FR-002–005)
- **SC-004**: A practiced player can activate all three minigames through ordinary table play
  during a single three-ball session. Each game also completes ten tested round trips with
  no lost pinball progress, duplicate bonus, stuck input, or unintended drain. (FR-007–014)
- **SC-005**: Each timed run ends at 30 seconds of active play within 0.25 seconds; both
  shooter games also end on last-life loss. Pause does not consume active time. Results remain
  visible for three seconds within 0.25 seconds before automatic return. (FR-012, FR-015–021)
- **SC-006**: A scoring audit reproduces every tested table award and all three minigame
  bonuses exactly. Defined low/high productive performance examples earn approximately
  1,000–10,000 points at multiplier one; duplicate results award nothing additional.
  (FR-011, FR-022–024)
- **SC-007**: Three pause/resume cycles in each playable mode and during transitions/results
  preserve score, ball/life counts, positions, and active time with no unintended input.
  Failed starts, interrupted runs, simultaneous end conditions, and repeated triggers also
  recover as specified. (FR-008–015 and Edge Cases)
- **SC-008**: On a documented representative modern Windows desktop at 1920×1080 and recorded
  quality settings, each mode averages at least 60 FPS during a five-minute observation
  including repeated minigame runs; 99% of active-play frames finish within 20 milliseconds,
  and no gameplay transition freezes the display for more than 250 milliseconds.
- **SC-009**: At least four of five first-time playtesters, using only in-game instructions,
  can launch the ball, identify all three special objectives, and use each minigame's
  controls. At least four rate ball readability and flipper responsiveness 4/5 or higher.
  (FR-003, FR-005, FR-007, FR-014, FR-025–026)
- **SC-010**: Review confirms independent minigames, theme-independent pinball behavior, and
  no required paid or copied protected content, with no unresolved constitutional conflict.
  (FR-010, FR-029–030)

## Assumptions

- This is one local single-player Windows cabinet, governed by
  [constitution v1.0.0](../../.specify/memory/constitution.md). Its technology and architecture
  constraints remain binding and will be addressed in planning, without prescribing design here.
- All three minigames are mandatory MVP deliverables. Story priorities order delivery; they
  do not make P2 stories optional. Each minigame can be tested with the common pinball loop
  without requiring either of the other games.
- The supplied range "1,00010,000" means approximately 1,000–10,000 points. Exact reward
  curves and balance settings are tuning work; FR-024 bounds their observable behavior.
- Thirty seconds is the default active-play duration for each minigame, consistent with the
  constitutional 15–45-second range; introductions, pause, and results do not count toward it.
- Keyboard and mouse are available. Planetary Defense uses mouse aim and Space fire; no
  gamepad, remapping UI, or keyboard-only aiming is required for this MVP.
- Initial tuning defaults are three local lives per shooter, three defense colonies,
  unlimited defense ammunition, one-hit colony destruction, non-wrapping asteroid boundaries,
  three-second results, and one-second return protection. These fill unspecified details and
  may be tuned through a reviewed spec update if their player-visible acceptance changes.
- Objectives can be replayed within one session and have no prerequisite mission or unlock
  chain. Completion indicators record at least one finished run; they do not prevent replay.
- Safe return uses a cabinet-configured playfield release rather than requiring restoration
  of the exact pre-trigger velocity. Pinball entitlement and session progress are preserved.
- The playtest sample and performance thresholds operationalize "fun," "readable," and
  "stable 60 FPS." Planning must identify the reference desktop and quality settings before
  performance acceptance; access to that machine and five first-time testers are dependencies.
- Original or appropriately usable no-cost content is required. Prototype polish is sufficient;
  production art, custom general-purpose physics, multiball, nudge/tilt, extra-ball awards,
  jackpots, combos, missions, and persistent saves/high scores are outside this feature.
- Multiplayer, networking, accounts, monetization, mobile, online leaderboards, procedural
  cabinets, VR, user-generated cabinets, and Steam integration remain out of scope.
