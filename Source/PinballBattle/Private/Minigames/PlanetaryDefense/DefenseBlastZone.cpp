#include "Minigames/PlanetaryDefense/DefenseBlastZone.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "UObject/ConstructorHelpers.h"

// The disk sits below targets; its query sphere supplies continuous threat sweeps across the plane.
ADefenseBlastZone::ADefenseBlastZone()
{
    PrimaryActorTick.bCanEverTick = true; Body = CreateDefaultSubobject<USphereComponent>(TEXT("BlastBody")); SetRootComponent(Body);
    Body->InitSphereRadius(1); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_GameTraceChannel4); Body->SetCollisionResponseToAllChannels(ECR_Overlap);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Blast")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Sphere.Object); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision); Visual->SetRelativeLocation(FVector(0, 0, -15));
}

// Configure before the first world tick so initial overlaps and visible radius agree immediately.
void ADefenseBlastZone::Configure(const FGuid& InRunId, float Radius, float Lifetime)
{
    RunId = InRunId; MaximumRadius = Radius; Remaining = Duration = Lifetime;
    Body->SetSphereRadius(Radius); Visual->SetRelativeScale3D(FVector(Radius / 50, Radius / 50, .04)); InterceptOverlaps();
}

// Shrink visual and collision together near expiry; expired zones can never accept another hit.
void ADefenseBlastZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner());
    if (!Runtime || !Runtime->Accepts(RunId) || !IsActive()) return;
    Remaining = FMath::Max(0.f, Remaining - FMath::Max(0.f, DeltaSeconds));
    if (!IsActive()) { Body->SetCollisionEnabled(ECollisionEnabled::NoCollision); Destroy(); return; }
    const float Radius = MaximumRadius * FMath::Clamp(Remaining / (Duration * .25f), 0.f, 1.f);
    Body->SetSphereRadius(Radius); Visual->SetRelativeScale3D(FVector(Radius / 50, Radius / 50, .04)); InterceptOverlaps();
}

// Only the owning run's live threats are eligible, regardless of how many zones overlap them.
void ADefenseBlastZone::InterceptOverlaps()
{
    auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner()); if (!Runtime || !Runtime->Accepts(RunId) || !IsActive()) return;
    TArray<FOverlapResult> Hits; FCollisionQueryParams Params(SCENE_QUERY_STAT(DefenseBlast), false, this);
    GetWorld()->OverlapMultiByObjectType(Hits, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_GameTraceChannel4), FCollisionShape::MakeSphere(Body->GetUnscaledSphereRadius()), Params);
    for (const auto& Hit : Hits) if (auto* Threat = Cast<ADefenseThreat>(Hit.GetActor())) if (Threat->GetOwner() == Runtime) Threat->Intercept(RunId);
}
