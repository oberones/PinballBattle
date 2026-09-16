#include "Tests/TableInteractionProbe.h"
#include "Framework/PinballGameModeBase.h"
#include "Framework/PinballPlayerController.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/ScoringTargetComponent.h"
#include "Pinball/LaneProgressComponent.h"
#include "Pinball/BumperResponseComponent.h"
#include "Pinball/TableSessionComponent.h"
#include "Data/PinballTuningData.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "PinballBattle.h"

ATableInteractionProbe::ATableInteractionProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ATableInteractionProbe::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    SetActorTickEnabled(false);
#else
    SetActorTickEnabled(FParse::Param(FCommandLine::Get(), TEXT("PinballInteractionProbe")));
    FParse::Value(FCommandLine::Get(), TEXT("PinballInteractionCycles="), TargetCycles);
    TargetCycles = FMath::Clamp(TargetCycles, 2, 100);
    if (FParse::Param(FCommandLine::Get(), TEXT("PinballFixturesOnly"))) TargetCycles = 0;
#endif
}

void ATableInteractionProbe::Key(FKey Input, bool bPressed)
{
    Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Input,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
}

void ATableInteractionProbe::Finish(bool bSuccess, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true;
    FString Summary = FString::Printf(TEXT("PINBALL_INTERACTIONS_%s: %s; naturalCycles=%d totalEvents=%d naturalSources=%d recoveries=%d"),
        bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Reason, Cycles, EventIds.Num(), NaturalSources.Num(), Table ? Table->GetRecoveryCount() : 0);
    if (Table)
    {
        for (int32 I = 0; I < Table->Targets.Num(); ++I)
            Summary += FString::Printf(TEXT(" target%d=%d"), I+1, NaturalSources.FindRef(Table->Targets[I]->Scoring->GetSourceId()));
        for (int32 I = 0; I < Table->Lanes.Num(); ++I)
            Summary += FString::Printf(TEXT(" lane%d=%d"), I+1, NaturalSources.FindRef(Table->Lanes[I]->Progress->GetSourceId()));
        for (int32 I = 0; I < Table->Bumpers.Num(); ++I)
            Summary += FString::Printf(TEXT(" bumper%d=%d"), I+1, NaturalSources.FindRef(Table->Bumpers[I]->Response->GetSourceId()));
    }
    UE_LOG(LogPinballBattle, Display, TEXT("%s"), *Summary);
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/Interactions");
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Name = TEXT("Probe");
    FParse::Value(FCommandLine::Get(), TEXT("PinballProbeName="), Name);
    FFileHelper::SaveStringToFile(Summary, *(Directory / (FPaths::MakeValidFileName(Name) + TEXT(".txt"))));
    FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void ATableInteractionProbe::ObserveEvent(const FScoringEvent& Event)
{
    if (EventIds.Contains(Event.EventId) || !Event.SourceId.IsValid() || Event.Sequence <= LastSequences.FindRef(Event.SourceId) ||
        Event.SessionId != Table->GetBallHandle().SessionId || Event.BallId != Table->GetBallHandle().BallId ||
        Event.PhaseEpoch != Table->GetPhysicalEpoch() || Event.ObservedPhase != EArcadeGameFlowState::PINBALL_PLAYING)
    { Finish(false, TEXT("Duplicate/stale/malformed typed event")); return; }
    EventIds.Add(Event.EventId);
    LastSequences.Add(Event.SourceId, Event.Sequence);
    ++SourceCounts.FindOrAdd(Event.SourceId);
    if (Fixture < 0) ++NaturalSources.FindOrAdd(Event.SourceId);
}

void ATableInteractionProbe::Place(const FVector& LocalPosition, const FVector& LocalVelocity, bool bSimulate)
{
    APinballBall* Ball = Table->GetBall();
    Ball->GetBody()->SetSimulatePhysics(false);
    Ball->SetActorLocation(Table->GetActorTransform().TransformPosition(LocalPosition), false, nullptr, ETeleportType::TeleportPhysics);
    Ball->GetBody()->SetSimulatePhysics(bSimulate);
    if (bSimulate)
    {
        Ball->GetBody()->SetPhysicsLinearVelocity(Table->GetActorTransform().TransformVectorNoScale(LocalVelocity));
        Ball->GetBody()->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        Ball->GetBody()->WakeAllRigidBodies();
    }
}

void ATableInteractionProbe::StartFixture(int32 Index)
{
    Fixture = Index;
    FixtureStart = Clock;
    BeforeEvents = EventIds.Num();
    BeforeRecovery = Table->GetRecoveryCount();
    UE_LOG(LogPinballBattle, Display, TEXT("Interaction fixture %d begins"), Index);
    if (Index < 4)
    {
        const FVector Positions[] = {{-200,540,14}, {-200,900,14}, {105,710,14}, {80,1040,14}};
        const FVector Velocities[] = {{-450,0,0}, {-450,0,0}, {450,0,0}, {0,500,0}};
        ExpectedSource = Table->Targets[Index]->Scoring->GetSourceId();
        BeforeEvents = SourceCounts.FindRef(ExpectedSource);
        Place(Positions[Index], Velocities[Index]);
    }
    else if (Index < 7)
    {
        APinballBumper* Bumper = Table->Bumpers[Index - 4];
        ExpectedSource = Bumper->Response->GetSourceId();
        BeforeEvents = SourceCounts.FindRef(ExpectedSource);
        FVector Local = Table->GetActorTransform().InverseTransformPosition(Bumper->GetActorLocation());
        Local.Y -= 105;
        Local.Z = 14;
        Place(Local, FVector(0,500,0));
    }
    else if (Index == 7) Place(FVector(0,450,14), FVector::ZeroVector, false); // Ten-second trap.
    else if (Index == 8) Place(FVector(600,600,14), FVector::ZeroVector); // Escape, same entitlement.
    else if (Index == 9)
    {
        Table->PrimaryReturn->SetRelativeLocation(FVector(0,450,-20)); // Inside solid playfield.
        Place(FVector(600,600,14), FVector::ZeroVector);
    }
    else if (Index == 10)
    {
        Table->BackupReturn->SetRelativeLocation(FVector(0,450,-20));
        Place(FVector(600,600,14), FVector::ZeroVector);
    }
    else if (Index == 11) Place(FVector(270,80,14), FVector::ZeroVector, false); // Launch exclusion.
    else if (Index == 12)
    {
        UBoxComponent* Capture = NewObject<UBoxComponent>(Table);
        Capture->SetBoxExtent(FVector(40,40,40));
        Capture->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Capture->RegisterComponent();
        Capture->SetWorldLocation(Table->GetActorTransform().TransformPosition(FVector(0,450,14)));
        Table->CaptureExemptions.Add(Capture);
        Place(FVector(0,450,14), FVector::ZeroVector, false);
    }
}

void ATableInteractionProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    Clock += DeltaSeconds;
    if (Clock < 1) return;
    if (!Mode)
    {
        Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
        Controller = Cast<APinballPlayerController>(GetWorld()->GetFirstPlayerController());
        Table = Mode ? Mode->GetTable() : nullptr;
        if (!Table || !Controller || !Mode->CanLaunch() || Table->Targets.Num() != 4 || Table->Lanes.Num() != 2)
        { Finish(false, TEXT("Incomplete ready cabinet")); return; }
        Table->OnScoringEvent.AddDynamic(this, &ThisClass::ObserveEvent);
        SavedSessionId = Table->GetBallHandle().SessionId;
        PrimaryPosition = Table->PrimaryReturn->GetRelativeLocation();
        BackupPosition = Table->BackupReturn->GetRelativeLocation();
        FScreenshotRequest::RequestScreenshot(TEXT("Phase3Ready"), true, true);
    }
    int32 LiveBalls = 0;
    for (TActorIterator<APinballBall> It(GetWorld()); It; ++It) if (!It->IsActorBeingDestroyed()) ++LiveBalls;
    if (LiveBalls > 1 || Table->GetSession()->GetBodies().Num() > 3 || SavedSessionId != Table->GetBallHandle().SessionId)
    { Finish(false, TEXT("Ball/registry/session invariant failed")); return; }
    if (Fixture >= 0)
    {
        const float Elapsed = Clock - FixtureStart;
        if (Fixture < 13 && Table->GetBallHandle().BallId != SavedBallId) { Finish(false, TEXT("Fixture consumed entitlement")); return; }
        if (Fixture < 7 && Elapsed > .23f)
        {
            if (SourceCounts.FindRef(ExpectedSource) != BeforeEvents + 1)
            {
                Finish(false, FString::Printf(TEXT("Physical fixture %d expected one strike, delta=%d local=%s velocity=%s"),
                    Fixture, SourceCounts.FindRef(ExpectedSource) - BeforeEvents,
                    *Table->GetActorTransform().InverseTransformPosition(Table->GetBall()->GetActorLocation()).ToCompactString(),
                    *Table->GetBall()->GetBody()->GetPhysicsLinearVelocity().ToCompactString()));
                return;
            }
            const UPointLightComponent* Lamp = Fixture < 4 ? Table->Targets[Fixture]->Flash.Get() : Table->Bumpers[Fixture-4]->Flash.Get();
            if (!Lamp || Lamp->Intensity <= 0) { Finish(false, TEXT("Accepted strike did not flash")); return; }
            StartFixture(Fixture + 1);
        }
        else if (Fixture == 7)
        {
            if (Elapsed < 9.9f && Table->GetRecoveryCount() != BeforeRecovery) { Finish(false, TEXT("Trap recovered too early")); return; }
            if (Elapsed > 10.15f)
            {
                if (Table->GetRecoveryCount() != BeforeRecovery + 1 || EventIds.Num() != BeforeEvents)
                { Finish(false, TEXT("Ten-second trap recovery failed or awarded an event")); return; }
                StartFixture(8);
            }
        }
        else if ((Fixture == 8 || Fixture == 9) && Elapsed > .2f)
        {
            if (Table->GetRecoveryCount() != BeforeRecovery + 1 || EventIds.Num() != BeforeEvents)
            { Finish(false, TEXT("Escape/fallback recovery failed or awarded an event")); return; }
            if (Fixture == 9 && Table->GetActorTransform().InverseTransformPosition(Table->GetBall()->GetActorLocation()).X > -150)
            { Finish(false, TEXT("Blocked primary did not use backup")); return; }
            StartFixture(Fixture + 1);
        }
        else if (Fixture == 10 && Elapsed > .5f)
        {
            if (Table->GetBallHandle().Disposition != EBallDisposition::Recovering || Table->GetRecoveryCount() != BeforeRecovery ||
                EventIds.Num() != BeforeEvents || Table->GetBall()->GetBody()->IsSimulatingPhysics() || Mode->RequestDrain(Table->GetBall()))
            { Finish(false, TEXT("Both-blocked recovery did not secure the ball and close events")); return; }
            Table->PrimaryReturn->SetRelativeLocation(PrimaryPosition);
            Table->BackupReturn->SetRelativeLocation(BackupPosition);
            if (!Table->RecoverBall()) { Finish(false, TEXT("Unblocked recovery did not resume")); return; }
            StartFixture(11);
        }
        else if ((Fixture == 11 || Fixture == 12) && Elapsed > 10.25f)
        {
            if (Table->GetRecoveryCount() != BeforeRecovery) { Finish(false, TEXT("Launch/capture exemption ignored")); return; }
            if (Fixture == 11) StartFixture(12);
            else
            {
                for (UBoxComponent* Capture : Table->CaptureExemptions) Capture->DestroyComponent();
                Table->CaptureExemptions.Reset();
                if (Mode->RequestDrainEvent(StaleDrain)) { Finish(false, TEXT("Stale epoch accepted a drain")); return; }
                APinballBall* OldBall = Table->GetBall();
                if (!Mode->RequestDrain(OldBall) || Mode->RequestDrain(OldBall))
                { Finish(false, TEXT("Duplicate drain accepted")); return; }
                Fixture = 13;
            }
        }
        else if (Fixture == 13 && Mode->CanLaunch())
        {
            Finish(Table->GetBallHandle().BallId != SavedBallId && LiveBalls == 1 && Table->GetSession()->GetBodies().Num() == 3,
                TEXT("Mapped practice, physical strikes, typed events, trap/escape/blocked recovery and drain identity checked"));
        }
        return;
    }
    if (Stage == 0 && Mode->CanLaunch())
    {
        if (Cycles >= TargetCycles)
        {
            Key(EKeys::Left, false); Key(EKeys::Right, false);
            Mode->RequestLaunch(Table->Tuning->MinLaunchImpulse);
            SavedBallId = Table->GetBallHandle().BallId;
            StaleDrain.SessionId = SavedSessionId; StaleDrain.BallId = SavedBallId;
            StaleDrain.EventId = FGuid::NewGuid(); StaleDrain.SourceId = FGuid::NewGuid();
            StaleDrain.PhaseEpoch = Table->GetPhysicalEpoch();
            Stage = 4;
            StageStart = Clock;
            return;
        }
        SavedBallId = Table->GetBallHandle().BallId;
        Key(EKeys::Down, true);
        StageStart = Clock;
        Stage = 1;
    }
    else if (Stage == 1 && Clock - StageStart > (Cycles % 3 == 0 ? .12f : Cycles % 3 == 1 ? .65f : 1.4f))
    {
        Key(EKeys::Down, false);
        StageStart = Clock;
        Stage = 2;
    }
    else if (Stage == 2 && Mode->CanPlay())
    {
        StageStart = Clock;
        Stage = 3;
    }
    else if (Stage == 3)
    {
        const float Elapsed = Clock - StageStart;
        const bool Left = Elapsed < 15 && FMath::Fmod(Elapsed + Cycles * .13f, .87f) < .23f;
        const bool Right = Elapsed < 15 && FMath::Fmod(Elapsed + Cycles * .21f, 1.07f) < .24f;
        if (Left != bLeft) { Key(EKeys::Left, Left); bLeft = Left; }
        if (Right != bRight) { Key(EKeys::Right, Right); bRight = Right; }
        if (Cycles == 0 && Elapsed > 3 && Elapsed - DeltaSeconds <= 3)
            FScreenshotRequest::RequestScreenshot(TEXT("Phase3Playing"), true, true);
        if (Mode->CanLaunch())
        {
            if (SavedBallId == Table->GetBallHandle().BallId) { Finish(false, TEXT("Drain reused an entitlement ID")); return; }
            UE_LOG(LogPinballBattle, Display, TEXT("Natural cabinet cycle %d duration=%.2f sources=%d"), Cycles+1, Elapsed, NaturalSources.Num());
            ++Cycles;
            Stage = 0;
        }
        else if (Elapsed > 75) Finish(false, TEXT("Natural ball did not drain within 75 seconds"));
    }
    else if (Stage == 4 && Clock - StageStart > .3f) StartFixture(0);
    if (Clock > TargetCycles * 80 + 90) Finish(false, TEXT("Probe watchdog"));
}
