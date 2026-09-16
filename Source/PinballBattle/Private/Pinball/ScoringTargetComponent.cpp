#include "Pinball/ScoringTargetComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/InteractionFeedbackComponent.h"
#include "Data/PinballTuningData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

UScoringTargetComponent::UScoringTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UScoringTargetComponent::Initialize(APinballTable* InTable, UPrimitiveComponent* InSurface)
{
    if (Surface) Surface->OnComponentHit.RemoveDynamic(this, &ThisClass::OnHit);
    Table = InTable;
    Surface = InSurface;
    SourceId = FGuid::NewGuid();
    Episode.Reset();
    Sequence = 0;
    if (!Table || !Surface) return;
    AddTickPrerequisiteActor(Table);
    Surface->SetNotifyRigidBodyCollision(true);
    Surface->SetPhysMaterialOverride(SurfaceMaterial ? SurfaceMaterial.Get() : Table->Tuning->PhysicalMaterial.Get());
    Surface->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void UScoringTargetComponent::OnHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult& Hit)
{
    APinballBall* Ball = Cast<APinballBall>(OtherActor);
    if (!Table || !Table->CanEmitEvent(Ball)) return;
    // Landing on a cap is a passive support collision, not a strike of the target face/ring.
    if (FMath::Abs(FVector::DotProduct(Hit.ImpactNormal, Table->GetTableNormal())) > .75f) return;
    if (Epoch != Table->GetPhysicalEpoch()) { Episode.Reset(); Epoch = Table->GetPhysicalEpoch(); }
    if (!Episode.TryBegin(Table->GetBallHandle().BallId)) return;
    OnQualifiedContact(Ball, Hit);
    const FScoringEvent Event = Table->MakeScoringEvent(SourceId, Category, ++Sequence);
    Table->PublishScoringEvent(Event);
    OnScoringEvent.Broadcast(Event);
    if (Feedback) Feedback->Pulse();
}

void UScoringTargetComponent::OnQualifiedContact(APinballBall*, const FHitResult&) {}

void UScoringTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
    if (!Table || !Surface) return;
    APinballBall* Ball = Table->GetBall();
    if (!Table->CanEmitEvent(Ball)) { Episode.Reset(); return; }
    FVector Closest;
    Episode.ObserveSeparation(Surface->GetClosestPointOnCollision(Ball->GetActorLocation(), Closest),
        Ball->GetBody()->GetScaledSphereRadius(), Table->Tuning->ContactSeparationTolerance);
}

void UScoringTargetComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Surface) Surface->OnComponentHit.RemoveDynamic(this, &ThisClass::OnHit);
    Super::EndPlay(Reason);
}

APinballScoringTarget::APinballScoringTarget()
{
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetSurface"));
    SetRootComponent(Surface);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Surface->SetStaticMesh(Mesh.Object);
    Surface->SetRelativeScale3D(FVector(.65, .24, .55));
    Surface->SetCollisionObjectType(ECC_GameTraceChannel2);
    Surface->SetCollisionResponseToAllChannels(ECR_Ignore);
    Surface->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    Scoring = CreateDefaultSubobject<UScoringTargetComponent>(TEXT("Scoring"));
    Feedback = CreateDefaultSubobject<UInteractionFeedbackComponent>(TEXT("Feedback"));
    Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
    Flash->SetupAttachment(Surface);
    Flash->SetRelativeLocation(FVector(0, 0, 100));
    Flash->SetIntensity(0);
    Flash->SetAttenuationRadius(220);
    Flash->SetCastShadows(false);
}
