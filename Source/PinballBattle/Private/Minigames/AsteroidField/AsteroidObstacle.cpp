#include "Minigames/AsteroidField/AsteroidObstacle.h"
#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Rocks block the ship but ignore each other, avoiding stationary collision piles at arena edges.
AAsteroidObstacle::AAsteroidObstacle()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body")); SetRootComponent(Body); Body->InitSphereRadius(32);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_WorldDynamic);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Shape.Object); Visual->SetRelativeScale3D(FVector(.64, .64, .5));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Destroyed targets remain briefly as a visual pulse but never move or accept more hits.
void AAsteroidObstacle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* Runtime = Cast<AAsteroidFieldRuntime>(GetOwner());
    if (!bDestroyed && Runtime && Runtime->Accepts(RunId)) Runtime->MoveBounded(this, Velocity, 32, DeltaSeconds);
}

// The runtime's live-set removal is the authoritative deduplication boundary.
bool AAsteroidObstacle::Hit(const FGuid& ShotRun)
{
    auto* Runtime = Cast<AAsteroidFieldRuntime>(GetOwner());
    if (bDestroyed || ShotRun != RunId || !Runtime) return false;
    return Runtime->DestroyObstacle(this, ShotRun);
}
