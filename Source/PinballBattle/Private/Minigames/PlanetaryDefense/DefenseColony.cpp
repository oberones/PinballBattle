#include "Minigames/PlanetaryDefense/DefenseColony.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Colony collision is an explicit small sphere and never blocks friendly interceptors.
ADefenseColony::ADefenseColony()
{
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("ColonyBody")); SetRootComponent(Body); Body->InitSphereRadius(44);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_GameTraceChannel4); Body->SetCollisionResponseToAllChannels(ECR_Overlap);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Colony")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Sphere.Object); Visual->SetRelativeScale3D(FVector(1.1, .8, .5)); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// The runtime owns colony accounting and validates both objects against this run.
bool ADefenseColony::Impact(ADefenseThreat* Threat, const FGuid& SourceRunId)
{
    auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner()); return Runtime && bAlive && Runtime->ImpactColony(Threat, this, SourceRunId);
}

// Flattened remains distinguish a lost colony from an intact dome through results presentation.
void ADefenseColony::Lose()
{
    if (!bAlive) return; bAlive = false; Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(.8, .18, .07)); Visual->SetRelativeRotation(FRotator(0, 25, 0));
}
