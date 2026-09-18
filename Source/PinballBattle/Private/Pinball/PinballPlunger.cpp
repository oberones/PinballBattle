#include "Pinball/PinballPlunger.h"
#include "Data/PinballTuningData.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// Suspension is cancellation, never release-fire.
bool APinballPlunger::SuspendForMinigame(int64 Generation) { CancelActions(); return Generation > 0; }
// A fresh post-return press is required to begin another charge.
bool APinballPlunger::PrepareRestore(int64 Generation) { CancelActions(); return Generation > 0; }

APinballPlunger::APinballPlunger()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlungerVisual"));
    Visual->SetupAttachment(RootComponent);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Visual->SetStaticMesh(Mesh.Object);
    Visual->SetRelativeScale3D(FVector(.45, .3, .2));
}

void APinballPlunger::BeginCharge()
{
    if (!Tuning || bCharging) return;
    ChargeSeconds = 0.f;
    bCharging = true;
}

float APinballPlunger::ReleaseCharge()
{
    const float Impulse = bCharging && Tuning ? Tuning->LaunchImpulse(ChargeSeconds) : 0.f;
    CancelActions();
    return Impulse;
}

void APinballPlunger::CancelActions()
{
    bCharging = false;
    ChargeSeconds = 0.f;
    Visual->SetRelativeLocation(FVector::ZeroVector);
}

float APinballPlunger::GetChargeAlpha() const
{
    return Tuning ? FMath::Clamp(ChargeSeconds / Tuning->MaxChargeSeconds, 0.f, 1.f) : 0.f;
}

void APinballPlunger::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bCharging && Tuning)
    {
        ChargeSeconds = FMath::Min(ChargeSeconds + DeltaSeconds, Tuning->MaxChargeSeconds);
        Visual->SetRelativeLocation(FVector(-35.f * GetChargeAlpha(), 0, 0));
    }
}
