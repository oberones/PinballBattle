#include "Tests/BasicSessionProbe.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballPlayerController.h"
#include "Framework/PinballScoringComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/BumperResponseComponent.h"
#include "Data/PinballTuningData.h"
#include "UI/PinballPresentationWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PinballBattle.h"

void ABasicSessionProbe::ObserveResetSession(const FSessionState& Snapshot)
{
    if (!bObservingReset) return;
    if (Snapshot.FlowState == EArcadeGameFlowState::PINBALL_READY)
    {
        ++ResetReadyNotifications;
        int32 Balls = 0;
        for (TActorIterator<APinballBall> It(GetWorld()); It; ++It) ++Balls;
        bResetObservationFailed |= Balls != 1 || !Snapshot.SessionId.IsValid() || !Snapshot.CurrentBallId.IsValid() ||
            Snapshot.BallsRemaining != 3 || Snapshot.Multiplier != 1 || State->GetScoring()->GetTotalScore() != 0 ||
            State->GetScoring()->GetLatestAward().SessionId != Snapshot.SessionId ||
            Table->GetBallHandle().SessionId != Snapshot.SessionId || Table->GetBallHandle().BallId != Snapshot.CurrentBallId;
    }
    else
    {
        bResetObservationFailed |= Snapshot.FlowState != EArcadeGameFlowState::ATTRACT && Snapshot.FlowState != EArcadeGameFlowState::GAME_OVER;
        bResetObservationFailed |= State->GetScoring()->GetTotalScore() != LockedScore || Table->GetBall() != nullptr;
    }
    // A synchronous observer must not recursively reset even while a failed attempt republishes its menu.
    bResetObservationFailed |= Mode->RequestNewSession();
}

void ABasicSessionProbe::ObserveResetScore(const FScoreAward& Award)
{
    if (!bObservingReset) return;
    ++ResetScoreNotifications;
    const auto Snapshot = State->GetSessionState();
    bResetObservationFailed |= Snapshot.FlowState != EArcadeGameFlowState::PINBALL_READY ||
        Award.SessionId != Snapshot.SessionId || State->GetScoring()->GetTotalScore() != 0 ||
        Snapshot.CurrentBallId != Table->GetBallHandle().BallId || Snapshot.BallsRemaining != 3;
}

bool ABasicSessionProbe::CheckFailedStartAndRetry()
{
    const auto Before = State->GetSessionState();
    LockedScore = State->GetScoring()->GetTotalScore();
    const auto AwardBefore = State->GetScoring()->GetLatestAward();
    const int32 ScoresBefore = ResetScoreNotifications;
    const int32 ReadyBefore = ResetReadyNotifications;
    auto* Blocker = GetWorld()->SpawnActor<AActor>();
    auto* Collision = NewObject<UBoxComponent>(Blocker);
    Blocker->SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(80));
    Collision->SetCollisionObjectType(ECC_WorldStatic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->RegisterComponent();
    Blocker->SetActorLocation(Table->BallSpawn->GetComponentLocation());
    bObservingReset = true;
    Controller->RequestStartIntent();
    const auto Failed = State->GetSessionState();
    bool bValid = Failed.FlowState == Before.FlowState && Failed.SessionId == Before.SessionId &&
        Failed.CurrentBallId == Before.CurrentBallId && Failed.BallsRemaining == Before.BallsRemaining &&
        Failed.Generation == Before.Generation + 1 && !Table->GetBall() && !Mode->CanLaunch() && !Mode->CanPlay() &&
        State->GetScoring()->GetTotalScore() == LockedScore &&
        State->GetScoring()->GetLatestAward().SessionId == AwardBefore.SessionId &&
        ResetScoreNotifications == ScoresBefore && ResetReadyNotifications == ReadyBefore &&
        Controller->GetPresentation()->GetStatusText().Contains(TEXT("Unable to place the ball"));
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Blocker->Destroy();
    if (!bValid) { bObservingReset = false; return false; }
    Controller->RequestStartIntent();
    bObservingReset = false;
    const auto Fresh = State->GetSessionState();
    bValid &= Fresh.FlowState == EArcadeGameFlowState::PINBALL_READY && Fresh.Generation == Before.Generation + 2 &&
        Fresh.SessionId != Before.SessionId && Fresh.SessionId.IsValid() &&
        Fresh.BallsRemaining == 3 && Fresh.Multiplier == 1 && State->GetScoring()->GetTotalScore() == 0 &&
        ResetScoreNotifications == ScoresBefore + 1 && ResetReadyNotifications == ReadyBefore + 1 && !bResetObservationFailed;
    Controller->RequestStartIntent(); // Repeated confirmation after success must not create another session.
    bValid &= State->GetSessionState().Generation == Fresh.Generation;
    int32 Balls = 0;
    for (TActorIterator<APinballBall> It(GetWorld()); It; ++It) ++Balls;
    bValid &= Balls == 1;
    Table->PublishScoringEvent(StaleScore);
    bValid &= !Mode->RequestDrainEvent(StaleDrain) && State->GetScoring()->GetTotalScore() == 0;
    UE_LOG(LogPinballBattle, Display, TEXT("PHASE4 remediation: terminal=%s preservedScore=%lld failedGeneration=%lld retryGeneration=%lld notifications=%d/%d valid=%d"),
        *UEnum::GetValueAsString(Before.FlowState), LockedScore, Failed.Generation, Fresh.Generation,
        ResetReadyNotifications, ResetScoreNotifications, bValid);
    return bValid;
}

void ABasicSessionProbe::StrikeTestBumper()
{
    auto* Ball = Table->GetBall();
    auto* Bumper = Table->Bumpers[0];
    // Keep the fixture clear of physical surfaces; only the explicitly injected ring contact awards.
    Ball->SetActorLocation(Table->GetActorTransform().TransformPosition(FVector(0, 450, 140)), false, nullptr, ETeleportType::TeleportPhysics);
    Ball->GetBody()->SetEnableGravity(false);
    Ball->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Bumper->Response->TickComponent(0, LEVELTICK_All, nullptr); // Observe real geometric separation before the next episode.
    FHitResult Hit;
    Hit.ImpactNormal = Table->GetActorForwardVector();
    Bumper->Surface->OnComponentHit.Broadcast(Bumper->Surface, Ball, Ball->GetBody(), FVector::ZeroVector, Hit);
}

void ABasicSessionProbe::TickRemediation(double Age)
{
    using E = EArcadeGameFlowState;
    const auto Snapshot = State->GetSessionState();
    if (Stage == 0 && Age > 1)
    {
        if (Snapshot.FlowState != E::ATTRACT) { Finish(false, TEXT("Remediation requires the Start menu")); return; }
        State->OnSessionChanged.AddDynamic(this, &ThisClass::ObserveResetSession);
        State->GetScoring()->OnScoreChanged.AddDynamic(this, &ThisClass::ObserveResetScore);
        OriginalBumperCooldown = Table->Tuning->BumperCooldown;
        Table->Tuning->BumperCooldown = 60;
        if (!CheckFailedStartAndRetry() || !Mode->RequestLaunch(65))
        { Finish(false, TEXT("Failed Start/retry did not preserve terminal state and commit coherently")); return; }
        StrikeTestBumper();
        Step(1);
    }
    else if (Stage == 1 && Age > .1)
    {
        if (State->GetScoring()->GetTotalScore() != 50 || Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().Size() < 100)
        { Finish(false, TEXT("First bumper contact did not award and impart an impulse")); return; }
        StrikeTestBumper();
        Step(2);
    }
    else if (Stage == 2 && Age > .1)
    {
        if (State->GetScoring()->GetTotalScore() != 100 || Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().Size() > 1)
        { Finish(false, TEXT("Distinct contact must score while same-session impulse cooldown remains closed")); return; }
        StaleScore = Table->MakeScoringEvent(FGuid::NewGuid(), EScoringCategory::Bumper, 1);
        StaleDrain = {Snapshot.SessionId, Snapshot.CurrentBallId, FGuid::NewGuid(), FGuid::NewGuid(), Table->GetPhysicalEpoch()};
        if (!Mode->RequestDrain(Table->GetBall())) { Finish(false, TEXT("Fixture drain rejected")); return; }
        Step(3);
    }
    else if (Stage == 3 && Age > .05)
    {
        if (Snapshot.FlowState == E::PINBALL_READY)
        {
            if (!Mode->RequestLaunch(65) || !Mode->RequestDrain(Table->GetBall()))
            { Finish(false, TEXT("Fixture entitlement accounting failed")); return; }
        }
        else if (Snapshot.FlowState == E::GAME_OVER)
        {
            if (State->GetScoring()->GetTotalScore() != 100 || !CheckFailedStartAndRetry() || !Mode->RequestLaunch(65))
            { Finish(false, TEXT("Failed Restart/retry changed final score or published a partial session")); return; }
            // Old IDs must remain rejected even after the successful retry has reopened active play.
            Table->PublishScoringEvent(StaleScore);
            if (Mode->RequestDrainEvent(StaleDrain) || State->GetScoring()->GetTotalScore() != 0)
            { Finish(false, TEXT("Old callbacks crossed the restart boundary")); return; }
            StrikeTestBumper();
            Step(4);
        }
    }
    else if (Stage == 4 && Age > .1)
    {
        const bool bPassed = State->GetScoring()->GetTotalScore() == 50 &&
            Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().Size() > 100 && !bResetObservationFailed;
        Table->Tuning->BumperCooldown = OriginalBumperCooldown;
        Finish(bPassed, TEXT("Blocked Start and scored Restart preserve state; retry commits once; observer/reentrant/stale checks; new-session bumper impulse before 60s cooldown"));
    }
}
