#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballFlipper.h"
#include "Pinball/PinballPlunger.h"
#include "Pinball/BumperResponseComponent.h"
#include "Pinball/DrainComponent.h"
#include "Pinball/TableSessionComponent.h"
#include "Data/PinballTuningData.h"
#include "Framework/PinballGameModeBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Pinball/ScoringTargetComponent.h"
#include "Pinball/LaneProgressComponent.h"
#include "Pinball/InteractionFeedbackComponent.h"
#include "Engine/OverlapResult.h"
#include "PinballBattle.h"

APinballTable::APinballTable()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TableRoot")));
    Session = CreateDefaultSubobject<UTableSessionComponent>(TEXT("TableSession"));
    BallSpawn = CreateDefaultSubobject<USceneComponent>(TEXT("BallSpawn"));
    BallSpawn->SetupAttachment(RootComponent);
    BallSpawn->SetRelativeLocation(FVector(270, 80, 14));
    TableCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TableCamera"));
    TableCamera->SetupAttachment(RootComponent);
    TableCamera->SetRelativeLocation(FVector(0, 400, 1600));
    TableCamera->SetRelativeRotation(FRotator(-90, 90, 0));
    TableCamera->FieldOfView = 53.f;
    TableCamera->bConstrainAspectRatio = true;
    TableCamera->AspectRatio = 16.f / 9.f;
    EscapeBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("EscapeBounds"));
    EscapeBounds->SetupAttachment(RootComponent);
    EscapeBounds->SetRelativeLocation(FVector(0, 550, 90));
    EscapeBounds->InitBoxExtent(FVector(370, 720, 160));
    EscapeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LaunchExemption = CreateDefaultSubobject<UBoxComponent>(TEXT("LaunchExemption"));
    LaunchExemption->SetupAttachment(RootComponent);
    LaunchExemption->SetRelativeLocation(FVector(270, 80, 15));
    LaunchExemption->InitBoxExtent(FVector(30, 55, 40));
    LaunchExemption->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrimaryReturn = CreateDefaultSubobject<USceneComponent>(TEXT("PrimaryReturn"));
    PrimaryReturn->SetupAttachment(RootComponent);
    PrimaryReturn->SetRelativeLocation(FVector(0, 450, 16));
    BackupReturn = CreateDefaultSubobject<USceneComponent>(TEXT("BackupReturn"));
    BackupReturn->SetupAttachment(RootComponent);
    BackupReturn->SetRelativeLocation(FVector(-180, 520, 16));
}

void APinballTable::BeginPlay()
{
    Super::BeginPlay();
    if (APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>()) Mode->RegisterTable(this);
}

bool APinballTable::InitializeTable(FString& OutError)
{
    if (!Tuning || !BallClass || !LeftFlipper || !RightFlipper || LeftFlipper == RightFlipper ||
        !Plunger || !Drain || Bumpers.Num() < 3 || !TableCamera || !BallSpawn)
    {
        OutError = TEXT("Table requires tuning, ball class, two distinct flippers, plunger, three bumpers, drain, spawn and camera.");
        return false;
    }
    if (!Tuning->Validate(OutError)) return false;
    if (ReturnVelocity.ContainsNaN() || ReturnVelocity.IsNearlyZero() || ReturnVelocity.Size() > Tuning->MaxBallSpeed ||
        (bRequireCompleteInventory && (Targets.Num() < 4 || Lanes.Num() < 2)))
    {
        OutError = TEXT("Invalid release velocity or incomplete target/lane inventory.");
        return false;
    }
    TSet<const AActor*> Inventory;
    for (const AActor* Element : Bumpers) { if (!Element || Inventory.Contains(Element)) return false; Inventory.Add(Element); }
    for (const AActor* Element : Targets) { if (!Element || Inventory.Contains(Element)) return false; Inventory.Add(Element); }
    for (const AActor* Element : Lanes) { if (!Element || Inventory.Contains(Element)) return false; Inventory.Add(Element); }
    BallHandle.SessionId = FGuid::NewGuid();
    for (APinballBumper* Bumper : Bumpers)
    {
        if (!IsValid(Bumper)) { OutError = TEXT("Missing explicit bumper reference."); return false; }
    }
    LeftFlipper->Configure(Tuning);
    RightFlipper->Configure(Tuning);
    Session->RegisterBody(LeftFlipper->GetBody());
    Session->RegisterBody(RightFlipper->GetBody());
    Plunger->Configure(Tuning);
    for (APinballBumper* Bumper : Bumpers)
    {
        Bumper->Response->Initialize(this, Bumper->Surface);
        Bumper->Response->Feedback = Bumper->Feedback;
        Bumper->Feedback->Initialize(Bumper->Flash);
    }
    for (APinballScoringTarget* Target : Targets)
    {
        Target->Scoring->Initialize(this, Target->Surface);
        Target->Scoring->Feedback = Target->Feedback;
        Target->Feedback->Initialize(Target->Flash);
    }
    for (APinballLane* Lane : Lanes)
    {
        Lane->Progress->Initialize(this, Lane->Corridor);
        Lane->Progress->Feedback = Lane->Feedback;
        Lane->Feedback->Initialize(Lane->Flash);
    }
    Drain->Drain->Initialize(this);
    return true;
}

bool APinballTable::SpawnReadyBall()
{
    if (IsValid(CurrentBall) || !Tuning || !BallClass) return false;
    CurrentBall = GetWorld()->SpawnActorDeferred<APinballBall>(BallClass, BallSpawn->GetComponentTransform(),
        this, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
    if (!CurrentBall) return false;
    CurrentBall->Configure(Tuning);
    CurrentBall->FinishSpawning(BallSpawn->GetComponentTransform());
    if (!IsValid(CurrentBall)) { CurrentBall = nullptr; return false; }
    Session->RegisterBody(CurrentBall->GetBody());
    BallHandle.BallId = FGuid::NewGuid();
    BallHandle.Ball = CurrentBall;
    BallHandle.Disposition = EBallDisposition::Ready;
    ++PhysicalEpoch;
    LowMotionSeconds = 0;
    return true;
}

void APinballTable::RemoveBall()
{
    APinballBall* OldBall = CurrentBall;
    CurrentBall = nullptr;
    BallHandle.Disposition = EBallDisposition::Drained;
    BallHandle.Ball.Reset();
    ++PhysicalEpoch;
    if (IsValid(OldBall))
    {
        Session->UnregisterBody(OldBall->GetBody());
        OldBall->Destroy();
    }
}

void APinballTable::CancelActions()
{
    if (LeftFlipper) LeftFlipper->SetHeld(false);
    if (RightFlipper) RightFlipper->SetHeld(false);
    if (Plunger) Plunger->CancelActions();
}

void APinballTable::ResetForNewSession(FGuid SessionId)
{
    BallHandle.SessionId = SessionId;
    ++PhysicalEpoch;
    CancelActions();
    RemoveBall();
    BallHandle.BallId.Invalidate();
    EventProtectionUntil = RecoveryRetryAt = 0;
    LowMotionSeconds = 0;
    RecoveryCount = 0;
    for (int32& Count : InteractionCounts) Count = 0;
    for (APinballBumper* Bumper : Bumpers) if (IsValid(Bumper)) Bumper->Feedback->ResetFeedback();
    for (APinballScoringTarget* Target : Targets) if (IsValid(Target)) Target->Feedback->ResetFeedback();
    for (APinballLane* Lane : Lanes) if (IsValid(Lane)) Lane->Feedback->ResetFeedback();
}

bool APinballTable::IsCurrentBall(const APinballBall* Ball) const
{
    return IsValid(Ball) && Ball == CurrentBall;
}

void APinballTable::NotifyBallLaunched()
{
    BallHandle.Disposition = EBallDisposition::Active;
    ++PhysicalEpoch;
    LowMotionSeconds = 0;
}

bool APinballTable::CanEmitEvent(const APinballBall* Ball) const
{
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    return Mode && Mode->CanPlay() && IsCurrentBall(Ball) && Ball->IsLaunched() &&
        BallHandle.Disposition == EBallDisposition::Active && GetWorld()->GetTimeSeconds() >= EventProtectionUntil;
}

FScoringEvent APinballTable::MakeScoringEvent(FGuid SourceId, EScoringCategory Category, int64 Sequence) const
{
    FScoringEvent Event;
    Event.SessionId = BallHandle.SessionId;
    Event.BallId = BallHandle.BallId;
    Event.EventId = FGuid::NewGuid();
    Event.SourceId = SourceId;
    Event.Category = Category;
    Event.Sequence = Sequence;
    Event.ObservedPhase = EArcadeGameFlowState::PINBALL_PLAYING;
    Event.PhaseEpoch = PhysicalEpoch;
    return Event;
}

void APinballTable::PublishScoringEvent(const FScoringEvent& Event)
{
    if (!CanEmitEvent(CurrentBall) || Event.SessionId != BallHandle.SessionId || Event.BallId != BallHandle.BallId ||
        Event.PhaseEpoch != PhysicalEpoch || !Event.EventId.IsValid() || !Event.SourceId.IsValid() || Event.Sequence <= 0 ||
        static_cast<uint8>(Event.Category) >= UE_ARRAY_COUNT(InteractionCounts)) return;
    ++InteractionCounts[static_cast<uint8>(Event.Category)];
    UE_LOG(LogPinballBattle, Log, TEXT("Table event category=%d sequence=%lld ball=%s event=%s"),
        static_cast<int32>(Event.Category), Event.Sequence, *Event.BallId.ToString(), *Event.EventId.ToString());
    OnScoringEvent.Broadcast(Event);
}

bool APinballTable::IsInside(const UBoxComponent* Volume, const FVector& Position) const
{
    if (!IsValid(Volume)) return false;
    const FVector Local = Volume->GetComponentTransform().InverseTransformPositionNoScale(Position);
    const FVector Extent = Volume->GetScaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool APinballTable::IsSafeRelease(const FVector& Position, const FVector& Velocity) const
{
    if (Position.ContainsNaN() || !IsInside(EscapeBounds, Position)) return false;
    const float Radius = CurrentBall->GetBody()->GetScaledSphereRadius() + 1.f;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    Objects.AddObjectTypesToQuery(ECC_GameTraceChannel2);
    Objects.AddObjectTypesToQuery(ECC_GameTraceChannel3);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PinballSafeRelease), false, CurrentBall);
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByObjectType(Overlaps, Position, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Radius), Query);
    if (!Overlaps.IsEmpty()) return false;
    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByObjectType(Hits, Position, Position + Velocity.GetSafeNormal() * Radius * 3,
        FQuat::Identity, Objects, FCollisionShape::MakeSphere(Radius), Query);
    return Hits.IsEmpty();
}

bool APinballTable::RecoverBall()
{
    if (!IsValid(CurrentBall) || !CurrentBall->IsLaunched()) return false;
    if (BallHandle.Disposition != EBallDisposition::Recovering)
    {
        UE_LOG(LogPinballBattle, Display, TEXT("Recovery requested at table position=%s lowMotionSeconds=%.3f"),
            *GetActorTransform().InverseTransformPosition(CurrentBall->GetActorLocation()).ToCompactString(), LowMotionSeconds);
        BallHandle.Disposition = EBallDisposition::Recovering;
        ++PhysicalEpoch;
        CurrentBall->GetBody()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        CurrentBall->GetBody()->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        CurrentBall->GetBody()->SetSimulatePhysics(false);
        CancelActions();
    }
    const FVector Velocity = GetActorTransform().TransformVectorNoScale(ReturnVelocity);
    for (const USceneComponent* Marker : {PrimaryReturn.Get(), BackupReturn.Get()})
    {
        if (!Marker || !IsSafeRelease(Marker->GetComponentLocation(), Velocity)) continue;
        CurrentBall->SetActorLocation(Marker->GetComponentLocation(), false, nullptr, ETeleportType::TeleportPhysics);
        CurrentBall->GetBody()->SetSimulatePhysics(true);
        CurrentBall->GetBody()->SetPhysicsLinearVelocity(Velocity);
        CurrentBall->GetBody()->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        CurrentBall->GetBody()->WakeAllRigidBodies();
        BallHandle.Disposition = EBallDisposition::Active;
        EventProtectionUntil = GetWorld()->GetTimeSeconds() + .15;
        LowMotionSeconds = 0;
        ++RecoveryCount;
        UE_LOG(LogPinballBattle, Display, TEXT("Ball recovered without award/loss: ball=%s backup=%d"),
            *BallHandle.BallId.ToString(), Marker == BackupReturn);
        return true;
    }
    RecoveryRetryAt = GetWorld()->GetTimeSeconds() + .25;
    return false; // Keep one secured ball and retry; never spawn an unsafe replacement.
}

void APinballTable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (!Mode || !Mode->CanPlay() || !IsValid(CurrentBall)) { LowMotionSeconds = 0; return; }
    if (BallHandle.Disposition == EBallDisposition::Recovering)
    {
        if (GetWorld()->GetTimeSeconds() >= RecoveryRetryAt) RecoverBall();
        return;
    }
    const FVector Position = CurrentBall->GetActorLocation();
    if (Position.ContainsNaN() || !IsInside(EscapeBounds, Position)) { RecoverBall(); return; }
    bool bExempt = IsInside(LaunchExemption, Position);
    for (const UBoxComponent* Volume : CaptureExemptions) bExempt |= IsInside(Volume, Position);
    if (bExempt || CurrentBall->GetBody()->GetPhysicsLinearVelocity().Size() > Tuning->LowMotionSpeed)
    {
        LowMotionSeconds = 0;
        TrapAnchor = Position;
        return;
    }
    if (LowMotionSeconds == 0 || FVector::DistSquared(TrapAnchor, Position) > FMath::Square(CurrentBall->GetBody()->GetScaledSphereRadius()))
    {
        LowMotionSeconds = 0;
        TrapAnchor = Position;
    }
    LowMotionSeconds += DeltaSeconds;
    if (LowMotionSeconds >= Tuning->TrapWindowSeconds) RecoverBall();
}

void APinballTable::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelActions();
    RemoveBall();
    Super::EndPlay(Reason);
}
