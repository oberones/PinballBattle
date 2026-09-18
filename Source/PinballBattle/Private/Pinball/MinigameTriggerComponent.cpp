#include "Pinball/MinigameTriggerComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Framework/GameFlowComponent.h"
#include "Framework/PinballGameModeBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// Post-physics sampling separates actual occupancy from setup-induced overlap notifications.
UMinigameTriggerComponent::UMinigameTriggerComponent()
{ PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickGroup = TG_PostPhysics; }
// Configuration is explicit on the placed objective, not discovered by actor names.
void UMinigameTriggerComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (!Table || !Definition || !Volume || ObjectiveId.IsNone() || !Mode ||
        !Mode->FindComponentByClass<UGameFlowComponent>()->RegisterObjective(this)) SetComponentTickEnabled(false);
}
// Departure is sampled even while the table is secured, but only playable entry can request flow.
void UMinigameTriggerComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt, TickType, Tick);
    if (!Table || !Volume || !Table->GetBall()) return;
    const FVector Local = Volume->GetComponentTransform().InverseTransformPosition(Table->GetBall()->GetActorLocation());
    const bool Inside = FBox(-Volume->GetUnscaledBoxExtent(), Volume->GetUnscaledBoxExtent()).IsInsideOrOn(Local);
    if (!Inside)
    {
        bArmed = true;
        if (bWasInside) if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>())
            Mode->FindComponentByClass<UGameFlowComponent>()->NotifyObjectiveExit(this);
    }
    if (Inside && !bWasInside && bArmed && Table->CanEmitEvent(Table->GetBall()))
        if (auto* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>())
            Mode->FindComponentByClass<UGameFlowComponent>()->RequestObjective(this, Table->GetBallHandle().SessionId,
                Table->GetBallHandle().BallId, FGuid::NewGuid());
    bWasInside = Inside;
}
// The insert is visual only; its box participates in safe-return overlap exclusion.
AMinigameObjective::AMinigameObjective()
{
    Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("ObjectiveVolume")); SetRootComponent(Volume);
    Volume->InitBoxExtent(FVector(45, 45, 35)); Volume->SetCollisionObjectType(ECC_GameTraceChannel3);
    Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger = CreateDefaultSubobject<UMinigameTriggerComponent>(TEXT("ObjectiveTrigger")); Trigger->Volume = Volume;
    auto* Insert = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Insert")); Insert->SetupAttachment(Volume);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Insert->SetStaticMesh(Mesh.Object); Insert->SetRelativeScale3D(FVector(.85, .85, .02));
    Insert->SetRelativeLocation(FVector(0, 0, -12)); Insert->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
