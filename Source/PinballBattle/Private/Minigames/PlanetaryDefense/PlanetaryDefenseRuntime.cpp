#include "Minigames/PlanetaryDefense/PlanetaryDefenseRuntime.h"
#include "Minigames/PlanetaryDefense/DefenseAimPawn.h"
#include "Minigames/PlanetaryDefense/DefenseThreat.h"
#include "Minigames/PlanetaryDefense/DefenseColony.h"
#include "Minigames/PlanetaryDefense/DefenseInterceptor.h"
#include "Minigames/PlanetaryDefense/DefenseBlastZone.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Local primitives and explicit anchors remain hidden with the dormant resident root.
APlanetaryDefenseRuntime::APlanetaryDefenseRuntime()
{
    ThreatClass = ADefenseThreat::StaticClass(); ColonyClass = ADefenseColony::StaticClass();
    InterceptorClass = ADefenseInterceptor::StaticClass(); BlastClass = ADefenseBlastZone::StaticClass();
    Camera->SetProjectionMode(ECameraProjectionMode::Orthographic); Camera->SetOrthoWidth(2400);
    Camera->SetRelativeLocation(FVector(350, 0, 1200));
    Camera->SetRelativeRotation(FRotator(-90, 90, 0));
    Camera->bOverrideAspectRatioAxisConstraint = true;
    Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
    Camera->SetAutoCalculateOrthoPlanes(false); Camera->SetUpdateOrthoPlanes(false);
    // Unreal recenters orthographic views on the target; include both sides of the local plane.
    Camera->SetOrthoNearClipPlane(-1500); Camera->SetOrthoFarClipPlane(3000);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Launcher = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Launcher")); Launcher->SetupAttachment(RootComponent);
    Launcher->SetStaticMesh(Sphere.Object); Launcher->SetRelativeLocation(FVector(0, -380, 0));
    Launcher->SetRelativeScale3D(FVector(.65, .65, .3)); Launcher->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel")); Barrel->SetupAttachment(RootComponent);
    Barrel->SetStaticMesh(Cube.Object); Barrel->SetRelativeLocation(FVector(0, -360, 15));
    Barrel->SetRelativeScale3D(FVector(.75, .12, .12)); Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        auto* Anchor = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("ColonyAnchor%d"), Index));
        Anchor->SetupAttachment(RootComponent); Anchor->SetRelativeLocation(FVector((Index - 1) * 550, -300, 0));
        ColonyAnchors.Add(Anchor);
    }
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Rail = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Boundary%d"), Index));
        Rail->SetupAttachment(RootComponent); Rail->SetStaticMesh(Cube.Object); Rail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Rail->SetRelativeLocation(Index < 2 ? FVector(Index ? 800 : -800, 0, -20) : FVector(0, Index == 2 ? -430 : 430, -20));
        Rail->SetRelativeScale3D(Index < 2 ? FVector(.1, 8.6, .12) : FVector(16, .1, .12));
    }
}

// Initialization validates every gameplay divisor/range and allocates only registered dormant actors.
bool APlanetaryDefenseRuntime::OnInitializeRun_Implementation(const FMiniGameContext& C)
{
    Threats.Reset(); Colonies.Reset(); StopAudio(); DestroyedCount = SpawnedCount = ShotsFired = 0;
    NextSpawn = NextShot = 0; Random.Initialize(C.Seed);
    auto* Aim = Cast<ADefenseAimPawn>(GetRunPawn());
    if (!Aim || !ThreatClass || !ColonyClass || !InterceptorClass || !BlastClass || ColonyAnchors.Num() != 3 ||
        !FMath::IsFinite(SpawnInterval) || SpawnInterval < .3f || SpawnInterval > .8f ||
        !FMath::IsFinite(FireInterval) || FireInterval < .1f || FireInterval > .5f ||
        !FMath::IsFinite(ThreatSpeed) || ThreatSpeed < 60 || ThreatSpeed > 180 ||
        !FMath::IsFinite(InterceptorSpeed) || InterceptorSpeed < 600 || InterceptorSpeed > 2500 ||
        !FMath::IsFinite(BlastRadius) || BlastRadius < 60 || BlastRadius > 200 ||
        !FMath::IsFinite(BlastLifetime) || BlastLifetime < .5f || BlastLifetime > 3 ||
        LocalPointsPerThreat < 0 || LocalPointsPerThreat > 10000 || LocalPointsPerSurvivor < 0 || LocalPointsPerSurvivor > 10000 ||
        C.ArenaExtent.X < 800 || C.ArenaExtent.Y < 430 || C.MaximumObjectives < 100 ||
        C.MetricBounds.FindRef(TEXT("ThreatsDestroyed")) < 100 || C.MetricBounds.FindRef(TEXT("StructuresSurviving")) != 3) return false;
    Aim->ConfigureRun(this);
    for (USceneComponent* Anchor : ColonyAnchors)
    {
        if (!IsValid(Anchor)) return false;
        auto* Colony = Cast<ADefenseColony>(SpawnRunActor(ColonyClass, Anchor->GetComponentTransform()));
        if (!Colony) return false;
        Colony->RunId = C.RunId; Colonies.Add(Colony);
    }
    return true;
}

// Camera readiness precedes Start; a fresh confirmation never passes through to FireAt.
bool APlanetaryDefenseRuntime::OnStartRun_Implementation()
{
    CastChecked<ADefenseAimPawn>(GetRunPawn())->InitializeCursor();
    for (int32 Index = 0; Index < 3; ++Index) if (!SpawnThreat()) return false;
    NextSpawn = SpawnInterval; return true;
}

// Active-time scheduling does not stop when colonies reach zero and cannot spawn after terminal admission.
void APlanetaryDefenseRuntime::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Accepts(GetContext().RunId)) { StopAudio(); return; }
    Threats.RemoveAll([](const ADefenseThreat* Threat) { return !IsValid(Threat) || Threat->IsResolved(); });
    Sounds.RemoveAll([](const UAudioComponent* Sound) { return !IsValid(Sound) || !Sound->IsPlaying(); });
    while (GetElapsed() >= NextSpawn && SpawnedCount < 100)
    {
        NextSpawn += SpawnInterval;
        if (!SpawnThreat()) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return; }
    }
}

// A unique run and live lifecycle are required even for callbacks from still-valid old actors.
bool APlanetaryDefenseRuntime::Accepts(const FGuid& RunId) const
{
    return RunId.IsValid() && RunId == GetContext().RunId && GetLifecycle() == EMiniGameLifecycleState::Playing && !GetWorld()->IsPaused();
}

// The target band excludes colony bodies so aiming down cannot waste a shot inside the launcher.
FVector APlanetaryDefenseRuntime::ClampAim(const FVector& Local) const
{
    if (Local.ContainsNaN()) return FVector(0, 100, 0);
    return FVector(FMath::Clamp(Local.X, -GetContext().ArenaExtent.X + 30, GetContext().ArenaExtent.X - 30),
        FMath::Clamp(Local.Y, -GetContext().ArenaExtent.Y + 200, GetContext().ArenaExtent.Y - 30), 0);
}

// A deterministic spread alternates colony lanes while remaining distinct across seeded runs.
bool APlanetaryDefenseRuntime::SpawnThreat()
{
    const int32 TargetIndex = SpawnedCount % 3;
    const FVector Target = GetActorTransform().InverseTransformPosition(ColonyAnchors[TargetIndex]->GetComponentLocation());
    const FVector Start(FMath::Clamp(Target.X + Random.FRandRange(-260.f, 260.f), -750., 750.), 390, 0);
    auto* Threat = Cast<ADefenseThreat>(SpawnRunActor(ThreatClass, FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Start))));
    if (!Threat) return false;
    Threat->RunId = GetContext().RunId; Threat->Velocity = GetActorTransform().TransformVectorNoScale((Target - Start).GetSafeNormal() * ThreatSpeed);
    Threat->SetActorRotation(Threat->Velocity.Rotation()); Threats.Add(Threat); ++SpawnedCount; return true;
}

// Shots capture the clamped destination and share a rate limit for press and held-action paths.
bool APlanetaryDefenseRuntime::FireAt(const FVector& LocalAim)
{
    if (!Accepts(GetContext().RunId) || LocalAim.ContainsNaN() || GetElapsed() + UE_SMALL_NUMBER < NextShot) return false;
    auto* Shot = Cast<ADefenseInterceptor>(SpawnRunActor(InterceptorClass, FTransform(GetActorRotation(), GetLauncherLocation())));
    if (!Shot) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return false; }
    NextShot = GetElapsed() + FireInterval; ++ShotsFired;
    Shot->RunId = GetContext().RunId; Shot->Speed = InterceptorSpeed;
    Shot->Destination = GetActorTransform().TransformPosition(ClampAim(LocalAim));
    return true;
}

// Removal is the atomic credit boundary shared by projectile sweeps and all overlapping blasts.
bool APlanetaryDefenseRuntime::InterceptThreat(ADefenseThreat* Threat, const FGuid& RunId)
{
    if (!Accepts(RunId) || !IsValid(Threat) || Threat->GetOwner() != this || Threat->RunId != RunId ||
        Threat->IsResolved() || !Threats.RemoveSingle(Threat)) return false;
    ++DestroyedCount; Threat->Resolve(true);
    if (InterceptionSound)
        if (auto* Sound = UGameplayStatics::SpawnSoundAttached(InterceptionSound, RootComponent, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true, .3f)) Sounds.Add(Sound);
    OnProgress.Broadcast(GetElapsed(), GetLocalScore()); return true;
}

// A spent threat cannot destroy multiple colonies, and an impact never counts as an interception.
bool APlanetaryDefenseRuntime::ImpactColony(ADefenseThreat* Threat, ADefenseColony* Colony, const FGuid& RunId)
{
    if (!Accepts(RunId) || !IsValid(Threat) || !IsValid(Colony) || Threat->GetOwner() != this || Colony->GetOwner() != this ||
        Threat->RunId != RunId || Colony->RunId != RunId || Threat->IsResolved() || !Colony->IsAlive() ||
        !Colonies.Contains(Colony) || !Threats.RemoveSingle(Threat)) return false;
    Colony->Lose(); Threat->Resolve(false); OnProgress.Broadcast(GetElapsed(), GetLocalScore()); return true;
}

// Registry ownership guarantees that even a just-created blast is reclaimed on rollback or return.
bool APlanetaryDefenseRuntime::Detonate(const FVector& WorldLocation, const FGuid& RunId)
{
    if (!Accepts(RunId) || WorldLocation.ContainsNaN()) return false;
    auto* Blast = Cast<ADefenseBlastZone>(SpawnRunActor(BlastClass, FTransform(GetActorRotation(), WorldLocation)));
    if (!Blast) { EndMiniGame(FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::RuntimeFailed)); return false; }
    Blast->Configure(RunId, BlastRadius, BlastLifetime); return true;
}

// All shots use this explicit component rather than a named world-actor lookup.
FVector APlanetaryDefenseRuntime::GetLauncherLocation() const { return Launcher->GetComponentLocation(); }

// Colonies are local objectives, not pinball balls or shooter lives.
int32 APlanetaryDefenseRuntime::GetSurvivorCount() const
{
    int32 Count = 0; for (const ADefenseColony* Colony : Colonies) if (IsValid(Colony) && Colony->IsAlive()) ++Count; return Count;
}

// Survival points appear at the end; the central profile independently converts both metrics to bonus.
FMiniGameResult APlanetaryDefenseRuntime::BuildResult_Implementation() const
{
    auto Result = FMiniGameResult::Failure(GetContext(), EMiniGameEndReason::TimedOut);
    Result.DurationSeconds = GetElapsed(); Result.bSuccess = GetSurvivorCount() > 0;
    Result.ObjectivesCompleted = DestroyedCount; Result.RawScore = int64(DestroyedCount) * LocalPointsPerThreat;
    if (GetElapsed() >= GetContext().DurationLimit) Result.RawScore += int64(GetSurvivorCount()) * LocalPointsPerSurvivor;
    Result.Metrics.FindOrAdd(TEXT("ThreatsDestroyed")) = DestroyedCount;
    Result.Metrics.FindOrAdd(TEXT("StructuresSurviving")) = GetSurvivorCount(); return Result;
}

// Refresh deprojection here too so a press cannot use a target from the preceding frame.
void APlanetaryDefenseRuntime::OnAction_Implementation()
{
    if (auto* Aim = Cast<ADefenseAimPawn>(GetRunPawn())) { Aim->UpdateMouseAim(); FireAt(Aim->GetLocalAim()); }
}

// Sound components are separately owned resources and must not outlive the active arena.
void APlanetaryDefenseRuntime::StopAudio()
{
    for (UAudioComponent* Sound : Sounds) if (IsValid(Sound)) Sound->Stop(); Sounds.Reset();
}

// Idempotent cleanup leaves no local metrics or effects to contaminate a replay.
void APlanetaryDefenseRuntime::OnCleanupRun_Implementation()
{
    StopAudio(); Threats.Reset(); Colonies.Reset(); DestroyedCount = SpawnedCount = ShotsFired = 0;
}

// HUD text remains a projection; losing the last colony explicitly invites continued interception.
FText APlanetaryDefenseRuntime::GetLocalStatus() const
{
    return FText::FromString(FString::Printf(TEXT("Colonies: %d / 3    Interceptions: %d%s\nMOUSE: aim    Hold SPACE: launch    ESCAPE: pause"),
        GetSurvivorCount(), DestroyedCount, GetSurvivorCount() ? TEXT("") : TEXT("    Keep intercepting!")));
}
