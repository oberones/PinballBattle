#include "Tests/AlienAssaultProbe.h"
#include "Tests/ReturnBallProbe.h"
#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Minigames/AlienAssault/AssaultShipPawn.h"
#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Minigames/AlienAssault/AssaultProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballScoringComponent.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballPlayerController.h"
#include "EnhancedPlayerInput.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/MinigameTriggerComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "PinballBattle.h"

// Only this development observer ticks while paused; gameplay still uses native pause.
AAlienAssaultProbe::AAlienAssaultProbe()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}
// Ordinary players can open the same map without the probe taking control.
void AAlienAssaultProbe::BeginPlay()
{
    Super::BeginPlay(); bEnabled = FParse::Param(FCommandLine::Get(), TEXT("AssaultProbe")); Started = FPlatformTime::Seconds();
}
// Input injection exercises mappings, fresh-input barriers and the pawn's bindings.
void AAlienAssaultProbe::Key(FKey Input, bool Pressed)
{
    GetWorld()->GetFirstPlayerController()->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
// A pass requires explicit assertions, never just a successful process exit.
void AAlienAssaultProbe::Finish(bool Passed, const FString& Detail)
{
    bEnabled = false;
    UE_LOG(LogPinballBattle, Display, TEXT("PINBALL_ASSAULT_%s %s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    FPlatformMisc::RequestExitWithStatus(false, Passed ? 0 : 1);
}
// Results are observations only; the probe cannot award or advance flow.
void AAlienAssaultProbe::ObserveResult(const FMiniGameResult& Result)
{
    ++ResultsSeen; TerminalResult = Result;
}
// Setup manipulates entry and aim only; movement, projectile hits, results and return use production code.
void AAlienAssaultProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bEnabled) return;
    if (FPlatformTime::Seconds() - Started > 110) { Finish(false, TEXT("Deadline")); return; }
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    auto* State = GetWorld()->GetGameState<APinballGameStateBase>();
    if (!Mode || !State || !Objective || !Mode->GetTable()) return;
    auto* Flow = Mode->FindComponentByClass<UGameFlowComponent>(); auto* Table = Mode->GetTable();
    auto* Runtime = Cast<AAlienAssaultRuntime>(Flow->GetRuntime());
    const auto Phase = Flow->GetTransition().Phase;
    if (Stage == 0)
    {
        if (Flow->GetCurrentState() != EArcadeGameFlowState::ATTRACT) return;
        if (!Mode->RequestNewSession() || !Mode->RequestLaunch(100)) { Finish(false, TEXT("Start/launch")); return; }
        SessionId = State->GetSessionState().SessionId; BallId = Table->GetBallHandle().BallId; Stage = 1;
    }
    if (SessionId != State->GetSessionState().SessionId || BallId != Table->GetBallHandle().BallId || State->GetSessionState().BallsRemaining != 3)
    { Finish(false, TEXT("Session/ball accounting changed")); return; }
    if (Stage == 1)
    {
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation() + FVector(140, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector); Wait += DeltaSeconds;
        if (Wait < 1.2 || !Objective->Trigger->IsArmed()) return;
        Baseline = State->GetScoring()->GetTotalScore();
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
        Stage = 2; Wait = 0; return;
    }
    if (Stage == 2 && Phase == ETransitionPhase::AwaitingConfirmation)
    {
        if (!Runtime) { Finish(false, TEXT("Missing Assault runtime")); return; }
        Runtime->OnMiniGameEnded.AddDynamic(this, &AAlienAssaultProbe::ObserveResult);
        LastKills = 0;
        FrozenBall = Table->GetBall()->GetActorLocation();
        Baseline = State->GetScoring()->GetTotalScore(); // Capture after table gates close, including legitimate entry contacts.
        if (!Flow->ConfirmMiniGame()) { Finish(false, TEXT("Confirm")); return; }
        Stage = 3; return;
    }
    if (Stage == 3 && Runtime)
    {
        if (!FrozenBall.Equals(Table->GetBall()->GetActorLocation(), .01) || Table->GetBall()->GetBody()->IsSimulatingPhysics())
        { Finish(false, TEXT("Table was not suspended")); return; }
        auto* Ship = Cast<AAssaultShipPawn>(Runtime->GetRunPawn());
        if (GetWorld()->IsPaused())
        {
            if (Runtime->GetElapsed() != PausedClock) { Finish(false, TEXT("Pause clock")); return; }
            if (FPlatformTime::Seconds() - PauseStarted > .3) Mode->RequestTogglePause(GetWorld()->GetFirstPlayerController());
            return;
        }
        if (Phase == ETransitionPhase::Playing)
        {
            const double Time = Runtime->GetElapsed();
            if (Runtime->GetDestroyedCount() > LastKills)
            {
                bool bFlash = false, bSound = false;
                for (TActorIterator<AAssaultEnemy> It(GetWorld()); It; ++It)
                    if (It->GetOwner() == Runtime && It->bDestroyed && !It->GetActorEnableCollision() &&
                        It->Visual->GetRelativeScale3D().X > 1) bFlash = true;
                TArray<UAudioComponent*> Audio;
                Runtime->GetComponents<UAudioComponent>(Audio);
                for (auto* Component : Audio) if (Component->IsPlaying()) bSound = true;
                if (!bFlash || !bSound) { Finish(false, TEXT("Missing destruction flash/audio playback")); return; }
                FeedbackSeen += Runtime->GetDestroyedCount() - LastKills;
                LastKills = Runtime->GetDestroyedCount();
            }
            if (Time < .2) return; // Observe a neutral frame after instruction confirmation before fresh input.
            if (Cycle == 0 && Time < .5) { Key(EKeys::Right, true); return; }
            if (!bRightTurnChecked)
            {
                Key(EKeys::Right, false);
                const FVector Local = Runtime->GetActorTransform().InverseTransformPosition(Ship->GetActorLocation());
                FVector2D Center, Position, Top;
                auto* Player = GetWorld()->GetFirstPlayerController();
                Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(FVector(0, -370, 0)), Center);
                Player->ProjectWorldLocationToScreen(Ship->GetActorLocation(), Position);
                Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(FVector(0, 280, 0)), Top);
                if (Position.X < Center.X + 20 || Top.Y >= Center.Y || FMath::Abs(Local.Y + 370) > 1)
                { Finish(false, TEXT("Screen-right/bottom movement and upward fire framing")); return; }
                bRightTurnChecked = true; Ship->Respawn(); return;
            }
            if (Cycle == 0 && Time < .9) { Key(EKeys::Left, true); return; }
            if (!bInputChecked)
            {
                Key(EKeys::Left, false);
                if (Runtime->GetActorTransform().InverseTransformPosition(Ship->GetActorLocation()).X < 50)
                { Finish(false, TEXT("Left movement")); return; }
                bInputChecked = true;
                Ship->SetActorLocation(Runtime->GetActorTransform().TransformPosition(FVector(790, -370, 0)));
                Key(EKeys::Left, true); return;
            }
            if (!bReboundChecked)
            {
                Key(EKeys::Left, false);
                if (Runtime->GetActorTransform().InverseTransformPosition(Ship->GetActorLocation()).X > 776.1)
                { Finish(false, TEXT("Horizontal bound")); return; }
                bReboundChecked = true; Ship->Respawn();
            }
            if (!bPaused && Time > 2)
            {
                bPaused = true; PausedClock = Time; PauseStarted = FPlatformTime::Seconds();
                Mode->RequestTogglePause(GetWorld()->GetFirstPlayerController()); return;
            }
            // Assisted horizontal targeting retains real Enhanced Input fire and swept projectile hits.
            AAssaultEnemy* Target = nullptr;
            for (TActorIterator<AAssaultEnemy> It(GetWorld()); It; ++It)
                if (It->GetOwner() == Runtime && !It->bDestroyed) { Target = *It; break; }
            if (Target && Cycle == 0)
            {
                const auto Safety = [this, Runtime](double X)
                {
                    double Clearance = 1000;
                    for (TActorIterator<AAssaultProjectile> It(GetWorld()); It; ++It)
                    {
                        if (It->GetOwner() != Runtime || !It->bEnemyShot) continue;
                        const FVector Local = Runtime->GetActorTransform().InverseTransformPosition(It->GetActorLocation());
                        const FVector Velocity = Runtime->GetActorTransform().InverseTransformVectorNoScale(It->Velocity);
                        if (Velocity.Y >= 0) continue;
                        const double Arrival = (-370 - Local.Y) / Velocity.Y;
                        if (Arrival >= -.15 && Arrival < 1.)
                            Clearance = FMath::Min(Clearance, FMath::Abs(X - (Local.X + Velocity.X * FMath::Max(0., Arrival))));
                    }
                    return Clearance;
                };
                double AimX = 0, Best = TNumericLimits<double>::Max();
                const double ShipX = Runtime->GetActorTransform().InverseTransformPosition(Ship->GetActorLocation()).X;
                for (TActorIterator<AAssaultEnemy> It(GetWorld()); It; ++It)
                {
                    if (It->GetOwner() != Runtime || It->bDestroyed) continue;
                    const double Age = Time;
                    const double Flight = (It->FormationPosition.Y - Age * 14 + 334) / 1114.;
                    const double X = It->FormationPosition.X + 130 * FMath::Sin((Age + Flight) * 1.5);
                    const double Cost = FMath::Abs(X - ShipX);
                    if (Safety(X) > 110 && Cost < Best) { Best = Cost; AimX = X; }
                }
                if (Best == TNumericLimits<double>::Max())
                {
                    double BestSafety = -1;
                    for (double X = -700; X <= 700; X += 100)
                        if (Safety(X) > BestSafety) { AimX = X; BestSafety = Safety(X); }
                }
                Ship->SetActorLocation(Runtime->GetActorTransform().TransformPosition(FVector(AimX, -370, 0)));
            }
            Key(EKeys::SpaceBar, Cycle == 0);
            if (Cycle == 1 && Time > 3 && !Runtime->IsProtected())
            {
                // Place a real hostile shot on a collision course; the sweep owns damage admission.
                auto* Shot = Cast<AAssaultProjectile>(Runtime->SpawnRunActor(Runtime->ProjectileClass,
                    FTransform(FRotator::ZeroRotator, Ship->GetActorLocation() + FVector(0, 60, 0))));
                if (!Shot) { Finish(false, TEXT("Hostile shot spawn")); return; }
                Shot->RunId = Runtime->GetContext().RunId; Shot->bEnemyShot = true; Shot->Velocity = FVector(0, -600, 0);
                const int32 Before = Runtime->GetLives(); Shot->Tick(.1f);
                if (Runtime->GetLives() != Before - 1) { Finish(false, TEXT("Projectile damage")); return; }
                ++DamageCount;
                if (Runtime->DamageShip(Runtime->GetContext().RunId)) { Finish(false, TEXT("Duplicate protected damage")); return; }
            }
            if (Cycle == 0 && Time > 15 && Time < 15.1) FScreenshotRequest::RequestScreenshot(TEXT("Phase8Playing.png"), true, false);
            return;
        }
        if (Phase == ETransitionPhase::Results)
        {
            Key(EKeys::SpaceBar, false);
            const int32 Kills = Runtime->GetDestroyedCount();
            if (ResultsSeen != Cycle + 1 || TerminalResult.bSuccess != (Cycle == 0) ||
                TerminalResult.EndReason != (Cycle == 0 ? EMiniGameEndReason::TimedOut : EMiniGameEndReason::LivesExhausted) ||
                TerminalResult.Metrics.FindRef(TEXT("EnemiesDestroyed")) != Kills ||
                TerminalResult.Metrics.FindRef(TEXT("SurvivalSeconds")) != Runtime->GetElapsed())
            { Finish(false, FString::Printf(TEXT("Terminal count=%d cycle=%d success=%d reason=%d kills=%d lives=%d time=%.3f"),
                ResultsSeen, Cycle, TerminalResult.bSuccess, int32(TerminalResult.EndReason), Kills, Runtime->GetLives(), Runtime->GetElapsed())); return; }
            if ((Cycle == 0 && (Runtime->GetElapsed() != 30 || Kills < 28 || Runtime->GetLives() <= 0)) ||
                (Cycle == 1 && (DamageCount < 1 || Runtime->GetLives() != 0))) { Finish(false, TEXT("Ending/high-score conditions")); return; }
            if (TerminalResult.RawScore != int64(Kills) * 100 + FMath::FloorToInt(Runtime->GetElapsed()) * 10)
            { Finish(false, TEXT("Terminal local destruction/survival points")); return; }
            const int64 Expected = FMath::Min(10000, Kills * 250 + 100 * FMath::FloorToInt(Runtime->GetElapsed()));
            if (State->GetScoring()->GetTotalScore() != Baseline + Expected)
            { Finish(false, FString::Printf(TEXT("Award mismatch baseline=%lld expected=%lld actual=%lld award=%lld"), Baseline, Expected, State->GetScoring()->GetTotalScore(), Flow->GetTransition().Award.AwardedPoints)); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("ASSAULT_ROUND cycle=%d kills=%d lives=%d duration=%.2f bonus=%lld"), Cycle, Kills, Runtime->GetLives(), Runtime->GetElapsed(), Expected);
            if (Cycle == 1)
            {
                // Exercise a real backup release while leaving its authored feed untouched.
                SavedPrimaryReturn = Table->PrimaryReturn->GetComponentTransform();
                Table->PrimaryReturn->SetWorldLocation(Table->GetActorLocation());
            }
            Stage = 4; return;
        }
    }
    if (Stage == 4 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        ReturnBall = NewObject<UReturnBallProbe>(this); ReturnBall->Begin(Table, GetWorld()->GetFirstPlayerController());
        if (Cycle == 1) Table->PrimaryReturn->SetWorldTransform(SavedPrimaryReturn);
        Stage = 5;
    }
    if (Stage == 5 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        FString Failure;
        const auto Check = ReturnBall->Tick(DeltaSeconds, Failure);
        if (Check == UReturnBallProbe::EResult::Running) return;
        if (Check == UReturnBallProbe::EResult::Failed) { Finish(false, Failure); return; }
        Stage = 6; ReturnControls.Reset();
    }
    if (Stage == 6 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation() + FVector(140, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        FString Failure;
        const auto Check = ReturnControls.Tick(Table, GetWorld()->GetFirstPlayerController(), DeltaSeconds, Failure);
        if (Check == FFlipperReturnCheck::EResult::Running) return;
        if (Check == FFlipperReturnCheck::EResult::Failed) { Finish(false, Failure); return; }
        if (++Cycle == 2) { Finish(true, TEXT("Horizontal input/bounds, pause, player/enemy projectiles, both endings, score and same-ball return")); return; }
        Stage = 1; Wait = 0;
    }
}
