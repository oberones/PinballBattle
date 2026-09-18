#include "Tests/MiniGameTestRuntime.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// Native primitive visuals make the test arena useful without a dependency on any real minigame.
AMiniGameTestRuntime::AMiniGameTestRuntime()
{
    auto* Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaFloor")); Floor->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Floor->SetStaticMesh(Mesh.Object); Floor->SetRelativeScale3D(FVector(8, 8, .1));
    Floor->SetRelativeLocation(FVector(0, 0, -30)); Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
// This hook may fail after the base spawned the pawn, exercising partial initialization cleanup.
bool AMiniGameTestRuntime::OnInitializeRun_Implementation(const FMiniGameContext& RunContext) { Actions = 0; return !bFailInitialize; }
// A test-only flag injects failure through the shipping guarded Start path.
bool AMiniGameTestRuntime::OnStartRun_Implementation() { return !bFailStart; }
// Actions remain local and bounded by the authored result schema.
void AMiniGameTestRuntime::OnAction_Implementation()
{
    Actions = FMath::Min(Actions + 1, 100);
    if (GetRunPawn()) if (auto* Light = GetRunPawn()->FindComponentByClass<UPointLightComponent>())
        Light->SetIntensity(Actions % 2 ? 6000.f : 2000.f);
}
// The common timeout supplies the final duration and rating before publishing this result.
FMiniGameResult AMiniGameTestRuntime::BuildResult_Implementation() const
{
    auto R = FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::TimedOut);
    R.Metrics.FindOrAdd(TEXT("Actions")) = Actions; R.RawScore = Actions * 100; R.ObjectivesCompleted = Actions;
    R.bSuccess = true; return R;
}
// The fixture pawn has no autonomous movement or BeginPlay input behavior.
AMiniGameTestPawn::AMiniGameTestPawn()
{
    auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PawnVisual")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Mesh->SetStaticMesh(Shape.Object); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight")); Light->SetupAttachment(Mesh);
    Light->SetRelativeLocation(FVector(0, 0, 140)); Light->SetAttenuationRadius(400);
    Light->SetLightColor(FLinearColor(.1f, .8f, 1.f)); Light->SetIntensity(0); Light->SetCastShadows(false);
}
