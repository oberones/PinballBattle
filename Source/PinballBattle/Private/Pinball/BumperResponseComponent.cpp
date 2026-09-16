#include "Pinball/BumperResponseComponent.h"
#include "Pinball/PinballBall.h"
#include "Pinball/PinballTable.h"
#include "Framework/PinballGameModeBase.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PinballTuningData.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

void UBumperResponseComponent::Initialize(APinballTable* InTable, UPrimitiveComponent* InSurface)
{
    if (Surface) Surface->OnComponentHit.RemoveDynamic(this, &ThisClass::OnHit);
    Table = InTable;
    Surface = InSurface;
    Surface->SetNotifyRigidBodyCollision(true);
    Surface->SetPhysMaterialOverride(Table->Tuning->PhysicalMaterial);
    Surface->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void UBumperResponseComponent::OnHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult&)
{
    APinballBall* Ball = Cast<APinballBall>(OtherActor);
    const APinballGameModeBase* Mode = GetWorld()->GetAuthGameMode<APinballGameModeBase>();
    if (!Table || !Mode || !Mode->CanPlay() || !Table->IsCurrentBall(Ball) || !Ball->IsLaunched()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpulseTime < Table->Tuning->BumperCooldown) return;
    LastImpulseTime = Now;
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
}
