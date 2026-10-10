#include "Minigames/AlienAssault/AssaultProjectile.h"
#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

// Shots use query sweeps instead of simulating fast tiny rigid bodies.
AAssaultProjectile::AAssaultProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shot")); SetRootComponent(Visual);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Shape.Object); Visual->SetRelativeScale3D(FVector(.12)); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Multi-object sweep ignores unrelated worlds/actors and awards only the closest owned live target.
void AAssaultProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = Cast<AAlienAssaultRuntime>(GetOwner());
    if (!Runtime || !Runtime->Accepts(RunId)) return;
    const FVector Start = GetActorLocation(), End = Start + Velocity * DeltaSeconds;
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AssaultShot), false, this);
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(bEnemyShot ? ECC_Pawn : ECC_WorldDynamic), FCollisionShape::MakeSphere(6), Params);
    for (const auto& Hit : Hits)
    {
        if (bEnemyShot && Hit.GetActor() == Runtime->GetRunPawn()) { Runtime->DamageShip(RunId); Destroy(); return; }
        if (bEnemyShot) continue;
        if (auto* Enemy = Cast<AAssaultEnemy>(Hit.GetActor()))
            if (Enemy->GetOwner() == Runtime && Enemy->Hit(RunId)) { Destroy(); return; }
    }
    SetActorLocation(End); Remaining -= DeltaSeconds;
    const FVector Local = Runtime->GetActorTransform().InverseTransformPosition(End);
    if (Remaining <= 0 || FMath::Abs(Local.X) > Runtime->GetContext().ArenaExtent.X || FMath::Abs(Local.Y) > Runtime->GetContext().ArenaExtent.Y) Destroy();
}
