#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Query collision lets both ship contacts and player projectile sweeps recognize this enemy.
AAssaultEnemy::AAssaultEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body")); SetRootComponent(Body); Body->InitSphereRadius(32);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_WorldDynamic);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Saucer")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Shape.Object); Visual->SetRelativeScale3D(FVector(.8, .5, .25));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Destroyed targets remain briefly as a visual pulse but never move or accept more hits.
void AAssaultEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = Cast<AAlienAssaultRuntime>(GetOwner());
    if (!bDestroyed && Runtime && Runtime->Accepts(RunId)) { Runtime->MoveEnemy(this, DeltaSeconds); Runtime->EnemyFire(this); }
}

// The runtime's live-set removal is the authoritative deduplication boundary.
bool AAssaultEnemy::Hit(const FGuid& ShotRun)
{
    auto* Runtime = Cast<AAlienAssaultRuntime>(GetOwner());
    if (bDestroyed || ShotRun != RunId || !Runtime) return false;
    return Runtime->DestroyEnemy(this, ShotRun);
}
