#include "Tests/BasicSessionProbe.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballPlayerController.h"
#include "Framework/PinballScoringComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballFlipper.h"
#include "Pinball/PinballPlunger.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UI/PinballPresentationWidget.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "PinballBattle.h"

ABasicSessionProbe::ABasicSessionProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ABasicSessionProbe::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    SetActorTickEnabled(false);
#else
    SetActorTickEnabled(FParse::Param(FCommandLine::Get(), TEXT("PinballSessionProbe")));
#endif
    StartedAt = StepAt = FPlatformTime::Seconds();
}

void ABasicSessionProbe::Key(FKey Input, bool bPressed)
{
    Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void ABasicSessionProbe::Step(int32 Next)
{
    Stage = Next;
    StepAt = FPlatformTime::Seconds();
}

void ABasicSessionProbe::Screenshot(const FString& Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Automation/Phase4") / (Name + TEXT(".png")), true, false);
}

void ABasicSessionProbe::Finish(bool bSuccess, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true;
    const FString Summary = FString::Printf(TEXT("PINBALL_SESSION_%s: %s; sessions=%d launches=%d drains=%d movingPauses=%d restarts=%d score=%lld"),
        bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Reason, Sessions, Launches, Drains, Pauses, FMath::Max(0, Sessions - 1),
        State ? State->GetScoring()->GetTotalScore() : 0);
    UE_LOG(LogPinballBattle, Display, TEXT("%s"), *Summary);
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Automation/Phase4")), true);
    FFileHelper::SaveStringToFile(Summary, *(FPaths::ProjectSavedDir() / TEXT("Automation/Phase4/Session.txt")));
    if (bSuccess) Controller->RequestQuitIntent();
    else FPlatformMisc::RequestExitWithStatus(false, 1);
}

void ABasicSessionProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    const double Now = FPlatformTime::Seconds();
    const double Age = Now - StepAt;
    if (Now - StartedAt > 420) { Finish(false, TEXT("Session harness deadline")); return; }
    if (!Mode)
    {
        Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
        Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController());
        State = GetWorld()->GetGameState<APinballGameStateBase>();
        Table = Mode ? Mode->GetTable() : nullptr;
        if (!Mode || !Controller || !State || !Table) return;
    }
    const FSessionState Snapshot = State->GetSessionState();
    using E = EArcadeGameFlowState;
    if (Stage == 0 && Age > 1)
    {
        if (Snapshot.FlowState != E::ATTRACT || Table->GetBall() || !Controller->GetPresentation() ||
            Controller->GetPresentation()->Screen != EPinballScreen::Start)
        { Finish(false, TEXT("Start screen/bootstrap mismatch")); return; }
        Screenshot(TEXT("Start"));
        Step(1);
    }
    else if (Stage == 1 && Age > .4)
    {
        Controller->RequestStartIntent();
        Step(2);
    }
    else if (Stage == 2 && Age > .25)
    {
        int32 Balls = 0;
        for (TActorIterator<APinballBall> It(GetWorld()); It; ++It) ++Balls;
        if (Snapshot.FlowState != E::PINBALL_READY || Snapshot.BallsRemaining != 3 || Snapshot.Multiplier != 1 ||
            State->GetScoring()->GetTotalScore() != 0 || Balls != 1 || Snapshot.SessionId == PreviousSession || !Snapshot.SessionId.IsValid())
        { Finish(false, TEXT("Fresh session did not reset to score=0 balls=3 multiplier=1 and one new ball")); return; }
        PreviousSession = Snapshot.SessionId;
        ++Sessions;
        UE_LOG(LogPinballBattle, Display, TEXT("PHASE4 fresh session=%d generation=%lld score=0 balls=3 multiplier=1 actors=1"), Sessions, Snapshot.Generation);
        Table->PublishScoringEvent(StaleScore);
        if (Mode->RequestDrainEvent(StaleDrain) || State->GetScoring()->GetTotalScore() != 0)
        { Finish(false, TEXT("Prior-session callbacks accepted")); return; }
        Screenshot(TEXT("Ready"));
        Step(Sessions == 1 ? 13 : 3);
    }
    else if (Stage == 3 && Age > .25)
    {
        if (!Mode->CanLaunch()) { Finish(false, TEXT("Replacement not ready")); return; }
        PreviousBall = Snapshot.CurrentBallId;
        Key(EKeys::Down, true);
        Step(4);
    }
    else if (Stage == 4 && Age > .8)
    {
        Key(EKeys::Down, false);
        Step(5);
    }
    else if (Stage == 5 && Age > .15)
    {
        if (!Mode->CanPlay()) { Finish(false, TEXT("Mapped plunger release failed")); return; }
        ++Launches;
        PlayingAt = Now;
        StaleScore = Table->MakeScoringEvent(FGuid::NewGuid(), EScoringCategory::Target, 1);
        StaleDrain.SessionId = Snapshot.SessionId;
        StaleDrain.BallId = Snapshot.CurrentBallId;
        StaleDrain.EventId = FGuid::NewGuid();
        StaleDrain.SourceId = FGuid::NewGuid();
        StaleDrain.PhaseEpoch = Table->GetPhysicalEpoch();
        Step(6);
    }
    else if (Stage == 6)
    {
        if (Snapshot.FlowState == E::PINBALL_READY || Snapshot.FlowState == E::GAME_OVER)
        {
            ++Drains;
            const int32 ExpectedBalls = 3 - ((Drains - 1) % 3 + 1);
            if (Snapshot.BallsRemaining != ExpectedBalls || Mode->RequestDrainEvent(StaleDrain))
            { Finish(false, TEXT("Drain accounting or duplicate rejection failed")); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("PHASE4 drain=%d balls=%d score=%lld"), Drains, Snapshot.BallsRemaining, State->GetScoring()->GetTotalScore());
            if (Snapshot.FlowState == E::GAME_OVER)
            {
                LockedScore = State->GetScoring()->GetTotalScore();
                if (LockedScore <= 0 || Table->GetBall() || !Controller->GetPresentation() ||
                    Controller->GetPresentation()->Screen != EPinballScreen::GameOver)
                { Finish(false, TEXT("Final score/ball/menu mismatch")); return; }
                Screenshot(TEXT("GameOver"));
                Step(10);
            }
            else
            {
                if (Snapshot.CurrentBallId == PreviousBall) { Finish(false, TEXT("Replacement reused entitlement")); return; }
                Step(3);
            }
            return;
        }
        if (Sessions == 1 && Pauses < 3 && Age > .35 && Table->GetBall() &&
            Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().Size() > 10)
        {
            Key(EKeys::Left, true);
            Key(EKeys::Down, true);
            Key(EKeys::Escape, true);
            Step(7);
        }
        else if (Now - PlayingAt > 65) { Finish(false, TEXT("Natural ball did not drain within 65 seconds")); }
    }
    else if (Stage == 7 && Age > .15)
    {
        Key(EKeys::Escape, false);
        if (Snapshot.FlowState != E::PAUSED || !GetWorld()->IsPaused() || !Table->GetBall())
        { Finish(false, TEXT("Mapped Escape failed to pause native world")); return; }
        PausedBall = Table->GetBall()->GetActorTransform();
        PausedLeft = Table->LeftFlipper->GetBody()->GetComponentTransform();
        PausedRight = Table->RightFlipper->GetBody()->GetComponentTransform();
        PausedVelocity = Table->GetBall()->GetBody()->GetPhysicsLinearVelocity();
        PausedScore = State->GetScoring()->GetTotalScore();
        PausedWorldTime = GetWorld()->GetTimeSeconds();
        Screenshot(TEXT("Pause"));
        Step(8);
    }
    else if (Stage == 8 && Age > .6)
    {
        Table->PublishScoringEvent(StaleScore);
        if (!PausedBall.Equals(Table->GetBall()->GetActorTransform(), .001) ||
            !PausedLeft.Equals(Table->LeftFlipper->GetBody()->GetComponentTransform(), .001) ||
            !PausedRight.Equals(Table->RightFlipper->GetBody()->GetComponentTransform(), .001) ||
            !PausedVelocity.Equals(Table->GetBall()->GetBody()->GetPhysicsLinearVelocity(), .001) ||
            State->GetScoring()->GetTotalScore() != PausedScore || GetWorld()->GetTimeSeconds() != PausedWorldTime ||
            Mode->RequestDrainEvent(StaleDrain) || Mode->RequestLaunch(100))
        { Finish(false, TEXT("Pause changed bodies, velocity, score, clock or accepted gameplay")); return; }
        ++Pauses;
        UE_LOG(LogPinballBattle, Display, TEXT("PHASE4 pause=%d frozen ball/flippers/velocity/score/world clock; stale events rejected"), Pauses);
        Key(EKeys::Escape, true);
        Step(9);
    }
    else if (Stage == 9 && Age > .15)
    {
        Key(EKeys::Escape, false);
        Key(EKeys::Left, false);
        Key(EKeys::Down, false);
        if (!Mode->CanPlay() || GetWorld()->IsPaused() || Table->Plunger->IsCharging() || Table->LeftFlipper->IsHeld())
        { Finish(false, TEXT("Resume phase or held plunger cancellation failed")); return; }
        Step(11);
    }
    else if (Stage == 10 && Age > .5)
    {
        Table->PublishScoringEvent(StaleScore);
        if (Mode->RequestLaunch(100) || Mode->RequestDrainEvent(StaleDrain) || State->GetScoring()->GetTotalScore() != LockedScore)
        { Finish(false, TEXT("Game-over score/input lock failed")); return; }
        if (Sessions == 3)
        {
            if (Pauses != 3) { Finish(false, TEXT("Missing moving pause coverage")); return; }
            Finish(true, TEXT("Three natural scored sessions; 3->2->1->0 each; frozen native pause; two clean restarts; invoking Quit"));
        }
        else { Controller->RequestStartIntent(); Step(2); }
    }
    else if (Stage == 11 && Age > .1)
    {
        Key(EKeys::Left, true);
        Key(EKeys::Right, true);
        Step(12);
    }
    else if (Stage == 12 && Age > .1)
    {
        if (!Table->LeftFlipper->IsHeld() || !Table->RightFlipper->IsHeld())
        { Finish(false, TEXT("Fresh flipper presses did not work after resume")); return; }
        Key(EKeys::Left, false);
        Key(EKeys::Right, false);
        Screenshot(TEXT("Playing"));
        Step(6);
    }
    else if (Stage == 13 && Age > .25)
    {
        Key(EKeys::Down, true);
        Step(14);
    }
    else if (Stage == 14 && Age > .25)
    {
        if (!Table->Plunger->IsCharging()) { Finish(false, TEXT("Ready-state charge did not start")); return; }
        Key(EKeys::Escape, true);
        Step(15);
    }
    else if (Stage == 15 && Age > .25)
    {
        Key(EKeys::Escape, false);
        Key(EKeys::Down, false);
        if (Snapshot.FlowState != E::PAUSED || Snapshot.ResumeState != E::PINBALL_READY || Table->Plunger->IsCharging())
        { Finish(false, TEXT("Pausing a charged plunger did not cancel it")); return; }
        // Exercise the menu Resume intent as well as the mapped Escape path used below.
        Controller->RequestPauseIntent();
        Step(16);
    }
    else if (Stage == 16 && Age > .25)
    {
        if (!Mode->CanLaunch() || Table->GetBall()->IsLaunched() || Table->Plunger->IsCharging())
        { Finish(false, TEXT("Cancelled ready-state plunger fired on release/resume")); return; }
        UE_LOG(LogPinballBattle, Display, TEXT("PHASE4 ready pause cancels charge without launch; Resume intent preserves ready state"));
        Step(3);
    }
}
