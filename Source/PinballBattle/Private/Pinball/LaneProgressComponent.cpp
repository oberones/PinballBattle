#include "Pinball/LaneProgressComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/InteractionFeedbackComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

ULaneProgressComponent::ULaneProgressComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void ULaneProgressComponent::Initialize(APinballTable* InTable, UBoxComponent* InCorridor)
{
    Table = InTable;
    Corridor = InCorridor;
    SourceId = FGuid::NewGuid();
    Sequence = 0;
    bHasPrevious = false;
    Traversal.Reset();
    if (Table) AddTickPrerequisiteActor(Table);
}

void ULaneProgressComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
    if (!Table || !Corridor) return;
    const APinballBall* Ball = Table->GetBall();
    if (!Table->CanEmitEvent(Ball)) { Traversal.Reset(); bHasPrevious = false; return; }
    const FVector Current = Corridor->GetComponentTransform().InverseTransformPositionNoScale(Ball->GetActorLocation());
    if (Epoch != Table->GetPhysicalEpoch() || BallId != Table->GetBallHandle().BallId)
    {
        Traversal.Reset();
        bHasPrevious = false;
        Epoch = Table->GetPhysicalEpoch();
        BallId = Table->GetBallHandle().BallId;
    }
    if (bHasPrevious && Traversal.Advance(Previous, Current, Corridor->GetScaledBoxExtent(), Ball->GetBody()->GetScaledSphereRadius()))
    {
        const FScoringEvent Event = Table->MakeScoringEvent(SourceId, EScoringCategory::Lane, ++Sequence);
        Table->PublishScoringEvent(Event);
        OnScoringEvent.Broadcast(Event);
        if (Feedback) Feedback->Pulse();
    }
    Previous = Current;
    bHasPrevious = true;
}

APinballLane::APinballLane()
{
    Corridor = CreateDefaultSubobject<UBoxComponent>(TEXT("Corridor"));
    SetRootComponent(Corridor);
    Corridor->InitBoxExtent(FVector(45, 90, 40));
    Corridor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Progress = CreateDefaultSubobject<ULaneProgressComponent>(TEXT("LaneProgress"));
    Insert = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaneInsert"));
    Insert->SetupAttachment(Corridor);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Insert->SetStaticMesh(Mesh.Object);
    Insert->SetRelativeScale3D(FVector(.75, 1.8, .01));
    Insert->SetRelativeLocation(FVector(0, 0, -13));
    Insert->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Feedback = CreateDefaultSubobject<UInteractionFeedbackComponent>(TEXT("Feedback"));
    Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
    Flash->SetupAttachment(Corridor);
    Flash->SetRelativeLocation(FVector(0, 0, 50));
    Flash->SetIntensity(0);
    Flash->SetAttenuationRadius(230);
    Flash->SetCastShadows(false);
}
