#include "Minigames/Shared/MiniGameRuntimeBase.h"
#include "Minigames/Shared/MinigameWorldSubsystem.h"
#include "Data/ScoringProfile.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Roots remain resident but dormant; only Initialize may allocate run resources.
AMiniGameRuntimeBase::AMiniGameRuntimeBase()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.bStartWithTickEnabled = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot")));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
    Camera->SetupAttachment(RootComponent); Camera->SetRelativeLocation(FVector(0, 0, 1200));
    Camera->SetRelativeRotation(FRotator(-90, 0, 0));
}

// Registration carries the actual streamed level pointer, never an actor-name lookup.
void AMiniGameRuntimeBase::BeginPlay()
{
    Super::BeginPlay(); SetActorTickEnabled(false); SetActorHiddenInGame(true);
    GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()->RegisterRoot(this);
}

// World teardown cleans only owned resources and never attempts controller restoration.
void AMiniGameRuntimeBase::EndPlay(const EEndPlayReason::Type Reason)
{
    Cleanup();
    if (auto* Subsystem = GetWorld()->GetSubsystem<UMinigameWorldSubsystem>()) Subsystem->UnregisterRoot(this);
    Super::EndPlay(Reason);
}

// Enter Initialized before fallible content setup so partial initialization remains an owned run.
bool AMiniGameRuntimeBase::Initialize(const FMiniGameContext& C)
{
    if (Lifecycle != EMiniGameLifecycleState::Dormant || !C.IsValid() || !PawnClass) return false;
    Context = C; Elapsed = 0; bPresentationReady = false; Lifecycle = EMiniGameLifecycleState::Initialized;
    RunPawn = Cast<APawn>(SpawnRunActor(PawnClass, GetActorTransform()));
    if (!RunPawn || !OnInitializeRun(C) || !HasPresentationTargets()) return false;
    SetActorHiddenInGame(false);
    return true;
}

// Explicit targets must be in the same streamed instance and world as the registered root.
bool AMiniGameRuntimeBase::HasPresentationTargets() const
{
    return IsValid(RunPawn) && IsValid(Camera) && Camera->GetOwner() == this &&
        RunPawn->GetLevel() == GetLevel() && RunPawn->GetWorld() == GetWorld();
}

// Acknowledge presentation only before the run starts, never after an end or stale cleanup.
bool AMiniGameRuntimeBase::SetPresentationReady(bool Ready)
{
    if (Lifecycle != EMiniGameLifecycleState::Initialized || !HasPresentationTargets()) return false;
    bPresentationReady = Ready; return true;
}

// A start hook cannot publish gameplay before the common guard has opened the run.
bool AMiniGameRuntimeBase::StartMiniGame()
{
    if (Lifecycle != EMiniGameLifecycleState::Initialized || !bPresentationReady || !HasPresentationTargets()) return false;
    Lifecycle = EMiniGameLifecycleState::Playing;
    if (!OnStartRun()) { Lifecycle = EMiniGameLifecycleState::Initialized; StopProducers(); return false; }
    SetActorTickEnabled(true);
    for (auto Handle : DeferredActiveTimers) GetWorldTimerManager().UnPauseTimer(Handle);
    DeferredActiveTimers.Reset();
    for (AActor* Actor : RunActors) if (IsValid(Actor)) { Actor->SetActorTickEnabled(true); Actor->SetActorEnableCollision(true); }
    return true;
}

// Active world time automatically stops under native pause; no real-time timer owns gameplay.
void AMiniGameRuntimeBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Lifecycle != EMiniGameLifecycleState::Playing || GetWorld()->IsPaused()) return;
    Elapsed = FMath::Min(Context.DurationLimit, Elapsed + FMath::Max(0.f, DeltaSeconds));
    OnProgress.Broadcast(Elapsed, BuildResult().RawScore);
    if (Elapsed >= Context.DurationLimit)
    {
        FMiniGameResult R = BuildResult();
        R.SessionId = Context.SessionId; R.RunId = Context.RunId; R.MiniGameId = Context.MiniGameId;
        R.Generation = Context.Generation; R.DurationSeconds = Elapsed; R.EndReason = EMiniGameEndReason::TimedOut;
        EndMiniGame(R);
    }
}

// Malformed current-run payloads terminate as zero failures; stale identities never end another run.
bool AMiniGameRuntimeBase::EndMiniGame(const FMiniGameResult& Result)
{
    if (Lifecycle != EMiniGameLifecycleState::Playing || !Result.Matches(Context) || GetWorld()->IsPaused()) return false;
    FMiniGameResult Accepted = Result;
    double Base; EPerformanceRating Rating;
    if (!UScoringProfile::EvaluateMiniGame(Accepted, Context, Base, Rating))
        Accepted = FMiniGameResult::Failure(Context, EMiniGameEndReason::RuntimeFailed);
    else Accepted.PerformanceRating = Rating;
    Lifecycle = EMiniGameLifecycleState::Ended; StopProducers();
    OnMiniGameEnded.Broadcast(Accepted);
    return true;
}

// All actors are level-owned as well as registry-owned, so Owner alone is never the cleanup policy.
AActor* AMiniGameRuntimeBase::SpawnRunActor(TSubclassOf<AActor> Class, const FTransform& Transform)
{
    if (!Class || bCleaning || (Lifecycle != EMiniGameLifecycleState::Initialized && Lifecycle != EMiniGameLifecycleState::Playing)) return nullptr;
    FActorSpawnParameters Params; Params.Owner = this; Params.OverrideLevel = GetLevel();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Actor = GetWorld()->SpawnActor<AActor>(Class, Transform, Params);
    if (Actor)
    {
        RunActors.Add(Actor); Actor->SetActorTickEnabled(Lifecycle == EMiniGameLifecycleState::Playing);
        Actor->SetActorEnableCollision(Lifecycle == EMiniGameLifecycleState::Playing);
    }
    return Actor;
}

// Timer handles must be registered immediately after creation, including content setup timers.
void AMiniGameRuntimeBase::RegisterRunTimer(FTimerHandle Handle)
{
    RunTimers.AddUnique(Handle);
    if (Lifecycle != EMiniGameLifecycleState::Playing)
    {
        if (GetWorldTimerManager().IsTimerActive(Handle)) DeferredActiveTimers.AddUnique(Handle);
        GetWorldTimerManager().PauseTimer(Handle);
    }
}

// Freeze collision, physics, component ticks and local timers before any end delegate executes.
void AMiniGameRuntimeBase::StopProducers()
{
    SetActorTickEnabled(false);
    for (auto Handle : RunTimers) GetWorldTimerManager().PauseTimer(Handle);
    for (AActor* Actor : RunActors) if (IsValid(Actor))
    {
        Actor->SetActorTickEnabled(false); Actor->SetActorEnableCollision(false);
        TInlineComponentArray<UActorComponent*> Components(Actor);
        for (UActorComponent* Component : Components)
        {
            Component->SetComponentTickEnabled(false);
            if (auto* Body = Cast<UPrimitiveComponent>(Component)) Body->SetSimulatePhysics(false);
        }
    }
}

// Clear subscriptions after producer shutdown; this also handles incomplete Initialize/Start hooks.
void AMiniGameRuntimeBase::Cleanup()
{
    if (bCleaning) return;
    TGuardValue<bool> Guard(bCleaning, true);
    StopProducers();
    if (Lifecycle != EMiniGameLifecycleState::Dormant) OnCleanupRun();
    for (auto& Handle : RunTimers) GetWorldTimerManager().ClearTimer(Handle);
    RunTimers.Reset(); DeferredActiveTimers.Reset();
    for (AActor* Actor : RunActors) if (IsValid(Actor)) Actor->Destroy();
    RunActors.Reset(); RunPawn = nullptr; OnMiniGameEnded.Clear(); OnProgress.Clear();
    Context = FMiniGameContext(); Elapsed = 0; bPresentationReady = false;
    Lifecycle = EMiniGameLifecycleState::Dormant; SetActorHiddenInGame(true);
}

// Default content hooks intentionally have no gameplay side effects.
bool AMiniGameRuntimeBase::OnInitializeRun_Implementation(const FMiniGameContext& RunContext) { return true; }
// Derived content may reject startup; the base remains responsible for closing and cleaning the run.
bool AMiniGameRuntimeBase::OnStartRun_Implementation() { return true; }
// Supply every configured metric even for a zero-performance timeout.
FMiniGameResult AMiniGameRuntimeBase::BuildResult_Implementation() const { return FMiniGameResult::Failure(Context, EMiniGameEndReason::TimedOut); }
// Empty shared action hook leaves per-game rules entirely in the selected runtime.
void AMiniGameRuntimeBase::OnAction_Implementation() {}
// Content can release extra listeners here but cannot replace the base cleanup registry.
void AMiniGameRuntimeBase::OnCleanupRun_Implementation() {}
// Route only fresh active input; a confirmation event is consumed by flow instead.
void AMiniGameRuntimeBase::SubmitAction() { if (Lifecycle == EMiniGameLifecycleState::Playing && !GetWorld()->IsPaused()) OnAction(); }
