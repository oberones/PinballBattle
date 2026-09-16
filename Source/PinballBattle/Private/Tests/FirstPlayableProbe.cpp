#include "Tests/FirstPlayableProbe.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballPlayerController.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballFlipper.h"
#include "Pinball/PinballPlunger.h"
#include "Pinball/BumperResponseComponent.h"
#include "Pinball/TableSessionComponent.h"
#include "Data/PinballTuningData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "PinballBattle.h"

AFirstPlayableProbe::AFirstPlayableProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AFirstPlayableProbe::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    SetActorTickEnabled(false);
#else
    SetActorTickEnabled(FParse::Param(FCommandLine::Get(), TEXT("PinballPracticeProbe")));
    FParse::Value(FCommandLine::Get(), TEXT("PinballProbeCycles="), TargetCycles);
    TargetCycles = FMath::Clamp(TargetCycles, 2, 100);
#endif
}

void AFirstPlayableProbe::Key(FKey InKey, bool bPressed)
{
    Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, InKey,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void AFirstPlayableProbe::Finish(bool bSuccess, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true;
    const FString Summary = FString::Printf(
        TEXT("%s: %s; cycles=%d contacts=%d bumper=%d flipper=%d peakSpeed=%.1f leftTravel=%.1f rightTravel=%.1f shortSpeed=%.1f fullSpeed=%.1f observedFPS=%.1f"),
        bSuccess ? TEXT("PINBALL_PRACTICE_PASS") : TEXT("PINBALL_PRACTICE_FAIL"), *Reason,
        Cycles, RelevantContacts, BumperContacts, FlipperContacts, PeakSpeed,
        LeftMaxAngle - LeftMinAngle, RightMaxAngle - RightMinAngle, ShortLaunchSpeed, FullLaunchSpeed,
        SampledTime > 0 ? SampledFrames / SampledTime : 0);
    UE_LOG(LogPinballBattle, Display, TEXT("%s"), *Summary);
    FString Name = TEXT("PracticeProbe");
    FParse::Value(FCommandLine::Get(), TEXT("PinballProbeName="), Name);
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Automation/Practice");
    IFileManager::Get().MakeDirectory(*Dir, true);
    FFileHelper::SaveStringToFile(Summary, *(Dir / (FPaths::MakeValidFileName(Name) + TEXT(".txt"))));
    FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void AFirstPlayableProbe::ObserveHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent* Other,
    FVector, const FHitResult& Hit)
{
    if (!Other || !Table) return;
    // Exclude playfield support; re-arm only after measurable separation, never per substep.
    if (FMath::Abs(FVector::DotProduct(Hit.ImpactNormal, Table->GetTableNormal())) > .7f) return;
    if (LastContacts.Contains(Other)) return;
    LastContacts.Add(Other, Clock);
    ++RelevantContacts;
    if (Cast<APinballBumper>(OtherActor)) ++BumperContacts;
    if (Cast<APinballFlipper>(OtherActor)) ++FlipperContacts;
}

void AFirstPlayableProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    Clock += DeltaSeconds;
    if (Clock < 1.f) return;
    if (!Mode)
    {
        Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
        Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController());
        Table = Mode ? Mode->GetTable() : nullptr;
        if (!Table || !Controller || !Mode->CanLaunch()) { Finish(false, TEXT("Boot did not produce ready ball")); return; }
        if (Controller->GetViewTarget() != Table) { Finish(false, TEXT("Wrong camera")); return; }
        // No launch can result from cancellation followed by a release callback.
        Table->Plunger->BeginCharge();
        Table->Plunger->CancelActions();
        if (Table->Plunger->ReleaseCharge() != 0 || !Mode->CanLaunch())
        { Finish(false, TEXT("Cancellation fired plunger")); return; }
    }
    int32 LiveBalls = 0;
    for (TActorIterator<APinballBall> It(GetWorld()); It; ++It) if (!It->IsActorBeingDestroyed()) ++LiveBalls;
    if (LiveBalls > 1 || Table->GetSession()->GetBodies().Num() > 3)
    { Finish(false, TEXT("Duplicate ball or leaked registry entry")); return; }

    const auto Angle = [this](APinballFlipper* Flipper)
    {
        const FVector Local = Flipper->GetActorTransform().InverseTransformVectorNoScale(Flipper->GetBody()->GetForwardVector());
        return FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
    };
    const float LeftAngle = Angle(Table->LeftFlipper);
    const float RightAngle = Angle(Table->RightFlipper);
    LeftMinAngle = FMath::Min(LeftMinAngle, LeftAngle); LeftMaxAngle = FMath::Max(LeftMaxAngle, LeftAngle);
    RightMinAngle = FMath::Min(RightMinAngle, RightAngle); RightMaxAngle = FMath::Max(RightMaxAngle, RightAngle);

    if (Stage == 0 && Mode->CanLaunch())
    {
        if (Cycles == 0) FScreenshotRequest::RequestScreenshot(TEXT("PracticeReady"), true, true);
        ObservedBall = Table->GetBall();
        ObservedBall->GetBody()->OnComponentHit.AddDynamic(this, &ThisClass::ObserveHit);
        if (Mode->RequestDrain(ObservedBall.Get())) { Finish(false, TEXT("Ready ball accepted a drain")); return; }
        LastContacts.Reset();
        Key(EKeys::Down, true);
        StageStart = Clock;
        Stage = 1;
    }
    else if (Stage == 1 && Clock - StageStart > (Cycles % 2 == 0 ? .12f : 1.4f))
    {
        Key(EKeys::Down, false);
        StageStart = Clock;
        Stage = 2;
        if (Cycles == 0) FScreenshotRequest::RequestScreenshot(TEXT("PracticeLaunch"), true, true);
    }
    else if (Stage == 2)
    {
        if (Mode->CanPlay())
        {
            LaunchSpeed = Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().Size();
            if (Cycles % 2 == 0) ShortLaunchSpeed = LaunchSpeed; else FullLaunchSpeed = LaunchSpeed;
            MaxTravel = 0;
            StageStart = Clock;
            Stage = 3;
            if (Mode->RequestLaunch(100)) { Finish(false, TEXT("Double launch accepted")); return; }
        }
        else if (Clock - StageStart > 1) { Finish(false, TEXT("Mapped Down release did not launch")); return; }
    }
    else if (Stage == 3)
    {
        const float PlayingTime = Clock - StageStart;
        if (PlayingTime > 3.2f) { ++SampledFrames; SampledTime += DeltaSeconds; }
        if (Cycles == 0 && PlayingTime > 3.f && PlayingTime - DeltaSeconds <= 3.f)
            FScreenshotRequest::RequestScreenshot(TEXT("PracticePlaying"), true, true);
        // Both flippers use independent mapped keys. Stop flipping after ten seconds to drain naturally.
        const bool Left = PlayingTime < 10 && FMath::Fmod(PlayingTime, .9f) < .25f;
        const bool Right = PlayingTime < 10 && FMath::Fmod(PlayingTime + .35f, 1.1f) < .25f;
        if (Left != bLeft) { Key(EKeys::Left, Left); bLeft = Left; }
        if (Right != bRight) { Key(EKeys::Right, Right); bRight = Right; }
        if (ObservedBall.IsValid())
        {
            for (auto It = LastContacts.CreateIterator(); It; ++It)
            {
                FVector Closest;
                if (!It.Key().IsValid() || It.Key()->GetClosestPointOnCollision(ObservedBall->GetActorLocation(), Closest) >
                    ObservedBall->GetBody()->GetScaledSphereRadius() + 1.f) It.RemoveCurrent();
            }
            const FVector Local = Table->GetActorTransform().InverseTransformPosition(ObservedBall->GetActorLocation());
            MaxTravel = FMath::Max(MaxTravel, static_cast<float>(Local.Y));
            PeakSpeed = FMath::Max(PeakSpeed, static_cast<float>(ObservedBall->GetBody()->GetPhysicsLinearVelocity().Size()));
            if (Local.Z < -30 || FMath::Abs(Local.X) > 350 || Local.Y > 1240)
            { Finish(false, FString::Printf(TEXT("Ball escaped thick collision: %s"), *Local.ToCompactString())); return; }
            if (Clock - LastSample > 1.f)
            {
                UE_LOG(LogPinballBattle, Log, TEXT("Probe ball: cycle=%d time=%.2f local=%s speed=%.1f flippers=%.1f/%.1f contacts=%d"),
                    Cycles + 1, PlayingTime, *Local.ToCompactString(), ObservedBall->GetBody()->GetPhysicsLinearVelocity().Size(),
                    LeftAngle, RightAngle, RelevantContacts);
                LastSample = Clock;
            }
        }
        if (Mode->CanLaunch())
        {
            if (ObservedBall.IsValid() || LiveBalls != 1 || MaxTravel < 900 || PlayingTime < 2)
            { Finish(false, TEXT("Replacement identity, useful launch or immediate-drain check failed")); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("Probe cycle %d: charge=%s launchSpeed=%.1f maxY=%.1f duration=%.2f contacts=%d"),
                Cycles + 1, Cycles % 2 == 0 ? TEXT("short") : TEXT("full"), LaunchSpeed, MaxTravel, PlayingTime, RelevantContacts);
            ++Cycles;
            Stage = 0;
            if (Cycles >= TargetCycles)
            {
                // Adversarial duplicate delivery uses the same production gate after natural acceptance.
                APinballBall* FinalBall = Table->GetBall();
                if (!Mode->RequestLaunch(Table->Tuning->MinLaunchImpulse) || !Mode->RequestDrain(FinalBall) ||
                    Mode->RequestDrain(FinalBall))
                { Finish(false, TEXT("Once-only drain gate failed")); return; }
                Stage = 4;
            }
        }
        else if (PlayingTime > 50) Finish(false, TEXT("Ball did not drain within 50 seconds; inspect geometry"));
    }
    else if (Stage == 4 && Mode->CanLaunch())
    {
        const bool bTravel = LeftMaxAngle - LeftMinAngle > 40 && RightMaxAngle - RightMinAngle > 40;
        Finish(bTravel && BumperContacts > 0 && FlipperContacts > 0 && LiveBalls == 1 &&
            Table->GetSession()->GetBodies().Num() == 3 && FullLaunchSpeed > ShortLaunchSpeed + 250.f,
            TEXT("Natural mapped cycles and duplicate-drain/one-replacement checks complete"));
    }
    if (Clock > TargetCycles * 55.f + 10.f) Finish(false, TEXT("Probe watchdog"));
}
