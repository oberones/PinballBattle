#include "Pinball/PinballBall.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PinballTuningData.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

APinballBall::APinballBall()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("BallBody"));
    SetRootComponent(Body);
    Body->InitSphereRadius(12.f);
    Body->SetCollisionObjectType(ECC_GameTraceChannel1);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
    Body->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetGenerateOverlapEvents(true);
    Body->SetNotifyRigidBodyCollision(true);
    Body->SetUseCCD(true);
    Body->OnComponentHit.AddDynamic(this, &ThisClass::OnBodyHit);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallVisual"));
    Visual->SetupAttachment(Body);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Mesh.Object);
    Visual->SetRelativeScale3D(FVector(.24f));
}

void APinballBall::Configure(UPinballTuningData* InTuning)
{
    check(InTuning);
    Tuning = InTuning;
    Body->SetSphereRadius(Tuning->BallRadius * Tuning->TableScale);
    Visual->SetRelativeScale3D(FVector(Body->GetUnscaledSphereRadius() / 50.f));
    Body->SetMassOverrideInKg(NAME_None, Tuning->BallMass, true);
    Body->SetPhysMaterialOverride(Tuning->PhysicalMaterial);
    Body->SetLinearDamping(Tuning->LinearDamping);
    Body->SetAngularDamping(.08f);
    Body->SetSimulatePhysics(false);
}

bool APinballBall::Launch(const FVector& Direction, float Impulse)
{
    if (!Tuning || bLaunched || bDrained || Direction.ContainsNaN() || Direction.IsNearlyZero() ||
        !FMath::IsFinite(Impulse) || Impulse <= 0.f) return false;
    bLaunched = true;
    Body->SetSimulatePhysics(true);
    Body->SetEnableGravity(true);
    Body->WakeAllRigidBodies();
    AddBoundedImpulse(Direction.GetSafeNormal() * Impulse);
    return true;
}

void APinballBall::AddBoundedImpulse(const FVector& Impulse)
{
    if (!Tuning || !IsLaunched() || Impulse.ContainsNaN()) return;
    const FVector Velocity = Body->GetPhysicsLinearVelocity() + Impulse / Tuning->BallMass;
    Body->SetPhysicsLinearVelocity(Velocity.GetClampedToMaxSize(Tuning->MaxBallSpeed));
}

bool APinballBall::MarkDrained()
{
    if (!IsLaunched()) return false;
    bDrained = true;
    Body->SetSimulatePhysics(false);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    return true;
}

void APinballBall::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Tuning && IsLaunched())
    {
        const FVector Velocity = Body->GetPhysicsLinearVelocity();
        if (Velocity.SizeSquared() > FMath::Square(Tuning->MaxBallSpeed))
            Body->SetPhysicsLinearVelocity(Velocity.GetClampedToMaxSize(Tuning->MaxBallSpeed));
    }
}

void APinballBall::OnBodyHit(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult&)
{
    if (IsLaunched()) ++ContactCount;
}
