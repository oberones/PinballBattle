#include "Minigames/AlienAssault/AssaultShipPawn.h"
#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Engine/World.h"
#include "Framework/PinballPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// The sphere supplies collision while a flattened cone indicates the ship's firing direction.
AAssaultShipPawn::AAssaultShipPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body")); SetRootComponent(Body); Body->InitSphereRadius(20);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_Pawn);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ship")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Visual->SetStaticMesh(Cone.Object); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeRotation(FRotator(-90, 90, 0)); Visual->SetRelativeScale3D(FVector(.35, .35, .65));
}

// Action-value bindings allow polling without storing stale held flags across pause/context removal.
void AAssaultShipPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    if (auto* Enhanced = Cast<UEnhancedInputComponent>(Input))
        for (const auto* Action : {LeftAction.Get(), RightAction.Get(), FireAction.Get()})
            if (Action) Enhanced->BindActionValue(Action);
}

// Horizontal movement stays on the bottom rail and requires fresh gameplay input.
void AAssaultShipPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = RunRuntime.Get();
    if (!Runtime || !Runtime->Accepts(RunId)) return;
    auto* Player = Cast<APinballPlayerController>(GetController());
    auto* Input = Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
    if (Input && Player->IsMiniGameInputReady())
    {
        const float Move = (RightAction && Input->GetActionValue(RightAction).Get<bool>() ? 1.f : 0.f) -
            (LeftAction && Input->GetActionValue(LeftAction).Get<bool>() ? 1.f : 0.f);
        FVector Local = Runtime->GetActorTransform().InverseTransformPosition(GetActorLocation());
        const double Limit = Runtime->GetContext().ArenaExtent.X - 24;
        // The overhead +90 yaw camera projects local -X toward screen right.
        Local.X = FMath::Clamp(Local.X - Move * MovementSpeed * DeltaSeconds, -Limit, Limit);
        Local.Y = -Runtime->GetContext().ArenaExtent.Y + 60; Local.Z = 0;
        const FVector End = Runtime->GetActorTransform().TransformPosition(Local);
        TArray<FHitResult> Hits;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(AssaultShipContact), false, this);
        GetWorld()->SweepMultiByObjectType(Hits, GetActorLocation(), End, FQuat::Identity,
            FCollisionObjectQueryParams(ECC_WorldDynamic), FCollisionShape::MakeSphere(20), Params);
        bool bRespawned = false;
        for (const auto& Hit : Hits)
            if (auto* Enemy = Cast<AAssaultEnemy>(Hit.GetActor()))
                if (Enemy->GetOwner() == Runtime && !Enemy->bDestroyed && Enemy->RunId == RunId)
                    { bRespawned = Runtime->DamageShip(RunId); break; }
        if (!bRespawned) SetActorLocation(End);
        if (FireAction && Input->GetActionValue(FireAction).Get<bool>()) Runtime->Fire();
    }
    Visual->SetVisibility(!Runtime->IsProtected() || FMath::Fmod(Runtime->GetElapsed(), .2) < .14);
}

// A respawn keeps possession and input bindings, restoring the centered bottom spawn.
void AAssaultShipPawn::Respawn()
{
    if (const auto* Runtime = RunRuntime.Get())
    {
        SetActorTransform(Runtime->GetActorTransform());
        SetActorLocation(Runtime->GetActorTransform().TransformPosition(FVector(0, -Runtime->GetContext().ArenaExtent.Y + 60, 0)));
    }
    Visual->SetVisibility(true);
}

// APawn ownership changes on possession, so keep the immutable run link independent of Actor Owner.
void AAssaultShipPawn::ConfigureRun(AAlienAssaultRuntime* InRuntime)
{
    RunRuntime = InRuntime; RunId = InRuntime ? InRuntime->GetContext().RunId : FGuid();
}
