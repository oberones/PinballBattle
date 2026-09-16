#include "Tests/PinballTransitionFunctionalTest.h"
#include "Tests/MiniGameTestRuntime.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballPlayerController.h"
#include "Framework/PinballScoringComponent.h"
#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include "Pinball/MinigameTriggerComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/TableSessionComponent.h"
#include "Pinball/PinballFlipper.h"
#include "Pinball/PinballPlunger.h"
#include "InputKeyEventArgs.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PinballBattle.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

// The fixture observes pause; shipping gameplay actors continue to use ordinary paused world ticks.
APinballTransitionFunctionalTest::APinballTransitionFunctionalTest()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics; TimeLimit = 240;
}
// Command-line opt-in keeps an authored test map playable without automatic test interference.
void APinballTransitionFunctionalTest::BeginPlay()
{
    Super::BeginPlay(); bProbe = FParse::Param(FCommandLine::Get(), TEXT("PinballTransitionProbe"));
    if (bProbe) RunTest();
}
// A bounded real-time watchdog detects deadlocks even when the native world is paused.
void APinballTransitionFunctionalTest::StartTest()
{
    Super::StartTest(); StartedAt = FPlatformTime::Seconds(); Stage = 0;
    Cycle = FParse::Param(FCommandLine::Get(), TEXT("PinballTransitionRecoveryOnly")) ? 19 : 0;
    bCompleted = bPassed = false; PausedPhases.Reset();
}
// Preserve evidence in both the functional framework and a stable standalone log marker.
void APinballTransitionFunctionalTest::Complete(bool Success, const FString& Message)
{
    bCompleted = true; bPassed = Success;
    UE_LOG(LogPinballBattle, Display, TEXT("PINBALL_TRANSITION_%s cycles=%d %s"), Success ? TEXT("PASS") : TEXT("FAIL"), Cycle, *Message);
    FinishTest(Success ? EFunctionalTestResult::Succeeded : EFunctionalTestResult::Failed, Message);
    if (bProbe) FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}
// Inject at the controller boundary so Enhanced Input mappings and held-key suppression actually run.
void APinballTransitionFunctionalTest::Key(FKey Input, bool Pressed)
{
    if (auto* Controller = GetWorld()->GetFirstPlayerController())
        Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
            Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

// Observe the published acceptance boundary before the next pre-physics continuation can secure bodies.
void APinballTransitionFunctionalTest::ObservePhase(const FSessionState& State)
{
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    auto* Flow = Mode ? Mode->FindComponentByClass<UGameFlowComponent>() : nullptr;
    if (Cycle == 0 && Flow && State.FlowState == EArcadeGameFlowState::MINIGAME_TRANSITION &&
        Flow->GetTransition().Phase == ETransitionPhase::Securing && !PausedPhases.Contains(ETransitionPhase::Securing))
    {
        PausedPhases.Add(ETransitionPhase::Securing); SavedClock = 0; PauseAt = FPlatformTime::Seconds();
        Mode->RequestTogglePause(GetWorld()->GetFirstPlayerController());
    }
}

// The fixture manipulates entry placement and local terminal events, never substitutes transition logic.
void APinballTransitionFunctionalTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsRunning() || bCompleted) return;
    if (FPlatformTime::Seconds() - StartedAt > 220) { Complete(false, TEXT("Acceptance deadline exceeded")); return; }
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    auto* State = GetWorld()->GetGameState<APinballGameStateBase>();
    auto* Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController());
    if (!Mode || !State || !Controller || !Objective) return;
    auto* Table = Mode->GetTable(); auto* Flow = Mode->FindComponentByClass<UGameFlowComponent>();
    if (!Table || !Flow) return;
    const auto Public = Flow->GetCurrentState(); const auto Phase = Flow->GetTransition().Phase;
    if (Public == EArcadeGameFlowState::BOOT) return;
    if (Stage == 0)
    {
        if (Public != EArcadeGameFlowState::ATTRACT || !Mode->RequestNewSession() || !Mode->RequestLaunch(100))
        { Complete(false, TEXT("Could not start/launch fixture")); return; }
        SessionId = State->GetSessionState().SessionId; BallId = Table->GetBallHandle().BallId;
        Balls = State->GetSessionState().BallsRemaining; Stage = 1;
        PrimaryReturn = Table->PrimaryReturn->GetComponentTransform(); BackupReturn = Table->BackupReturn->GetComponentTransform();
        StalledParticipant = NewObject<UTransitionTestParticipant>(this); StalledParticipant->RegisterComponent();
        Table->GetSession()->RegisterParticipant(StalledParticipant);
        GetWorldTimerManager().SetTimer(ActiveTableTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ++TimerCallbacks; }), .2f, true);
        GetWorldTimerManager().SetTimer(PausedTableTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Complete(false, TEXT("Originally paused timer resumed")); }), .2f, true);
        GetWorldTimerManager().PauseTimer(PausedTableTimer);
        Table->GetSession()->RegisterTimer(ActiveTableTimer); Table->GetSession()->RegisterTimer(PausedTableTimer);
        State->OnSessionChanged.AddUniqueDynamic(this, &ThisClass::ObservePhase);
    }
    if (State->GetSessionState().SessionId != SessionId || Table->GetBallHandle().BallId != BallId ||
        State->GetSessionState().BallsRemaining != Balls)
    { Complete(false, TEXT("Session/ball entitlement changed")); return; }
    if (Public == EArcadeGameFlowState::PAUSED)
    {
        if (Flow->GetTransition().PhaseSeconds != SavedClock) { Complete(false, TEXT("Paused phase clock advanced")); return; }
        if (Cycle == 9 && Flow->GetRuntime() && Flow->GetRuntime()->GetElapsed() != SavedRunClock)
        { Complete(false, TEXT("Near-timeout pause advanced gameplay clock")); return; }
        if (FPlatformTime::Seconds() - PauseAt > .1) Mode->RequestTogglePause(Controller);
        return;
    }
    if (Cycle == 0 && Phase != ETransitionPhase::None && !PausedPhases.Contains(Phase))
    {
        PausedPhases.Add(Phase); SavedClock = Flow->GetTransition().PhaseSeconds; PauseAt = FPlatformTime::Seconds();
        if (!Mode->RequestTogglePause(Controller)) { Complete(false, TEXT("Phase pause rejected")); return; }
        return;
    }
    if (Stage == 1 && Public == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        if (Cycle == 1 && EntryWait < 1.3)
        {
            // Enter during protection, then remain inside beyond expiry: expiry alone must not re-trigger.
            EntryWait += DeltaSeconds;
            Table->GetBall()->SetActorLocation(Objective->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
            Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
            return;
        }
        // Move outside first so each new cycle includes a physical departure and entry.
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation() + FVector(140, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector); Stage = 2; EntryWait = 0; return;
    }
    if (Stage == 2 && Public == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        // Wait out objective-only protection while holding the fixture in a safe authored area.
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation() + FVector(140, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        if (!Objective->Trigger->IsArmed()) return;
        if (FixtureRoot && !bInjected && Cycle >= 10)
        {
            if (Cycle == 10) FixtureRoot->bFailStart = true;
            if (Cycle == 11) FixtureRoot->bFailInitialize = true;
            if (Cycle == 12) { SavedCamera = FixtureRoot->Camera; FixtureRoot->Camera = nullptr; }
            if (Cycle == 13) GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->UnregisterRoot(FixtureRoot);
            if (Cycle == 15 || Cycle == 16)
            {
                Table->PrimaryReturn->SetWorldLocation(Table->GetActorLocation());
                if (Cycle == 16) Table->BackupReturn->SetWorldLocation(Table->GetActorLocation());
            }
            if (Cycle == 18 || Cycle == 19) StalledParticipant->bRejectRestore = true;
            bInjected = true;
        }
        EntryWait += DeltaSeconds;
        if (EntryWait < 1.1) return;
        if (Cycle == 0 && InputStep == 0)
            for (FKey Held : {EKeys::Left, EKeys::Right, EKeys::Down, EKeys::SpaceBar}) Key(Held, true);
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
        return;
    }
    if (Stage == 2 && Public == EArcadeGameFlowState::MINIGAME_TRANSITION)
    {
        BaselineScore = State->GetScoring()->GetTotalScore(); Stage = 3; bFrozen = false;
        if (Mode->RequestDrain(Table->GetBall()) || Flow->RequestObjective(Objective->Trigger, SessionId, BallId, FGuid::NewGuid()))
        { Complete(false, TEXT("Competing trigger/drain accepted")); return; }
        return;
    }
    if (Stage == 3 && (Phase == ETransitionPhase::AwaitingConfirmation || Phase == ETransitionPhase::Playing || Phase == ETransitionPhase::Results))
    {
        if (!bFrozen) { FrozenPosition = Table->GetBall()->GetActorLocation(); bFrozen = true; }
        if (IsValid(Table->GetBall()) && (!FrozenPosition.Equals(Table->GetBall()->GetActorLocation(), .01) || Table->GetBall()->GetBody()->IsSimulatingPhysics() || Mode->CanPlay()))
        { Complete(false, TEXT("Suspended table moved or accepted gameplay")); return; }
        for (const auto& B : Flow->GetTransition().Table.Bodies) if (B.Body.IsValid() &&
            (!B.Transform.Equals(B.Body->GetComponentTransform(), .01) || B.Body->IsSimulatingPhysics()))
        { Complete(false, TEXT("Registered body moved during round")); return; }
        for (const auto& T : Flow->GetTransition().Table.Timers)
            if (!GetWorldTimerManager().IsTimerPaused(T.Handle) || !FMath::IsNearlyEqual(GetWorldTimerManager().GetTimerRemaining(T.Handle), T.Remaining, .002f))
            { Complete(false, TEXT("Registered table timer advanced")); return; }
        if (bProbe && Phase == ETransitionPhase::Results && !bResultsShot)
        { FScreenshotRequest::RequestScreenshot(TEXT("Phase5Results.png"), true, false); bResultsShot = true; }
    }
    if (Stage == 3 && Phase == ETransitionPhase::AwaitingConfirmation)
    {
        FixtureRoot = Cast<AMiniGameTestRuntime>(Flow->GetRuntime());
        if (Cycle == 19) StalledParticipant->bRejectRestore = true;
        if (Cycle == 0)
        {
            if (bProbe && !bInstructionShot) { FScreenshotRequest::RequestScreenshot(TEXT("Phase5Instructions.png"), true, false); bInstructionShot = true; }
            if (InputStep == 0 && Flow->GetTransition().PhaseSeconds > .2)
            {
                for (FKey Held : {EKeys::Left, EKeys::Right, EKeys::Down, EKeys::SpaceBar}) Key(Held, false);
                InputStep = 1;
            }
            else if (InputStep == 1 && Flow->GetTransition().PhaseSeconds > .4) { Key(EKeys::SpaceBar, true); InputStep = 2; }
            return;
        }
        if (Cycle == 14 && FixtureRoot) FixtureRoot->GetRunPawn()->Destroy();
        const bool Confirmed = Flow->ConfirmMiniGame();
        if (!Confirmed && Cycle != 10 && Cycle != 14) { Complete(false, TEXT("Ready confirmation failed")); return; }
        return;
    }
    if (Stage == 3 && Phase == ETransitionPhase::Playing)
    {
        auto* Runtime = Cast<AMiniGameTestRuntime>(Flow->GetRuntime());
        if (!Runtime) { Complete(false, TEXT("Missing fixture root")); return; }
        if (Cycle == 9)
        {
            if (!bTimeoutActions) { Runtime->SubmitAction(); Runtime->SubmitAction(); bTimeoutActions = true; }
            if (!bNearTimeoutPause && Runtime->GetElapsed() > 29.7)
            {
                bNearTimeoutPause = true; SavedRunClock = Runtime->GetElapsed(); SavedClock = Flow->GetTransition().PhaseSeconds;
                PauseAt = FPlatformTime::Seconds(); Mode->RequestTogglePause(Controller);
            }
            return; // Exercise the real 30-active-second base timeout, without accelerated ticks.
        }
        if (Cycle == 0)
        {
            if (InputStep == 2)
            {
                if (Runtime->GetActionCount() != 0) { Complete(false, TEXT("Confirmation fired gameplay action")); return; }
                if (Runtime->GetElapsed() > .2) { Key(EKeys::SpaceBar, false); InputStep = 3; }
                return;
            }
            if (InputStep == 3) { if (Runtime->GetElapsed() > .4) { Key(EKeys::SpaceBar, true); InputStep = 4; } return; }
            if (InputStep == 4)
            {
                if (Runtime->GetElapsed() < .6) return;
                if (Runtime->GetActionCount() != 1) { Complete(false, TEXT("Fresh Enhanced Input action did not fire once")); return; }
                Key(EKeys::SpaceBar, false);
                for (FKey Held : {EKeys::Left, EKeys::Right, EKeys::Down}) Key(Held, true);
                InputStep = 5;
            }
            Runtime->SubmitAction();
        }
        else { Runtime->SubmitAction(); Runtime->SubmitAction(); }
        auto R = FMiniGameResult::Failure(Runtime->GetContext(), EMiniGameEndReason::LivesExhausted);
        R.Metrics[TEXT("Actions")] = 2; R.RawScore = 200; R.ObjectivesCompleted = 2; R.DurationSeconds = Runtime->GetElapsed();
        if (!Runtime->EndMiniGame(R) || Runtime->EndMiniGame(R)) { Complete(false, TEXT("Terminal latch failed")); return; }
        if (State->GetScoring()->GetTotalScore() != BaselineScore + 1000) { Complete(false, TEXT("Wrong bonus")); return; }
        if (Cycle == 17) Table->GetBall()->Destroy();
        return;
    }
    if (Stage == 3 && Public == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        if (Cycle == 0)
        {
            if (Table->LeftFlipper->IsHeld() || Table->RightFlipper->IsHeld() || Table->Plunger->IsCharging())
            { Complete(false, TEXT("Held input leaked through return")); return; }
            for (FKey Held : {EKeys::Left, EKeys::Right, EKeys::Down}) Key(Held, false);
        }
        if (GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->GetActiveRun().Context.RunId.IsValid())
        { Complete(false, TEXT("Active run leaked after return")); return; }
        for (TActorIterator<AMiniGameTestPawn> It(GetWorld()); It; ++It)
            if (IsValid(*It)) { Complete(false, TEXT("Run pawn leaked after cleanup")); return; }
        if (!GetWorldTimerManager().IsTimerActive(ActiveTableTimer) || !GetWorldTimerManager().IsTimerPaused(PausedTableTimer))
        { Complete(false, TEXT("Incorrect timer state after return")); return; }
        const bool FailedRound = Cycle >= 10 && Cycle <= 14;
        if (State->GetScoring()->GetTotalScore() != BaselineScore + (FailedRound ? 0 : 1000))
        { Complete(false, TEXT("Failure/return changed finalized bonus")); return; }
        if (FixtureRoot)
        {
            FixtureRoot->bFailStart = FixtureRoot->bFailInitialize = false;
            if (Cycle == 12) FixtureRoot->Camera = SavedCamera;
            if (Cycle == 13) GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->RegisterRoot(FixtureRoot);
        }
        Table->PrimaryReturn->SetWorldTransform(PrimaryReturn); Table->BackupReturn->SetWorldTransform(BackupReturn);
        ++Cycle; Stage = 1; bInjected = false; EntryWait = 0;
    }
    if (Phase == ETransitionPhase::RecoveryMenu)
    {
        if (Cycle != 16 && Cycle != 18 && Cycle != 19) { Complete(false, TEXT("Unexpected secured recovery menu")); return; }
        if (Mode->CanPlay() || State->GetScoring()->GetTotalScore() != BaselineScore + 1000)
        { Complete(false, TEXT("Blocked release leaked play or lost award")); return; }
        if (Cycle == 19)
        {
            const auto OldContext = Flow->GetTransition().Context;
            StalledParticipant->bRejectRestore = false;
            if (!Mode->RequestNewSession() || State->GetSessionState().SessionId == SessionId ||
                State->GetSessionState().BallsRemaining != 3 || State->GetScoring()->GetTotalScore() != 0)
            { Complete(false, TEXT("Explicit recovery restart failed")); return; }
            auto Stale = FMiniGameResult::Failure(OldContext, EMiniGameEndReason::TimedOut);
            Stale.Metrics[TEXT("Actions")] = 100;
            if (FixtureRoot->EndMiniGame(Stale)) { Complete(false, TEXT("Stale terminal accepted after restart")); return; }
            FixtureRoot->OnMiniGameEnded.Broadcast(Stale);
            if (State->GetScoring()->GetTotalScore() != 0 || !Mode->RequestLaunch(100) || !Mode->RequestDrain(Table->GetBall()) ||
                Flow->RequestObjective(Objective->Trigger, SessionId, BallId, FGuid::NewGuid()))
            { Complete(false, TEXT("Stale callback or drain-first race failed")); return; }
            ++Cycle;
            Complete(true, TEXT("20 scenarios: round trips, 30s timeout, phase/near-timeout pause, held Enhanced Input, frozen bodies/timers, failure recovery, retry, restart and stale callbacks"));
            return;
        }
        Table->PrimaryReturn->SetWorldTransform(PrimaryReturn); Table->BackupReturn->SetWorldTransform(BackupReturn);
        StalledParticipant->bRejectRestore = false;
        Flow->RetryReturn();
    }
}
