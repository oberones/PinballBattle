#include "Pinball/BumperResponseComponent.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballTable.h"
#include "Framework/PinballGameModeBase.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PinballTuningData.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Pinball/InteractionFeedbackComponent.h"
#include "Components/PointLightComponent.h"

UBumperResponseComponent::UBumperResponseComponent()
{
    Category = EScoringCategory::Bumper;
}

void UBumperResponseComponent::ResetForNewSession()
{
    bHasImpulse = false;
    LastImpulseTime = 0;
}

void UBumperResponseComponent::OnQualifiedContact(APinballBall* Ball, const FHitResult& Hit)
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (bHasImpulse && Now - LastImpulseTime < Table->Tuning->BumperCooldown) return;
    LastImpulseTime = Now;
    bHasImpulse = true;
    const FVector Direction = FVector::VectorPlaneProject(
        Ball->GetActorLocation() - Surface->GetComponentLocation(), Table->GetTableNormal()).GetSafeNormal();
    Ball->AddBoundedImpulse(Direction * Table->Tuning->BumperImpulse);
}

APinballBumper::APinballBumper()
{
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BumperSurface"));
    SetRootComponent(Surface);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Surface->SetStaticMesh(Mesh.Object);
    Surface->SetRelativeScale3D(FVector(.7, .7, .5));
    Surface->SetCollisionObjectType(ECC_GameTraceChannel2);
    Surface->SetCollisionResponseToAllChannels(ECR_Ignore);
    Surface->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    Surface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Response = CreateDefaultSubobject<UBumperResponseComponent>(TEXT("BumperResponse"));
    Feedback = CreateDefaultSubobject<UInteractionFeedbackComponent>(TEXT("Feedback"));
    Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
    Flash->SetupAttachment(Surface);
    Flash->SetRelativeLocation(FVector(0, 0, 110));
    Flash->SetIntensity(0);
    Flash->SetAttenuationRadius(240);
    Flash->SetCastShadows(false);
}
