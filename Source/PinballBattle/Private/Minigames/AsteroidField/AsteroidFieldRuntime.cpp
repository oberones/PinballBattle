#include "Minigames/AsteroidField/AsteroidFieldRuntime.h"
#include "Minigames/AsteroidField/AsteroidShipPawn.h"
#include "Minigames/AsteroidField/AsteroidObstacle.h"
#include "Minigames/AsteroidField/AsteroidProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Fixed overhead framing uses only local primitives; dormant roots have no lights or audio running.
AAsteroidFieldRuntime::AAsteroidFieldRuntime()
{
    ObstacleClass = AAsteroidObstacle::StaticClass(); ProjectileClass = AAsteroidProjectile::StaticClass();
    Camera->SetProjectionMode(ECameraProjectionMode::Orthographic); Camera->SetOrthoWidth(1900);
    Camera->SetRelativeRotation(FRotator(-90, -90, 0));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Rail = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Boundary%d"), Index));
        Rail->SetupAttachment(RootComponent); Rail->SetStaticMesh(Cube.Object);
        Rail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rail->SetRelativeLocation(Index < 2 ? FVector(Index ? 800 : -800, 0, 0) : FVector(0, Index == 2 ? -430 : 430, 0));
        Rail->SetRelativeScale3D(Index < 2 ? FVector(.12, 8.6, .15) : FVector(16, .12, .15));
    }
}

// Invalid tuning fails initialization through the existing zero-bonus recovery path.
bool AAsteroidFieldRuntime::OnInitializeRun_Implementation(const FMiniGameContext& C)
{
    Lives = C.StartingLives; DestroyedCount = 0; NextSpawn = 0; NextShot = 0; Obstacles.Reset();
    ProtectedUntil = ProtectionSeconds; Random.Initialize(C.Seed);
    if (!Cast<AAsteroidShipPawn>(GetRunPawn()) || Lives != 3 || !ObstacleClass || !ProjectileClass ||
        !FMath::IsFinite(SpawnInterval) || SpawnInterval < .1f || SpawnInterval > 1.f ||
        !FMath::IsFinite(FireInterval) || FireInterval < .05f || FireInterval > .5f ||
        !FMath::IsFinite(ProtectionSeconds) || ProtectionSeconds < 1 || ProtectionSeconds > 5 ||
        LocalPointsPerObject < 0 || LocalPointsPerObject > 10000 ||
        C.ArenaExtent.X < 200 || C.ArenaExtent.Y < 200 || C.MetricBounds.FindRef(TEXT("ObjectsDestroyed")) < 100) return false;
    CastChecked<AAsteroidShipPawn>(GetRunPawn())->ConfigureRun(this);
    CastChecked<AAsteroidShipPawn>(GetRunPawn())->Respawn();
    return true;
}

// An initial twelve targets plus paced replacements makes twenty destructions possible in thirty seconds.
bool AAsteroidFieldRuntime::OnStartRun_Implementation()
{
    for (int32 Index = 0; Index < 12; ++Index) if (!SpawnObstacle()) return false;
    NextSpawn = SpawnInterval; return true;
}

// Cap concurrent targets and catch up bounded spawn scheduling without a separate gameplay clock.
void AAsteroidFieldRuntime::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Accepts(GetContext().RunId)) return;
    Obstacles.RemoveAll([](const AAsteroidObstacle* Rock) { return !IsValid(Rock); });
    if (GetElapsed() >= NextSpawn)
    {
        NextSpawn = GetElapsed() + SpawnInterval;
        if (Obstacles.Num() < 28 && !SpawnObstacle())
            EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed));
    }
}

// Full ownership remains in the runtime registry; local callbacks additionally carry the unique run ID.
bool AAsteroidFieldRuntime::Accepts(const FGuid& RunId) const
{
    return RunId.IsValid() && RunId == GetContext().RunId && GetLifecycle() == EMiniGameLifecycleState::Playing && !GetWorld()->IsPaused();
}

// Spawn along a safe outer ring and drift toward the interior with a small randomized tangent.
bool AAsteroidFieldRuntime::SpawnObstacle()
{
    const double Angle = Random.FRandRange(0.f, 2.f * PI);
    const FVector Local(FMath::Cos(Angle) * (GetContext().ArenaExtent.X - 65), FMath::Sin(Angle) * (GetContext().ArenaExtent.Y - 65), 0);
    auto* Rock = Cast<AAsteroidObstacle>(SpawnRunActor(ObstacleClass, FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Local))));
    if (!Rock) return false;
    Rock->RunId = GetContext().RunId;
    Rock->Velocity = (-Local.GetSafeNormal() * Random.FRandRange(65.f, 110.f));
    Obstacles.Add(Rock); return true;
}

// Both fresh presses and held Enhanced Input fire use this same active-time rate limit.
void AAsteroidFieldRuntime::Fire()
{
    auto* Ship = Cast<AAsteroidShipPawn>(GetRunPawn());
    if (!Ship || !Accepts(GetContext().RunId) || GetElapsed() < NextShot) return;
    NextShot = GetElapsed() + FireInterval;
    auto* Shot = Cast<AAsteroidProjectile>(SpawnRunActor(ProjectileClass, FTransform(Ship->GetActorRotation(), Ship->GetActorLocation() + Ship->GetActorForwardVector() * 36)));
    if (!Shot) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return; }
    Shot->RunId = GetContext().RunId; Shot->Velocity = Ship->GetActorForwardVector() * 1100;
}

// Remove from the live set before feedback so reentrant or duplicate callbacks cannot score again.
bool AAsteroidFieldRuntime::DestroyObstacle(AAsteroidObstacle* Rock, const FGuid& RunId)
{
    if (!Accepts(RunId) || !IsValid(Rock) || Rock->GetOwner() != this || Rock->RunId != RunId || !Obstacles.RemoveSingle(Rock)) return false;
    ++DestroyedCount;
    if (DestroyedCount > GetContext().MaximumObjectives || DestroyedCount > GetContext().MetricBounds.FindRef(TEXT("ObjectsDestroyed")))
    { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return false; }
    Rock->bDestroyed = true; Rock->SetActorEnableCollision(false);
    // A brief enlarged flash retains the destroyed object's location while the metric/HUD changes.
    Rock->Visual->SetRelativeScale3D(FVector(1.2f, 1.2f, .15f)); Rock->SetLifeSpan(.10f);
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    if (DestructionSound) ActiveSound = UGameplayStatics::SpawnSoundAttached(DestructionSound, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true, .4f);
    OnProgress.Broadcast(GetElapsed(), GetLocalScore()); return true;
}

// Life exhaustion retains earned performance; local damage never touches pinball ball accounting.
bool AAsteroidFieldRuntime::DamageShip(const FGuid& RunId)
{
    if (!Accepts(RunId) || IsProtected() || Lives <= 0) return false;
    --Lives;
    if (!Lives)
    {
        auto Result = BuildResult(); Result.EndReason = EMiniGameEndReason::LivesExhausted; Result.bSuccess = false;
        EndMiniGame(Result);
    }
    else { ProtectedUntil = GetElapsed() + ProtectionSeconds; CastChecked<AAsteroidShipPawn>(GetRunPawn())->Respawn(); }
    return true;
}

// Swept actor movement catches contacts; simple arena-plane reflection supplies arcade rebounds.
void AAsteroidFieldRuntime::MoveBounded(AActor* Actor, FVector& Velocity, float Radius, float DeltaSeconds)
{
    if (!Actor || !Accepts(GetContext().RunId)) return;
    FHitResult Hit;
    Actor->AddActorWorldOffset(GetActorTransform().TransformVectorNoScale(Velocity * DeltaSeconds), true, &Hit);
    if (Hit.bBlockingHit)
    {
        const FVector Normal = GetActorTransform().InverseTransformVectorNoScale(Hit.Normal);
        if (FVector::DotProduct(Velocity, Normal) < 0) Velocity = Velocity.MirrorByVector(Normal);
        if ((Actor == GetRunPawn() && Cast<AAsteroidObstacle>(Hit.GetActor())) || Hit.GetActor() == GetRunPawn())
            if (DamageShip(GetContext().RunId) && Actor == GetRunPawn()) return;
    }
    FVector Local = GetActorTransform().InverseTransformPosition(Actor->GetActorLocation());
    for (int32 Axis = 0; Axis < 2; ++Axis)
    {
        const double Limit = GetContext().ArenaExtent[Axis] - Radius;
        if (FMath::Abs(Local[Axis]) >= Limit)
        {
            if (Local[Axis] * Velocity[Axis] > 0) Velocity[Axis] *= -1;
            Local[Axis] = FMath::Clamp(Local[Axis], -Limit, Limit);
        }
    }
    Local.Z = 0; Actor->SetActorLocation(GetActorTransform().TransformPosition(Local));
}

// The central profile converts metrics; raw local points are purely informational.
FMiniGameResult AAsteroidFieldRuntime::BuildResult_Implementation() const
{
    auto Result = FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::TimedOut);
    Result.DurationSeconds = GetElapsed(); Result.bSuccess = Lives > 0;
    Result.RawScore = int64(DestroyedCount) * LocalPointsPerObject; Result.ObjectivesCompleted = DestroyedCount;
    Result.Metrics.FindOrAdd(TEXT("ObjectsDestroyed")) = DestroyedCount; return Result;
}

// Common Space routing has already consumed the instruction confirmation press.
void AAsteroidFieldRuntime::OnAction_Implementation() { Fire(); }

// Audio is stopped explicitly because actor visibility and tick do not silence it.
void AAsteroidFieldRuntime::OnCleanupRun_Implementation()
{
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    ActiveSound = nullptr; Obstacles.Reset(); Lives = 0; DestroyedCount = 0;
}

// Project lives, progress and protection alongside the base HUD's authoritative active clock.
FText AAsteroidFieldRuntime::GetLocalStatus() const
{
    return FText::FromString(FString::Printf(TEXT("Lives: %d   Objects: %d%s\nLEFT/RIGHT: rotate   UP: thrust   SPACE: fire   ESCAPE: pause"), Lives, DestroyedCount, IsProtected() ? TEXT("   SHIELD") : TEXT("")));
}
