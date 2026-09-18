#include "Minigames/AsteroidField/AsteroidShipPawn.h"
#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Framework/PinballPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// The sphere supplies collision while a flattened cone indicates the ship's firing direction.
AAsteroidShipPawn::AAsteroidShipPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body")); SetRootComponent(Body); Body->InitSphereRadius(20);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_Pawn);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ship")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Visual->SetStaticMesh(Cone.Object); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeRotation(FRotator(-90, 0, 0)); Visual->SetRelativeScale3D(FVector(.35, .35, .65));
}

// Action-value bindings allow polling without storing stale held flags across pause/context removal.
void AAsteroidShipPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    if (auto* Enhanced = Cast<UEnhancedInputComponent>(Input))
        for (const auto* Action : {LeftAction.Get(), RightAction.Get(), ThrustAction.Get(), FireAction.Get()})
            if (Action) Enhanced->BindActionValue(Action);
}

// Acceleration is planar and speed bounded; commands require both active lifecycle and fresh input.
void AAsteroidShipPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = RunRuntime.Get();
    if (!Runtime || !Runtime->Accepts(RunId)) return;
    auto* Player = Cast<APinballPlayerController>(GetController());
    auto* Input = Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
    if (Input && Player->IsMiniGameInputReady())
    {
        const float Turn = (RightAction && Input->GetActionValue(RightAction).Get<bool>() ? 1.f : 0.f) -
            (LeftAction && Input->GetActionValue(LeftAction).Get<bool>() ? 1.f : 0.f);
        // Positive Unreal yaw turns clockwise in the arena camera: Right adds yaw, Left subtracts it.
        AddActorLocalRotation(FRotator(0, Turn * TurnSpeed * DeltaSeconds, 0));
        if (ThrustAction && Input->GetActionValue(ThrustAction).Get<bool>())
            Velocity += Runtime->GetActorTransform().InverseTransformVectorNoScale(GetActorForwardVector()) * Acceleration * DeltaSeconds;
        if (FireAction && Input->GetActionValue(FireAction).Get<bool>()) Runtime->Fire();
    }
    Velocity = Velocity.GetClampedToMaxSize(MaximumSpeed);
    Runtime->MoveBounded(this, Velocity, 20, DeltaSeconds);
    Visual->SetVisibility(!Runtime->IsProtected() || FMath::Fmod(Runtime->GetElapsed(), .2) < .14);
}

// A respawn keeps the same pawn and input bindings, discarding momentum and restoring a visible ship.
void AAsteroidShipPawn::Respawn()
{
    Velocity = FVector::ZeroVector;
    if (const auto* Runtime = RunRuntime.Get()) SetActorTransform(Runtime->GetActorTransform());
    Visual->SetVisibility(true);
}

// APawn ownership changes on possession, so keep the immutable run link independent of Actor Owner.
void AAsteroidShipPawn::ConfigureRun(AAsteroidFieldRuntime* InRuntime)
{
    RunRuntime = InRuntime; RunId = InRuntime ? InRuntime->GetContext().RunId : FGuid();
}
