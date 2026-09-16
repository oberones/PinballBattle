#include "Pinball/PinballFlipper.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Data/PinballTuningData.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

APinballFlipper::APinballFlipper()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Pivot")));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlipperBody"));
    Body->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Mesh.Object);
    Body->SetRelativeLocation(FVector(55, 0, 0));
    Body->SetRelativeScale3D(FVector(1.1f, .24f, .2f));
    Body->SetCollisionObjectType(ECC_GameTraceChannel2);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetEnableGravity(false);
    Body->SetUseCCD(true);
    Hinge = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Hinge"));
    Hinge->SetupAttachment(RootComponent);
    // Constraint X is the table normal: twist is the one permitted hinge axis.
    Hinge->SetRelativeRotation(FRotator(90, 0, 0));
    Hinge->SetLinearXLimit(LCM_Locked, 0);
    Hinge->SetLinearYLimit(LCM_Locked, 0);
    Hinge->SetLinearZLimit(LCM_Locked, 0);
    Hinge->SetAngularSwing1Limit(ACM_Locked, 0);
    Hinge->SetAngularSwing2Limit(ACM_Locked, 0);
    Hinge->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
    Hinge->SetOrientationDriveTwistAndSwing(true, false);
    Hinge->SetAngularVelocityDriveTwistAndSwing(true, false);
    Hinge->SetDisableCollision(true);
}

void APinballFlipper::Configure(UPinballTuningData* Tuning)
{
    HalfTravel = Tuning->FlipperTravelDegrees / 2.f;
    Body->SetPhysMaterialOverride(Tuning->PhysicalMaterial);
    Body->SetMassOverrideInKg(NAME_None, Tuning->FlipperMass, true);
    Body->SetSimulatePhysics(true);
    Hinge->SetAngularTwistLimit(ACM_Limited, HalfTravel);
    Hinge->SetAngularDriveParams(Tuning->FlipperStrength, Tuning->FlipperDamping, Tuning->FlipperMaxTorque);
    Hinge->SetConstrainedComponents(nullptr, NAME_None, Body, NAME_None);
    SetHeld(false);
}

void APinballFlipper::SetHeld(bool bInHeld)
{
    bHeld = bInHeld;
    const float Side = bReverseDrive ? -1.f : 1.f;
    Hinge->SetAngularOrientationTarget(FRotator(0, 0, Side * (bHeld ? -HalfTravel : HalfTravel)));
    Body->WakeAllRigidBodies();
}
