#include "Minigames/PlanetaryDefense/DefenseInterceptor.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// Fast projectiles use sweeps, without rigid-body motion or friendly collision responses.
ADefenseInterceptor::ADefenseInterceptor()
{
    PrimaryActorTick.bCanEverTick = true; Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Interceptor")); SetRootComponent(Visual);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Sphere.Object); Visual->SetRelativeScale3D(FVector(.28, .1, .1)); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Clamping distance prevents overshoot; one latch precedes both the interception and detonation callbacks.
void ADefenseInterceptor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner());
    if (!Runtime || !Runtime->Accepts(RunId) || bDetonated) return;
    const FVector Start = GetActorLocation(), Offset = Destination - Start;
    const double Step = FMath::Max(0.f, DeltaSeconds) * Speed, Distance = Offset.Size();
    FVector End = Start + Offset.GetSafeNormal() * FMath::Min(Distance, Step);
    SetActorRotation(Offset.Rotation());
    TArray<FHitResult> Hits; FCollisionQueryParams Params(SCENE_QUERY_STAT(DefenseInterceptor), false, this);
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_GameTraceChannel4), FCollisionShape::MakeSphere(7), Params);
    Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
    for (const auto& Hit : Hits)
        if (auto* Threat = Cast<ADefenseThreat>(Hit.GetActor()))
            if (Threat->GetOwner() == Runtime && Threat->Intercept(RunId))
            { End = FMath::Lerp(Start, End, Hit.Time); bDetonated = true; break; }
    if (Distance <= Step + UE_SMALL_NUMBER) bDetonated = true;
    SetActorLocation(End);
    if (bDetonated) { Runtime->Detonate(End, RunId); Destroy(); }
}
