#include "Minigames/AsteroidField/AsteroidProjectile.h"
#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Minigames/AsteroidField/AsteroidObstacle.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// Shots use query sweeps instead of simulating fast tiny rigid bodies.
AAsteroidProjectile::AAsteroidProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shot")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Mesh->SetStaticMesh(Shape.Object); Mesh->SetRelativeScale3D(FVector(.12)); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Multi-object sweep ignores unrelated worlds/actors and awards only the closest owned live target.
void AAsteroidProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = Cast<AAsteroidFieldRuntime>(GetOwner());
    if (!Runtime || !Runtime->Accepts(RunId)) return;
    const FVector Start = GetActorLocation(), End = Start + Velocity * DeltaSeconds;
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AsteroidShot), false, this);
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic), FCollisionShape::MakeSphere(6), Params);
    for (const auto& Hit : Hits)
        if (auto* Rock = Cast<AAsteroidObstacle>(Hit.GetActor()))
            if (Rock->GetOwner() == Runtime && Rock->Hit(RunId)) { Destroy(); return; }
    SetActorLocation(End); Remaining -= DeltaSeconds;
    const FVector Local = Runtime->GetActorTransform().InverseTransformPosition(End);
    if (Remaining <= 0 || FMath::Abs(Local.X) > Runtime->GetContext().ArenaExtent.X || FMath::Abs(Local.Y) > Runtime->GetContext().ArenaExtent.Y) Destroy();
}
