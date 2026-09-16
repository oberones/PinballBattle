# PinballBattle Constitution

## Core Principles

### I. Reusable, Theme-Independent Architecture

The project MUST use Unreal Engine 5 and a hybrid C++ and Blueprint architecture to build a
reusable framework for themed virtual pinball cabinets. Pinball mechanics MUST be independent
of cabinet themes, and cabinet content MUST remain separate from the underlying framework.
Cabinets MAY supply their own geometry, art, sounds, rules, scoring opportunities, and minigames.
Adding a cabinet MUST primarily require assets, configuration, Blueprints, levels, and data;
any required core change MUST represent a reusable capability rather than a theme dependency.

C++ SHOULD own reusable gameplay systems, interfaces, state management, scoring, minigame
lifecycle, data structures, and systems shared between cabinets because these form stable
contracts. Blueprints SHOULD own table construction, cabinet-specific behavior, tuning,
effects, audio hooks, trigger configuration, minigame presentation, and rapid iteration.
Blueprint-only early prototypes MAY validate gameplay, but the core architecture MUST NOT
depend on large monolithic Blueprints. Departures from these ownership defaults MUST be
documented with their iteration benefit and effect on reuse.

Systems MUST prefer composition over deep inheritance. Unreal interfaces, Actor Components,
Data Assets, structs, delegates/events, and subsystems MUST be used where they provide useful
separation, without requiring every mechanism for every feature. Systems MUST NOT depend on
hard-coded Actor names or level object names. Shared state MUST have an explicit owner and
necessary lifetime; unnecessary global state is prohibited. Abstractions MUST serve an MVP
need or the explicitly required cabinet/minigame extension boundaries, not hypothetical reuse.

### II. Enjoyable and Predictable Pinball

The MVP MUST use Unreal's 3D physics system. Physics tuning MUST prioritize enjoyable,
predictable arcade play over mechanical simulation accuracy. The project MUST NOT implement
a custom general-purpose physics engine.

The MVP MUST support a pinball, launcher/plunger, left and right flippers, bumpers, walls and
passive obstacles, scoring targets, lanes/ramps, minigame trigger zones, a drain, ball spawning
and reset, and a three-ball game. Ball accounting MUST charge each lost ball once and end the
game when all three balls have been lost. Launching, flipper response, collisions, drains,
and resets MUST be evaluated through playable scenarios before visual polish is prioritized.

### III. Explicit Game Flow and Safe Session Continuity

Game flow MUST support the conceptual states `BOOT`, `ATTRACT`, `PINBALL_READY`,
`PINBALL_PLAYING`, `MINIGAME_TRANSITION`, `MINIGAME_PLAYING`, `MINIGAME_RESULTS`, `BALL_LOST`,
`GAME_OVER`, and `PAUSED`. Implementations MAY use a state machine or an equivalent explicit
model, but input routing and gameplay behavior MUST respect the active state and legal
transitions. Pause MUST preserve the resumable state and suspend active gameplay progression.

The gameplay loop MUST allow the player to start a game, launch a ball, operate the flippers,
earn points from traditional elements, trigger a minigame, receive its score bonus, and resume
the same pinball session until all balls are lost. Special targets, holes, ramps, or zones MAY
activate configured minigames.

Entering a minigame MUST safely suspend pinball simulation and relevant gameplay timers,
prevent unintended scoring or drains, and preserve the session, score, remaining balls, and
ball state needed for resumption. Transitions MUST prevent overlapping minigames and repeated
trigger activation. On completion or failure, the transition owner MUST clean up temporary
resources, restore the appropriate input and presentation, and return control safely.
Ball resumption MAY restore captured motion or use an explicitly configured safe release;
it MUST NOT silently reset the session or consume a ball.

Input MUST use Unreal Engine Enhanced Input. Default bindings MUST be Left Arrow for the left
flipper, Right Arrow for the right flipper, Down Arrow held/released for the plunger, Space for
contextual minigame action/fire, and Escape for pause. Minigames MAY define their own mapping
contexts. Transitions MUST activate/deactivate contexts appropriately and clear stale held
actions so pinball and minigame controls do not interfere.

### IV. Independent Minigames with a Shared Contract

Each minigame MUST be an independent module implementing a shared lifecycle conceptually
equivalent to `Initialize(MiniGameContext)`, `StartMiniGame()`,
`EndMiniGame(MiniGameResult)`, and `Cleanup()`. Adding a minigame MUST NOT require modifying
existing minigames. Dependencies MUST pass through the shared contract or explicit
configuration rather than knowledge of another minigame's implementation.

The context MUST provide the session/configuration data required for a run. Completion MUST
return a structured result capable of expressing raw score, success/failure, performance
rating, objectives completed, duration, and an optional multiplier. Minigames MUST NOT directly
modify the global pinball score. Result delivery MUST occur at most once per run, and cleanup
MUST safely handle completion, cancellation, and initialization/start failures without leaving
actors, timers, input mappings, or callbacks active.

Minigames MUST be short arcade interludes lasting approximately 15–45 seconds, with a
configured duration limit and an explicit termination path.

### V. Centralized Scoring and Event-Driven Feedback

A central scoring system MUST own the pinball score and convert gameplay events and structured
minigame results into points. Pinball components and minigames MUST emit scoring events rather
than directly modifying score UI. UI, audio, and visual feedback MUST observe scoring changes
through defined events or presentation interfaces.

Scoring MUST support base points, target points, bumper points, minigame bonuses, and score
multipliers. Rules MUST define how raw minigame results become pinball bonuses, including
multiplier application, so a completed run cannot award its bonus twice. Cabinet-specific
values and rules MUST be configurable through the framework's extension boundaries.
The architecture MUST leave room for jackpots, combos, missions, and additional cabinet rules
without implementing them unless required by an approved MVP specification.

## MVP Scope and Product Constraints

The MVP MUST contain one retro alien/science-fiction cabinet and demonstrate traditional 3D
pinball integrated with at least one playable themed arcade minigame. The long-term framework
MUST support multiple themed cabinets; the MVP MUST NOT require producing multiple cabinets.

The first cabinet's visual language MUST draw from flying saucers, alien invasion, stars and
planets, radar screens, rockets, neon/emissive elements, 1950s science-fiction imagery, and
retro arcade graphics. The quality target is a polished prototype, not production-quality art.
The MVP MUST NOT require paid assets. Content MUST use Unreal primitives, simple custom meshes
where necessary, basic/emissive materials, Niagara effects, simple UI, and placeholder or
original audio as appropriate to the prototype.

Minigames MAY draw inspiration from generic asteroid-shooter, missile-defense, and
fixed-shooter mechanics. Content MUST use original equivalents and MUST NOT reproduce
copyrighted artwork, sprites, sounds, music, names, characters, exact level layouts, or
distinctive presentation from existing games.

Desktop Windows MUST be the first target platform. The MVP MUST target stable 60 FPS on
typical modern desktop hardware. Feature validation MUST record the reference hardware,
resolution, quality settings, and representative pinball/minigame scenarios used to assess
this target so performance claims are reproducible.

MVP tradeoffs MUST follow this priority order:

1. Fun pinball physics.
2. Reliable gameplay loop.
3. Seamless pinball/minigame integration.
4. Clean reusable architecture.
5. Satisfying scoring and feedback.
6. Visual polish.

Multiplayer, networking, accounts, monetization, mobile, online leaderboards, procedural
cabinet generation, VR, user-generated cabinets, and Steam integration MUST remain out of
scope unless explicitly added by an approved future specification. Priority order governs
tradeoffs within these requirements; it does not waive architectural or scope constraints.

## Development and Validation

Specifications and plans MUST identify the affected framework, cabinet, and minigame
boundaries, relevant states, scoring behavior, and input ownership before implementation.
New reusable abstractions MUST state their concrete purpose. Prototypes MUST identify any
temporary Blueprint ownership of shared systems and the boundary that preserves later reuse.

Validation MUST match the change. Automated checks MUST cover changed deterministic scoring,
result conversion, and lifecycle/state logic where practical; playable integration checks
MUST cover physics and presentation behavior that automated checks do not establish.
Reviews MUST record the checks performed and unresolved limitations rather than claim
unperformed validation.

Before accepting the MVP, recorded validation MUST demonstrate:

- A complete three-ball game from start through launches, scoring, drains, resets, and game over.
- A trigger-to-minigame-to-results-to-pinball round trip preserving the same session and
  applying the bonus once.
- Pause/resume during pinball and a minigame, correct input context switching, and no stale
  held flipper, plunger, or action input after transitions.
- Safe handling of repeated triggers, repeated completion callbacks, and a minigame that
  cannot start or is cancelled.
- Enjoyable launch/flipper response and predictable collisions during representative play.
- Performance measurement against the 60 FPS target on the documented desktop configuration.
- Cabinet data/presentation separation, the shared minigame contract, and compliant asset use.

Compliance review MUST check that a new cabinet can be configured primarily through content
and that a new minigame can use the common contract without editing existing minigames.
This review MUST NOT require building speculative cabinets or features solely to prove reuse.

## Governance

This constitution governs project specifications, plans, tasks, implementation, and reviews.
MUST requirements are mandatory; SHOULD requirements are defaults whose deviations require a
recorded rationale demonstrating preservation of the principle; MAY requirements are optional.
Conflicting project artifacts MUST be revised or accompanied by an approved constitutional
amendment before conflicting implementation proceeds.

Amendments MUST document the proposed change, reason, affected principles, impact on existing
artifacts or code, and any migration work. The project owner MUST approve amendments.
Each amendment MUST update the version and last-amended date while preserving the initial
ratification date. Semantic versioning MUST use MAJOR for incompatible principle removal or
redefinition, MINOR for new principles/sections or materially expanded guidance, and PATCH for
clarifications, typo fixes, or non-semantic refinements. Initial adoption is version 1.0.0.

Every feature plan and implementation review MUST assess constitutional compliance and record
any unresolved conflict. Future specifications MAY explicitly add the deferred product scope
listed above; they MUST still respect the remaining constitutional requirements. New scope
that changes those requirements also needs an amendment. Dependent templates and commands
read the constitution at runtime and are not modified by the constitution workflow.

**Version**: 1.0.0 | **Ratified**: 2026-09-15 | **Last Amended**: 2026-09-15
