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

// Capture the authored neutral pose before enabling the constrained body.
void APinballFlipper::Configure(UPinballTuningData* Tuning)
{
    NeutralTransform = Body->GetComponentTransform();
    HalfTravel = Tuning->FlipperTravelDegrees / 2.f;
    Body->SetPhysMaterialOverride(Tuning->PhysicalMaterial);
    Body->SetMassOverrideInKg(NAME_None, Tuning->FlipperMass, true);
    Body->SetSimulatePhysics(true);
    Hinge->SetAngularTwistLimit(ACM_Limited, HalfTravel);
    Hinge->SetAngularDriveParams(Tuning->FlipperStrength, Tuning->FlipperDamping, Tuning->FlipperMaxTorque);
    Hinge->SetConstrainedComponents(nullptr, NAME_None, Body, NAME_None);
    SetHeld(false);
}

// Cancellation during suspension must not wake or restart a constrained body.
void APinballFlipper::SetHeld(bool bInHeld)
{
    bHeld = bInHeld;
    const float Side = bReverseDrive ? -1.f : 1.f;
    Hinge->SetAngularOrientationTarget(FRotator(0, 0, Side * (bHeld ? -HalfTravel : HalfTravel)));
    if (!bSuspended) Body->WakeAllRigidBodies();
}

// Stop both orientation and angular velocity drives while the body's snapshot stays intact.
bool APinballFlipper::SuspendForMinigame(int64 Generation)
{
    bSuspended = true; SetHeld(false); Hinge->SetOrientationDriveTwistAndSwing(false, false);
    Hinge->SetAngularVelocityDriveTwistAndSwing(false, false); return Generation > 0;
}
// Clear stale held torque and return the body to its authored pose before simulation can resume.
bool APinballFlipper::PrepareRestore(int64 Generation)
{
    SetHeld(false); Body->SetWorldTransform(NeutralTransform, false, nullptr, ETeleportType::TeleportPhysics);
    return bSuspended && Generation > 0;
}
// Restore only the neutral motor target after all table participants have acknowledged readiness.
void APinballFlipper::CommitRestore(int64 Generation)
{
    bSuspended = false; Hinge->SetOrientationDriveTwistAndSwing(true, false);
    Hinge->SetAngularVelocityDriveTwistAndSwing(true, false); SetHeld(false);
}
