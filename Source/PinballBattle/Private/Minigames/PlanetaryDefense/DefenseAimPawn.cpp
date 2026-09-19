#include "Minigames/PlanetaryDefense/DefenseAimPawn.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Framework/PinballPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// Four short bars make the precise target readable without hiding an incoming threat.
ADefenseAimPawn::ADefenseAimPawn()
{
    PrimaryActorTick.bCanEverTick = true; SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("AimRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Bar = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Reticle%d"), Index));
        Bar->SetupAttachment(RootComponent); Bar->SetStaticMesh(Cube.Object); Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bar->SetRelativeLocation(Index < 2 ? FVector(Index ? 16 : -16, 0, 40) : FVector(0, Index == 2 ? -16 : 16, 40));
        Bar->SetRelativeScale3D(Index < 2 ? FVector(.12, .035, .025) : FVector(.035, .12, .025)); Reticle.Add(Bar);
    }
}

// Polling Enhanced Input values avoids latched fire flags when the context is removed for pause.
void ADefenseAimPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    if (auto* Enhanced = Cast<UEnhancedInputComponent>(Input)) if (FireAction) Enhanced->BindActionValue(FireAction);
}

// Mouse motion requires a live run; firing additionally requires the controller's fresh-key barrier.
void ADefenseAimPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = RunRuntime.Get(); if (!Runtime || !Runtime->Accepts(RunId)) return;
    UpdateMouseAim();
    auto* Player = Cast<APinballPlayerController>(GetController());
    auto* Input = Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
    if (Input && Player->IsMiniGameInputReady() && FireAction && Input->GetActionValue(FireAction).Get<bool>()) Runtime->FireAt(LocalAim);
}

// Possession changes Actor Owner, so the run reference and identity are retained explicitly.
void ADefenseAimPawn::ConfigureRun(APlanetaryDefenseRuntime* Runtime)
{
    RunRuntime = Runtime; RunId = Runtime ? Runtime->GetContext().RunId : FGuid(); LocalAim = FVector(0, 100, 0); PresentAim();
}

// World projection initializes a usable cursor even if entry came from a menu outside the play area.
void ADefenseAimPawn::InitializeCursor()
{
    auto* Runtime = RunRuntime.Get(); auto* Player = Cast<APlayerController>(GetController());
    if (!Runtime || !Player) return;
    FVector2D Screen;
    if (Player->ProjectWorldLocationToScreen(Runtime->GetActorTransform().TransformPosition(LocalAim), Screen))
        Player->SetMouseLocation(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
    PresentAim();
}

// Failure (for example focus loss) retains the previous valid target instead of jumping to world zero.
bool ADefenseAimPawn::UpdateMouseAim()
{
    auto* Runtime = RunRuntime.Get(); auto* Player = Cast<APlayerController>(GetController());
    if (!Runtime || !Player || !Runtime->Accepts(RunId)) return false;
    FVector Origin, Direction;
    return Player->DeprojectMousePositionToWorld(Origin, Direction) && AimFromRay(Origin, Direction);
}

// Intersect in local space so translated or rotated arena slots and different aspect ratios work alike.
bool ADefenseAimPawn::AimFromRay(const FVector& Origin, const FVector& Direction)
{
    const auto* Runtime = RunRuntime.Get();
    if (!Runtime || Origin.ContainsNaN() || Direction.ContainsNaN()) return false;
    const FTransform Transform = Runtime->GetActorTransform();
    const FVector LocalOrigin = Transform.InverseTransformPosition(Origin), LocalDirection = Transform.InverseTransformVectorNoScale(Direction);
    if (FMath::Abs(LocalDirection.Z) < UE_SMALL_NUMBER) return false;
    const double Distance = -LocalOrigin.Z / LocalDirection.Z;
    if (!FMath::IsFinite(Distance) || Distance < 0) return false;
    LocalAim = Runtime->ClampAim(LocalOrigin + LocalDirection * Distance); PresentAim(); return true;
}

// The crosshair and turret always point to the same destination that the next shot will capture.
void ADefenseAimPawn::PresentAim()
{
    if (auto* Runtime = RunRuntime.Get())
    {
        SetActorLocation(Runtime->GetActorTransform().TransformPosition(LocalAim)); SetActorRotation(Runtime->GetActorRotation());
        const FVector Direction = (LocalAim - FVector(0, -380, 0)).GetSafeNormal();
        Runtime->Barrel->SetRelativeLocation(FVector(0, -380, 15) + Direction * 28);
        Runtime->Barrel->SetRelativeRotation(Direction.Rotation());
    }
}
