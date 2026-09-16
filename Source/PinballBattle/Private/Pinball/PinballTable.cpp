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

APinballTable::APinballTable()
{
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
    for (APinballBumper* Bumper : Bumpers)
    {
        if (!IsValid(Bumper)) { OutError = TEXT("Missing explicit bumper reference."); return false; }
    }
    LeftFlipper->Configure(Tuning);
    RightFlipper->Configure(Tuning);
    Session->RegisterBody(LeftFlipper->GetBody());
    Session->RegisterBody(RightFlipper->GetBody());
    Plunger->Configure(Tuning);
    for (APinballBumper* Bumper : Bumpers) Bumper->Response->Initialize(this, Bumper->Surface);
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
    return true;
}

void APinballTable::RemoveBall()
{
    APinballBall* OldBall = CurrentBall;
    CurrentBall = nullptr;
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

bool APinballTable::IsCurrentBall(const APinballBall* Ball) const
{
    return IsValid(Ball) && Ball == CurrentBall;
}

void APinballTable::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelActions();
    RemoveBall();
    Super::EndPlay(Reason);
}
