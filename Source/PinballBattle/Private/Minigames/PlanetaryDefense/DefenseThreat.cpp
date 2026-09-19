#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseBlastZone.h"
#include "Minigames/PlanetaryDefense/DefenseColony.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// A query sphere provides reliable sweeps while the orange cone communicates its travel direction.
ADefenseThreat::ADefenseThreat()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("ThreatBody")); SetRootComponent(Body); Body->InitSphereRadius(13);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionObjectType(ECC_GameTraceChannel4);
    Body->SetCollisionResponseToAllChannels(ECR_Overlap);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Threat")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Visual->SetStaticMesh(Cone.Object); Visual->SetRelativeScale3D(FVector(.22, .22, .55));
    Visual->SetRelativeRotation(FRotator(-90, 0, 0)); Visual->SetRelativeLocation(FVector(0, 0, 20)); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Sweeping the entire step catches narrow blasts even at low frame rates; the first accepted contact wins.
void ADefenseThreat::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner());
    if (!Runtime || !Runtime->Accepts(RunId)) return;
    if (bResolved) { FlashRemaining -= DeltaSeconds; if (FlashRemaining <= 0) Destroy(); return; }
    const FVector Start = GetActorLocation(), End = Start + Velocity * FMath::Max(0.f, DeltaSeconds);
    TArray<FHitResult> Hits; FCollisionQueryParams Params(SCENE_QUERY_STAT(DefenseThreat), false, this);
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_GameTraceChannel4), FCollisionShape::MakeSphere(13), Params);
    Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
    for (const auto& Hit : Hits)
    {
        if (auto* Blast = Cast<ADefenseBlastZone>(Hit.GetActor()))
            if (Blast->GetOwner() == Runtime && Blast->RunId == RunId && Blast->IsActive())
            { SetActorLocation(FMath::Lerp(Start, End, Hit.Time)); if (Intercept(RunId)) return; }
        if (auto* Colony = Cast<ADefenseColony>(Hit.GetActor())) if (Colony->Impact(this, RunId)) return;
    }
    SetActorLocation(End);
    if (Runtime->GetActorTransform().InverseTransformPosition(End).Y < -Runtime->GetContext().ArenaExtent.Y) Resolve(false);
}

// All destruction paths share the runtime's live-set gate, including simultaneous blast overlaps.
bool ADefenseThreat::Intercept(const FGuid& SourceRunId)
{
    auto* Runtime = Cast<APlanetaryDefenseRuntime>(GetOwner()); return Runtime && !bResolved && Runtime->InterceptThreat(this, SourceRunId);
}

// Feedback lifetime uses active tick time, so pause and terminal freezing affect it exactly like gameplay.
void ADefenseThreat::Resolve(bool Intercepted)
{
    if (bResolved) return; bResolved = true; Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (!Intercepted) { Destroy(); return; }
    FlashRemaining = .15f; Visual->SetRelativeScale3D(FVector(.9, .9, .15)); Visual->SetRelativeRotation(FRotator::ZeroRotator);
}
