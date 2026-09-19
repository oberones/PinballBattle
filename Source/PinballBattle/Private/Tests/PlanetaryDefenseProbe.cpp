#include "Tests/PlanetaryDefenseProbe.h"
#include "Tests/ReturnBallProbe.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseAimPawn.h"
#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Minigames/PlanetaryDefense/DefenseBlastZone.h"
#include "Minigames/PlanetaryDefense/DefenseInterceptor.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballGameStateBase.h"
#include "Framework/PinballScoringComponent.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballPlayerController.h"
#include "UI/PinballPresentationWidget.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/MinigameTriggerComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "PinballBattle.h"

// Only the opt-in observer ticks during pause; production actors remain frozen by native pause.
APlanetaryDefenseProbe::APlanetaryDefenseProbe()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.bTickEvenWhenPaused = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
}

// Opening this test map normally still provides a playable single-game cabinet.
void APlanetaryDefenseProbe::BeginPlay()
{
    Super::BeginPlay(); bEnabled = FParse::Param(FCommandLine::Get(), TEXT("DefenseProbe")); Started = FPlatformTime::Seconds();
}

// Input injection retains real mappings and fresh-key handling instead of invoking FireAt directly.
void APlanetaryDefenseProbe::Key(FKey Input, bool Pressed)
{
    GetWorld()->GetFirstPlayerController()->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        Pressed ? IE_Pressed : IE_Released, Pressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

// The runtime's camera projects the assisted pointer; the pawn must independently deproject it correctly.
bool APlanetaryDefenseProbe::PointAt(const FVector& WorldPosition)
{
    auto* Player = GetWorld()->GetFirstPlayerController(); FVector2D Screen;
    if (!Player->ProjectWorldLocationToScreen(WorldPosition, Screen)) return false;
    Player->SetMouseLocation(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y)); return true;
}

// Explicit assertions, exit status and clean teardown are all required by the runner.
void APlanetaryDefenseProbe::Finish(bool Passed, const FString& Detail)
{
    bEnabled = false; UE_LOG(LogPinballBattle, Display, TEXT("PINBALL_DEFENSE_%s %s"), Passed ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    FPlatformMisc::RequestExitWithStatus(false, Passed ? 0 : 1);
}

// Observe the actual emitted payload so success and duration cannot be inferred merely from the HUD.
void APlanetaryDefenseProbe::ReceiveResult(const FMiniGameResult& Result)
{
    ++ResultCount; bResultSuccess = Result.bSuccess; ResultDuration = Result.DurationSeconds;
}

// Two real thirty-second runs validate controls, colony loss, pause and same-ball return with siblings absent.
void APlanetaryDefenseProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); if (!bEnabled) return;
    if (FPlatformTime::Seconds() - Started > 130) { Finish(false, FString::Printf(TEXT("Deadline stage=%d"), Stage)); return; }
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>(); auto* State = GetWorld()->GetGameState<APinballGameStateBase>();
    if (!Mode || !State || !Objective || !Mode->GetTable()) return;
    auto* Flow = Mode->FindComponentByClass<UGameFlowComponent>(); auto* Table = Mode->GetTable();
    auto* Player = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* Runtime = Cast<APlanetaryDefenseRuntime>(Flow->GetRuntime()); const auto Phase = Flow->GetTransition().Phase;
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
        Key(EKeys::SpaceBar, true);
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics); Stage = 2; Wait = 0; return;
    }
    if (Stage == 2 && Phase == ETransitionPhase::AwaitingConfirmation && Runtime)
    {
        Wait += DeltaSeconds;
        if (Runtime->GetElapsed() != 0 || Runtime->GetShotsFired() != 0) { Finish(false, TEXT("Held entry key started gameplay")); return; }
        if (Wait < .3) return;
        FrozenBall = Table->GetBall()->GetActorLocation(); Baseline = State->GetScoring()->GetTotalScore();
        Runtime->OnMiniGameEnded.AddUniqueDynamic(this, &ThisClass::ReceiveResult);
        Key(EKeys::SpaceBar, false); Stage = 3; Wait = 0; return;
    }
    if (Stage == 3)
    {
        Wait += DeltaSeconds; if (Wait < .2) return;
        Key(EKeys::SpaceBar, true); Stage = 4; return;
    }
    if (Stage == 4 && Runtime && Phase == ETransitionPhase::Playing)
    {
        if (Runtime->GetShotsFired() != 0) { Finish(false, TEXT("Confirmation fired a shot")); return; }
        if (Runtime->GetElapsed() < .3) return;
        Key(EKeys::SpaceBar, false); Stage = 5; ObservedRuntime = Runtime;
        FScreenshotRequest::RequestScreenshot(TEXT("Phase7Entry.png"), true, false); return;
    }
    if (Stage == 5 && Runtime)
    {
        if (!FrozenBall.Equals(Table->GetBall()->GetActorLocation(), .01) || Table->GetBall()->GetBody()->IsSimulatingPhysics())
        { Finish(false, TEXT("Table was not suspended")); return; }
        if (GetWorld()->IsPaused())
        {
            // Input is processed on the following frame. Capture the accepted pause boundary,
            // allowing a simultaneous Space press that was legitimately admitted before Escape.
            if (!bSawPause)
            {
                bSawPause = true; PausedClock = Runtime->GetElapsed(); PauseShots = Runtime->GetShotsFired();
                if (ObservedThreat.IsValid()) FrozenThreat = ObservedThreat->GetActorLocation();
            }
            if (!bEscapeReleased) { Key(EKeys::Escape, false); bEscapeReleased = true; }
            if (Runtime->GetElapsed() != PausedClock || Runtime->GetShotsFired() != PauseShots ||
                (ObservedThreat.IsValid() && !ObservedThreat->GetActorLocation().Equals(FrozenThreat, .01)))
            { Finish(false, TEXT("Pause advanced clock, fire or threat movement")); return; }
            if (FPlatformTime::Seconds() - PauseStarted > .3) Key(EKeys::Escape, true);
            return;
        }
        if (Phase == ETransitionPhase::Playing)
        {
            const double Time = Runtime->GetElapsed(); auto* Aim = CastChecked<ADefenseAimPawn>(Runtime->GetRunPawn());
            if (!Player->bShowMouseCursor) { Finish(false, TEXT("Mouse cursor hidden during Defense")); return; }
            if (!bAimChecked && Time > .5)
            {
                FVector2D Top, Bottom;
                Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(FVector(0, 390, 0)), Top);
                Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(FVector(0, -300, 0)), Bottom);
                if (Top.Y >= Bottom.Y) { Finish(false, TEXT("Threats must descend toward colonies on screen")); return; }
                const FVector Target(480, 210, 0);
                if (!PointAt(Runtime->GetActorTransform().TransformPosition(Target)) || !Aim->UpdateMouseAim() || !Aim->GetLocalAim().Equals(Target, 4))
                {
                    FVector Origin, Direction; FVector2D Screen; float MouseX = 0, MouseY = 0;
                    Player->DeprojectMousePositionToWorld(Origin, Direction); Player->GetMousePosition(MouseX, MouseY);
                    Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(Target), Screen);
                    Finish(false, FString::Printf(TEXT("Mouse aim=%s ray=%s / %s mouse=%.1f,%.1f screen=%s camera=%s / %s"),
                        *Aim->GetLocalAim().ToString(), *Origin.ToString(), *Direction.ToString(), MouseX, MouseY, *Screen.ToString(),
                        *Runtime->Camera->GetComponentLocation().ToString(), *Runtime->Camera->GetComponentRotation().ToString())); return;
                }
                bAimChecked = true;
            }
            if (!bPaused && Time > 2)
            {
                bPaused = true; PausedClock = Time; PauseShots = Runtime->GetShotsFired(); PauseStarted = FPlatformTime::Seconds();
                for (TActorIterator<ADefenseThreat> It(GetWorld()); It; ++It) if (!It->IsResolved() && It->GetOwner() == Runtime) { ObservedThreat = *It; FrozenThreat = It->GetActorLocation(); break; }
                Key(EKeys::SpaceBar, true); Key(EKeys::Escape, true); return;
            }
            if (bPaused && Time < PausedClock + .3)
            {
                if (bSawPause) Key(EKeys::Escape, false);
                if (Runtime->GetShotsFired() != PauseShots) { Finish(false, TEXT("Held fire leaked through resume")); return; }
                return;
            }
            if (bPaused && !bPauseReleased) { Key(EKeys::SpaceBar, false); bPauseReleased = true; return; }
            if (Time < .65 || (bPaused && Time < PausedClock + .45)) return;
            if (Cycle == 1 && Runtime->GetSurvivorCount() > 0) { Key(EKeys::SpaceBar, false); return; }
            if (Cycle == 1 && LossKills < 0) LossKills = Runtime->GetDestroyedCount();
            ADefenseThreat* Target = nullptr; double Lowest = TNumericLimits<double>::Max();
            for (TActorIterator<ADefenseThreat> It(GetWorld()); It; ++It)
            {
                if (It->GetOwner() != Runtime || It->IsResolved()) continue;
                const double Height = Runtime->GetActorTransform().InverseTransformPosition(It->GetActorLocation()).Y;
                if (Height < Lowest) { Lowest = Height; Target = *It; }
            }
            if (Target)
            {
                const double Travel = FVector::Distance(Runtime->GetLauncherLocation(), Target->GetActorLocation()) / Runtime->InterceptorSpeed;
                const FVector Prediction = Target->GetActorLocation() + Target->Velocity * Travel;
                PointAt(Runtime->GetActorTransform().TransformPosition(Runtime->ClampAim(Runtime->GetActorTransform().InverseTransformPosition(Prediction))));
            }
            Key(EKeys::SpaceBar, true);
            if (!bScreenshot && Time > 15)
            { bScreenshot = true; FScreenshotRequest::RequestScreenshot(Cycle ? TEXT("Phase7NoColonies.png") : TEXT("Phase7Playing.png"), true, false); }
            return;
        }
        if (Phase == ETransitionPhase::Results)
        {
            Key(EKeys::SpaceBar, false);
            const int32 Kills = Runtime->GetDestroyedCount(), Survivors = Runtime->GetSurvivorCount();
            if (Runtime->GetElapsed() != 30 || !bAimChecked || !bSawPause || ResultCount != 1 || ResultDuration != 30 || bResultSuccess != (Survivors > 0) ||
                (Cycle == 0 && (Kills < 28 || Survivors != 3)) || (Cycle == 1 && (Survivors != 0 || LossKills < 0 || Kills - LossKills < 4)))
            { Finish(false, FString::Printf(TEXT("Round outcome cycle=%d kills=%d survivors=%d time=%.2f"), Cycle, Kills, Survivors, Runtime->GetElapsed())); return; }
            const int64 Expected = FMath::Min(10000, Kills * 250 + Survivors * 1000);
            if (State->GetScoring()->GetTotalScore() != Baseline + Expected || Flow->GetTransition().Award.AwardedPoints != Expected)
            { Finish(false, TEXT("Central award mismatch")); return; }
            if (Runtime->GetLocalScore() != int64(Kills) * 100 + Survivors * 500) { Finish(false, TEXT("Local survivor score")); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("DEFENSE_ROUND cycle=%d threats=%d kills=%d colonies=%d duration=%.2f shots=%d bonus=%lld"),
                Cycle, Runtime->GetSpawnedCount(), Kills, Survivors, Runtime->GetElapsed(), Runtime->GetShotsFired(), Expected);
            if (Cycle == 1)
            {
                // Force the collision-checked backup path on the second return.
                SavedPrimaryReturn = Table->PrimaryReturn->GetComponentTransform();
                Table->PrimaryReturn->SetWorldLocation(Table->GetActorLocation());
            }
            Stage = 6; return;
        }
    }
    if (Stage == 6 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        if (Player->bShowMouseCursor || !Table->GetBall()->GetBody()->IsSimulatingPhysics()) { Finish(false, TEXT("Cursor/physics not restored")); return; }
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
            if (ObservedRuntime.IsValid() && It->GetOwner() == ObservedRuntime.Get() && !It->IsActorBeingDestroyed())
            { Finish(false, TEXT("Run actor leaked after return")); return; }
        ReturnBall = NewObject<UReturnBallProbe>(this); ReturnBall->Begin(Table, Player);
        if (Cycle == 1) Table->PrimaryReturn->SetWorldTransform(SavedPrimaryReturn);
        Stage = 7;
    }
    if (Stage == 7 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        FString Failure;
        const auto Check = ReturnBall->Tick(DeltaSeconds, Failure);
        if (Check == UReturnBallProbe::EResult::Running) return;
        if (Check == UReturnBallProbe::EResult::Failed) { Finish(false, Failure); return; }
        Stage = 8; ReturnControls.Reset();
    }
    if (Stage == 8 && Flow->GetCurrentState() == EArcadeGameFlowState::PINBALL_PLAYING)
    {
        // Keep the ball clear of targets while measuring each returned paddle independently.
        Table->GetBall()->SetActorLocation(Objective->GetActorLocation() + FVector(140, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
        Table->GetBall()->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        FString Failure;
        const auto Check = ReturnControls.Tick(Table, Player, DeltaSeconds, Failure);
        if (Check == FFlipperReturnCheck::EResult::Running) return;
        if (Check == FFlipperReturnCheck::EResult::Failed) { Finish(false, Failure); return; }
        if (++Cycle == 2) { Finish(true, TEXT("Mouse/Space, held-key barriers, pause, swept kills, both thirty-second endings, bonuses, cleanup and same-ball return")); return; }
        Stage = 1; Wait = 0; ResultCount = 0; bPaused = bPauseReleased = bAimChecked = bScreenshot = bSawPause = bEscapeReleased = false;
    }
}
