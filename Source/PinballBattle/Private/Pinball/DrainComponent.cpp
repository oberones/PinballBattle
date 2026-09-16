#include "Pinball/DrainComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Framework/PinballGameModeBase.h"
#include "Engine/World.h"

UDrainComponent::UDrainComponent()
{
    InitBoxExtent(FVector(300, 40, 80));
    SetCollisionObjectType(ECC_GameTraceChannel3);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetGenerateOverlapEvents(true);
    OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnEnter);
}

void UDrainComponent::OnEnter(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    APinballBall* Ball = Cast<APinballBall>(OtherActor);
    if (Table && Table->CanEmitEvent(Ball))
        if (APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>())
        {
            FDrainEvent Event;
            Event.SessionId = Table->GetBallHandle().SessionId;
            Event.BallId = Table->GetBallHandle().BallId;
            Event.EventId = FGuid::NewGuid();
            Event.SourceId = SourceId;
            Event.PhaseEpoch = Table->GetPhysicalEpoch();
            Mode->RequestDrainEvent(Event);
        }
}

APinballDrain::APinballDrain()
{
    Drain = CreateDefaultSubobject<UDrainComponent>(TEXT("Drain"));
    SetRootComponent(Drain);
}
