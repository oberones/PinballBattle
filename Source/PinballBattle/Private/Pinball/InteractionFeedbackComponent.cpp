#include "Pinball/InteractionFeedbackComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"

UInteractionFeedbackComponent::UInteractionFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionFeedbackComponent::Initialize(UPointLightComponent* InLight)
{
    Light = InLight;
    if (Light) { Light->SetIntensity(0); Light->SetLightColor(Color); }
}

void UInteractionFeedbackComponent::Pulse()
{
    Remaining = FMath::Clamp(FlashSeconds, .05f, .5f);
    if (Light) Light->SetIntensity(6000.f);
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    if (HitSound) ActiveSound = UGameplayStatics::SpawnSoundAtLocation(this, HitSound, GetOwner()->GetActorLocation(), FRotator::ZeroRotator, .35f);
}

void UInteractionFeedbackComponent::ResetFeedback()
{
    Remaining = 0;
    if (Light) Light->SetIntensity(0);
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    ActiveSound = nullptr;
}

void UInteractionFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
    Remaining = FMath::Max(0.f, Remaining - DeltaTime);
    if (Light) Light->SetIntensity(6000.f * Remaining / FMath::Clamp(FlashSeconds, .05f, .5f));
}
