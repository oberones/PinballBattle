#include "Tests/AsteroidFieldProbe.h"
#include "Tests/ReturnBallProbe.h"
#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Minigames/AsteroidField/AsteroidShipPawn.h"
#include "Minigames/AsteroidField/AsteroidObstacle.h"
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
AAsteroidFieldProbe::AAsteroidFieldProbe()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}
// Ordinary players can open the same map without the probe taking control.
void AAsteroidFieldProbe::BeginPlay()
{
    Super::BeginPlay(); bEnabled = FParse::Param(FCommandLine::Get(), TEXT("AsteroidProbe")); Started = FPlatformTime::Seconds();
}
// Input injection exercises mappings, fresh-input barriers and the pawn's bindings.
void AAsteroidFieldProbe::Key(FKey Input, bool Pressed)
{
    GetWorld()->GetFirstPlayerController()->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}
// A pass requires explicit assertions, never just a successful process exit.
void AAsteroidFieldProbe::Finish(bool Passed, const FString& Detail)
{
    bEnabled = false;
    UE_LOG(LogPinballBattle, Display, TEXT("PINBALL_ASTEROID_%s %s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    FPlatformMisc::RequestExitWithStatus(false, Passed ? 0 : 1);
}
// Setup manipulates entry and aim only; movement, projectile hits, results and return use production code.
void AAsteroidFieldProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bEnabled) return;
    if (FPlatformTime::Seconds() - Started > 110) { Finish(false, TEXT("Deadline")); return; }
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    auto* State = GetWorld()->GetGameState<APinballGameStateBase>();
    if (!Mode || !State || !Objective || !Mode->GetTable()) return;
    auto* Flow = Mode->FindComponentByClass<UGameFlowComponent>(); auto* Table = Mode->GetTable();
    auto* Runtime = Cast<AAsteroidFieldRuntime>(Flow->GetRuntime());
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
        FrozenBall = Table->GetBall()->GetActorLocation();
        Baseline = State->GetScoring()->GetTotalScore(); // Capture after table gates close, including legitimate entry contacts.
        if (!Flow->ConfirmMiniGame()) { Finish(false, TEXT("Confirm")); return; }
        Stage = 3; return;
    }
    if (Stage == 3 && Runtime)
    {
        if (!FrozenBall.Equals(Table->GetBall()->GetActorLocation(), .01) || Table->GetBall()->GetBody()->IsSimulatingPhysics())
        { Finish(false, TEXT("Table was not suspended")); return; }
        auto* Ship = Cast<AAsteroidShipPawn>(Runtime->GetRunPawn());
        if (GetWorld()->IsPaused())
        {
            if (Runtime->GetElapsed() != PausedClock) { Finish(false, TEXT("Pause clock")); return; }
            if (FPlatformTime::Seconds() - PauseStarted > .3) Mode->RequestTogglePause(GetWorld()->GetFirstPlayerController());
            return;
        }
        if (Phase == ETransitionPhase::Playing)
        {
            const double Time = Runtime->GetElapsed();
            if (Time < .2) return; // Observe a neutral frame after instruction confirmation before fresh input.
            if (Cycle == 0 && Time < .5) { Key(EKeys::Right, true); Key(EKeys::Up, true); return; }
            // Check each key's signed rotation; a magnitude-only check would miss reversed controls.
            if (!bRightTurnChecked)
            {
                Key(EKeys::Right, false); Key(EKeys::Up, false);
                if (Ship->GetActorRotation().Yaw < 10) { Finish(false, TEXT("Right must increase yaw")); return; }
                bRightTurnChecked = true; Ship->Respawn(); return;
            }
            if (Cycle == 0 && Time < .9) { Key(EKeys::Left, true); Key(EKeys::Up, true); return; }
            if (!bInputChecked)
            {
                Key(EKeys::Left, false); Key(EKeys::Up, false);
                if (Ship->Velocity.Size() < 20 || Ship->GetActorRotation().Yaw > -10)
                {
                    auto* PC = Cast<APinballPlayerController>(Ship->GetController());
                    auto* Input = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
                    Finish(false, FString::Printf(TEXT("Rotate/thrust input speed=%.1f yaw=%.1f ready=%d thrustAsset=%s input=%s value=%d"), Ship->Velocity.Size(), Ship->GetActorRotation().Yaw,
                        PC && PC->IsMiniGameInputReady(), *GetNameSafe(Ship->ThrustAction), *GetNameSafe(Ship->InputComponent), Input && Ship->ThrustAction && Input->GetActionValue(Ship->ThrustAction).Get<bool>())); return;
                }
                bInputChecked = true;
                Ship->SetActorLocation(Runtime->GetActorLocation() + FVector(775, 0, 0)); Ship->Velocity = FVector(420, 0, 0);
                return;
            }
            if (!bReboundChecked)
            {
                if (Ship->Velocity.X >= 0) return;
                bReboundChecked = true; Ship->Respawn();
            }
            if (!bPaused && Time > 2)
            {
                bPaused = true; PausedClock = Time; PauseStarted = FPlatformTime::Seconds();
                Mode->RequestTogglePause(GetWorld()->GetFirstPlayerController()); return;
            }
            // Assisted aim proves real projectile throughput; it is not a claim of human playtest difficulty.
            AAsteroidObstacle* Target = nullptr; double Distance = TNumericLimits<double>::Max();
            for (TActorIterator<AAsteroidObstacle> It(GetWorld()); It; ++It)
                if (It->GetOwner() == Runtime && !It->bDestroyed && FVector::DistSquared(Ship->GetActorLocation(), It->GetActorLocation()) < Distance)
                { Target = *It; Distance = FVector::DistSquared(Ship->GetActorLocation(), It->GetActorLocation()); }
            if (Target) Ship->SetActorRotation((Target->GetActorLocation() - Ship->GetActorLocation()).Rotation());
            Key(EKeys::SpaceBar, true);
            if (Cycle == 1 && Time > 3 && !Runtime->IsProtected())
            {
                if (!Target) { Finish(false, TEXT("No target for damaging contact")); return; }
                const int32 Before = Runtime->GetLives();
                Target->SetActorLocation(Ship->GetActorLocation() + FVector(60, 0, 0));
                Target->Velocity = FVector(-300, 0, 0);
                Runtime->MoveBounded(Target, Target->Velocity, 32, .1f);
                if (Runtime->GetLives() != Before - 1) { Finish(false, TEXT("Swept rock contact did not damage ship")); return; }
                ++DamageCount;
                if (Runtime->DamageShip(Runtime->GetContext().RunId)) { Finish(false, TEXT("Duplicate protected damage")); return; }
            }
            if (Cycle == 0 && Time > 15 && Time < 15.1) FScreenshotRequest::RequestScreenshot(TEXT("Phase6Playing.png"), true, false);
            return;
        }
        if (Phase == ETransitionPhase::Results)
        {
            Key(EKeys::SpaceBar, false);
            const int32 Kills = Runtime->GetDestroyedCount();
            if ((Cycle == 0 && (Runtime->GetElapsed() != 30 || Kills < 20 || Runtime->GetLives() <= 0)) ||
                (Cycle == 1 && (DamageCount != 3 || Runtime->GetLives() != 0))) { Finish(false, TEXT("Ending/high-score conditions")); return; }
            const int64 Expected = FMath::Min(10000, Kills * 500);
            if (State->GetScoring()->GetTotalScore() != Baseline + Expected)
            { Finish(false, FString::Printf(TEXT("Award mismatch baseline=%lld expected=%lld actual=%lld award=%lld"), Baseline, Expected, State->GetScoring()->GetTotalScore(), Flow->GetTransition().Award.AwardedPoints)); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("ASTEROID_ROUND cycle=%d kills=%d lives=%d duration=%.2f bonus=%lld"), Cycle, Kills, Runtime->GetLives(), Runtime->GetElapsed(), Expected);
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
        if (++Cycle == 2) { Finish(true, TEXT("Input, rebound, pause, projectiles, both endings, score and same-ball return")); return; }
        Stage = 1; Wait = 0;
    }
}
