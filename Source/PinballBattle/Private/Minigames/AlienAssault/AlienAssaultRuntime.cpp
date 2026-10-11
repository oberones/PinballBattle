#include "Minigames/AlienAssault/AlienAssaultRuntime.h"
#include "Minigames/AlienAssault/AssaultShipPawn.h"
#include "Minigames/AlienAssault/AssaultEnemy.h"
#include "Minigames/AlienAssault/AssaultProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Fixed overhead framing uses only local primitives; dormant roots have no lights or audio running.
AAlienAssaultRuntime::AAlienAssaultRuntime()
{
    EnemyClass = AAssaultEnemy::StaticClass(); ProjectileClass = AAssaultProjectile::StaticClass();
    Camera->SetProjectionMode(ECameraProjectionMode::Orthographic); Camera->SetOrthoWidth(1900);
    Camera->SetRelativeRotation(FRotator(-90, 90, 0));
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
bool AAlienAssaultRuntime::OnInitializeRun_Implementation(const FMiniGameContext& C)
{
    Lives = C.StartingLives; DestroyedCount = 0; NextShot = 0; Enemies.Reset();
    ProtectedUntil = ProtectionSeconds; RespawnAt.Init(-1., 12);
    if (!Cast<AAssaultShipPawn>(GetRunPawn()) ||
        !FMath::IsFinite(Cast<AAssaultShipPawn>(GetRunPawn())->MovementSpeed) ||
        Cast<AAssaultShipPawn>(GetRunPawn())->MovementSpeed <= 0 || Lives != 3 || !EnemyClass || !ProjectileClass ||
        !FMath::IsFinite(EnemyFireInterval) || EnemyFireInterval < 1.f || EnemyFireInterval > 5.f ||
        LocalPointsPerSecond < 0 || LocalPointsPerSecond > 1000 ||
        !FMath::IsFinite(FireInterval) || FireInterval < .05f || FireInterval > .5f ||
        !FMath::IsFinite(ProtectionSeconds) || ProtectionSeconds < 1 || ProtectionSeconds > 5 ||
        LocalPointsPerObject < 0 || LocalPointsPerObject > 10000 ||
        C.ArenaExtent.X < 700 || C.ArenaExtent.Y < 400 || C.MaximumObjectives < 120 ||
        C.MetricBounds.FindRef(TEXT("EnemiesDestroyed")) < 120 || C.MetricBounds.FindRef(TEXT("SurvivalSeconds")) < C.DurationLimit) return false;
    CastChecked<AAssaultShipPawn>(GetRunPawn())->ConfigureRun(this);
    CastChecked<AAssaultShipPawn>(GetRunPawn())->Respawn();
    return true;
}

// Start with twelve occupied slots; only accepted kills schedule replacements.
bool AAlienAssaultRuntime::OnStartRun_Implementation()
{
    for (int32 Slot = 0; Slot < 12; ++Slot) if (!SpawnEnemy(Slot)) return false;
    return true;
}

// Active-time deadlines freeze on pause and cannot spawn after the shared terminal latch.
void AAlienAssaultRuntime::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Accepts(GetContext().RunId)) return;
    for (int32 Slot = 0; Slot < RespawnAt.Num(); ++Slot)
    {
        if (RespawnAt[Slot] < 0 || GetElapsed() < RespawnAt[Slot]) continue;
        RespawnAt[Slot] = -1;
        if (!SpawnEnemy(Slot))
        { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return; }
    }
}

// Full ownership remains in the runtime registry; local callbacks additionally carry the unique run ID.
bool AAlienAssaultRuntime::Accepts(const FGuid& RunId) const
{
    return RunId.IsValid() && RunId == GetContext().RunId && GetLifecycle() == EMiniGameLifecycleState::Playing && !GetWorld()->IsPaused();
}

// New occupants join the slot's current position and phase, rather than restarting its motion.
bool AAlienAssaultRuntime::SpawnEnemy(int32 SlotIndex)
{
    const FVector Anchor(-500 + (SlotIndex % 6) * 200, 120 + (SlotIndex / 6) * 160, 0);
    const FVector Local = Anchor + FVector(130 * FMath::Sin(GetElapsed() * 1.5), -GetElapsed() * 14, 0);
    auto* Enemy = Cast<AAssaultEnemy>(SpawnRunActor(EnemyClass, FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Local))));
    if (!Enemy) return false;
    Enemy->RunId = GetContext().RunId; Enemy->FormationPosition = Anchor; Enemy->SlotIndex = SlotIndex;
    Enemy->NextShot = GetElapsed() + .8 + SlotIndex * .16;
    Enemies.Add(Enemy);
    return true;
}

// Formation motion is deterministic active-time motion; contact still traverses a swept segment.
void AAlienAssaultRuntime::MoveEnemy(AAssaultEnemy* Enemy, float DeltaSeconds)
{
    if (!Enemy || !Accepts(Enemy->RunId) || Enemy->bDestroyed || !Enemies.Contains(Enemy)) return;
    const double Age = GetElapsed();
    FVector Local = Enemy->FormationPosition;
    Local.X += 130 * FMath::Sin(Age * 1.5);
    Local.Y -= Age * 14;
    const FVector End = GetActorTransform().TransformPosition(Local);
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AssaultContact), false, Enemy);
    GetWorld()->SweepMultiByObjectType(Hits, Enemy->GetActorLocation(), End, FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(32), Params);
    for (const auto& Hit : Hits) if (Hit.GetActor() == GetRunPawn()) { DamageShip(Enemy->RunId); break; }
    if (Accepts(Enemy->RunId)) Enemy->SetActorLocation(End);
}

// Enemy ammunition uses the same owned projectile type with an explicit team and captured direction.
void AAlienAssaultRuntime::EnemyFire(AAssaultEnemy* Enemy)
{
    if (!Enemy || !Accepts(Enemy->RunId) || Enemy->bDestroyed || !Enemies.Contains(Enemy) || GetElapsed() < Enemy->NextShot) return;
    Enemy->NextShot = GetElapsed() + EnemyFireInterval;
    const FVector Start = Enemy->GetActorLocation();
    auto* Shot = Cast<AAssaultProjectile>(SpawnRunActor(ProjectileClass, FTransform(GetActorRotation(), Start)));
    if (!Shot) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return; }
    Shot->RunId = GetContext().RunId; Shot->bEnemyShot = true;
    Shot->Velocity = (GetRunPawn()->GetActorLocation() - Start).GetSafeNormal() * 260;
    Shot->Visual->SetRelativeScale3D(FVector(.20, .20, .12));
    Shot->Visual->SetMaterial(0, Enemy->Visual->GetMaterial(0));
}

// Both fresh presses and held Enhanced Input fire use this same active-time rate limit.
void AAlienAssaultRuntime::Fire()
{
    auto* Ship = Cast<AAssaultShipPawn>(GetRunPawn());
    if (!Ship || !Accepts(GetContext().RunId) || GetElapsed() < NextShot) return;
    NextShot = GetElapsed() + FireInterval;
    auto* Shot = Cast<AAssaultProjectile>(SpawnRunActor(ProjectileClass, FTransform(Ship->GetActorRotation(), Ship->GetActorLocation() + GetActorTransform().TransformVectorNoScale(FVector(0, 36, 0)))));
    if (!Shot) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return; }
    Shot->RunId = GetContext().RunId; Shot->Velocity = GetActorTransform().TransformVectorNoScale(FVector(0, 1100, 0));
}

// Remove from the live set before feedback so reentrant or duplicate callbacks cannot score again.
bool AAlienAssaultRuntime::DestroyEnemy(AAssaultEnemy* Enemy, const FGuid& RunId)
{
    if (!Accepts(RunId) || !IsValid(Enemy) || Enemy->GetOwner() != this || Enemy->RunId != RunId || !Enemies.RemoveSingle(Enemy)) return false;
    RespawnAt[Enemy->SlotIndex] = GetElapsed() + 3.;
    ++DestroyedCount;
    if (DestroyedCount > GetContext().MaximumObjectives || DestroyedCount > GetContext().MetricBounds.FindRef(TEXT("EnemiesDestroyed")))
    { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return false; }
    Enemy->bDestroyed = true; Enemy->SetActorEnableCollision(false);
    // A brief enlarged flash retains the destroyed object's location while the metric/HUD changes.
    Enemy->Visual->SetRelativeScale3D(FVector(1.2f, 1.2f, .15f)); Enemy->SetLifeSpan(.10f);
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    if (DestructionSound) ActiveSound = UGameplayStatics::SpawnSoundAttached(DestructionSound, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true, .4f);
    OnProgress.Broadcast(GetElapsed(), GetLocalScore()); return true;
}

// Life exhaustion retains earned performance; local damage never touches pinball ball accounting.
bool AAlienAssaultRuntime::DamageShip(const FGuid& RunId)
{
    if (!Accepts(RunId) || IsProtected() || Lives <= 0) return false;
    --Lives;
    if (!Lives)
    {
        auto Result = BuildResult(); Result.EndReason = EMiniGameEndReason::LivesExhausted; Result.bSuccess = false;
        EndMiniGame(Result);
    }
    else { ProtectedUntil = GetElapsed() + ProtectionSeconds; CastChecked<AAssaultShipPawn>(GetRunPawn())->Respawn(); }
    return true;
}

// The central profile converts metrics; raw local points are purely informational.
FMiniGameResult AAlienAssaultRuntime::BuildResult_Implementation() const
{
    auto Result = FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::TimedOut);
    Result.DurationSeconds = GetElapsed(); Result.bSuccess = Lives > 0;
    Result.RawScore = int64(DestroyedCount) * LocalPointsPerObject +
        ((GetLifecycle() == EMiniGameLifecycleState::Ended || GetElapsed() >= GetContext().DurationLimit || Lives == 0) ? int64(FMath::FloorToDouble(GetElapsed())) * LocalPointsPerSecond : 0); Result.ObjectivesCompleted = DestroyedCount;
    Result.Metrics.FindOrAdd(TEXT("EnemiesDestroyed")) = DestroyedCount;
    Result.Metrics.FindOrAdd(TEXT("SurvivalSeconds")) = GetElapsed(); return Result;
}

// Common Space routing has already consumed the instruction confirmation press.
void AAlienAssaultRuntime::OnAction_Implementation() { Fire(); }

// Audio is stopped explicitly because actor visibility and tick do not silence it.
void AAlienAssaultRuntime::OnCleanupRun_Implementation()
{
    if (IsValid(ActiveSound)) ActiveSound->Stop();
    ActiveSound = nullptr; Enemies.Reset(); RespawnAt.Reset(); Lives = 0; DestroyedCount = 0;
}

// Project lives, progress and protection alongside the base HUD's authoritative active clock.
FText AAlienAssaultRuntime::GetLocalStatus() const
{
    return FText::FromString(FString::Printf(TEXT("Lives: %d   Enemies: %d%s\nLEFT/RIGHT: move\nSPACE: fire   ESCAPE: pause"), Lives, DestroyedCount, IsProtected() ? TEXT("   SHIELD") : TEXT("")));
}
