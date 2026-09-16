#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minigames/Shared/MiniGameLifecycle.h"
#include "MiniGameRuntimeBase.generated.h"
class UCameraComponent;
class APawn;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMiniGameEnded, const FMiniGameResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMiniGameProgress, double, Elapsed, int64, RawScore);

/** World-local runtime with a guarded lifecycle, active-time ceiling and explicit resource registry. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AMiniGameRuntimeBase : public AActor, public IMiniGameLifecycle
{
    GENERATED_BODY()
public:
    /** Create the explicit camera and keep authored roots dormant at startup. */
    AMiniGameRuntimeBase();
    /** Register this exact level instance without starting a run from BeginPlay. */
    virtual void BeginPlay() override;
    /** Unregister and clean resources without restoring presentation during teardown. */
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    /** Advance only an active run and clamp its terminal clock at the duration limit. */
    virtual void Tick(float DeltaSeconds) override;
    /** Allocate a fresh local context and run pawn; failed partial initialization remains cleanable. */
    UFUNCTION(BlueprintCallable) virtual bool Initialize(const FMiniGameContext& InContext) override final;
    /** Start exactly once after flow certifies the camera, pawn and input barrier. */
    UFUNCTION(BlueprintCallable) virtual bool StartMiniGame() override final;
    /** Close producers before emitting one identity-checked terminal result. */
    UFUNCTION(BlueprintCallable) virtual bool EndMiniGame(const FMiniGameResult& Result) override final;
    /** Idempotently clear owned timers, actors, delegates and local state. */
    UFUNCTION(BlueprintCallable) virtual void Cleanup() override final;
    /** Record a controller acknowledgment only for this initialized run. */
    bool SetPresentationReady(bool Ready);
    /** Return true only while registered camera and pawn belong to this arena instance. */
    bool HasPresentationTargets() const;
    /** Spawn into this root's level and register ownership before content sees the actor. */
    UFUNCTION(BlueprintCallable) AActor* SpawnRunActor(TSubclassOf<AActor> Class, const FTransform& Transform);
    /** Register each local timer for terminal pause and cleanup. */
    void RegisterRunTimer(FTimerHandle Handle);
    /** Read the guarded lifecycle for flow and diagnostics. */
    EMiniGameLifecycleState GetLifecycle() const { return Lifecycle; }
    /** Read immutable accepted context for identity checks. */
    const FMiniGameContext& GetContext() const { return Context; }
    /** Read active elapsed time without handing clock ownership to UI. */
    double GetElapsed() const { return Elapsed; }
    /** Project local informational score without exposing the central scoring service. */
    int64 GetLocalScore() const { return BuildResult().RawScore; }
    /** Read the explicit possessed actor owned by this run. */
    APawn* GetRunPawn() const { return RunPawn; }
    /** Supply the boot-resolved class before Initialize; no synchronous activation load. */
    void ConfigurePawn(TSubclassOf<APawn> Class) { PawnClass = Class; }
    /** Forward a fresh contextual action to content only while Playing. */
    void SubmitAction();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(BlueprintAssignable) FMiniGameEnded OnMiniGameEnded;
    UPROPERTY(BlueprintAssignable) FMiniGameProgress OnProgress;
protected:
    /** Prepare content without starting timers or accepting gameplay input. */
    UFUNCTION(BlueprintNativeEvent) bool OnInitializeRun(const FMiniGameContext& RunContext);
    /** Start content after all technical and human readiness gates pass. */
    UFUNCTION(BlueprintNativeEvent) bool OnStartRun();
    /** Build local performance fields; the base supplies identity and duration. */
    UFUNCTION(BlueprintNativeEvent) FMiniGameResult BuildResult() const;
    /** Perform a local contextual action without access to global score. */
    UFUNCTION(BlueprintNativeEvent) void OnAction();
    /** Release content-owned bindings and effects; repeat calls are permitted. */
    UFUNCTION(BlueprintNativeEvent) void OnCleanupRun();
private:
    friend class UMinigameWorldSubsystem;
    /** Freeze run actors and timers before publishing a terminal notification. */
    void StopProducers();
    UPROPERTY(Transient) TObjectPtr<APawn> RunPawn;
    UPROPERTY(Transient) TSubclassOf<APawn> PawnClass;
    UPROPERTY(Transient) TArray<TObjectPtr<AActor>> RunActors;
    TArray<FTimerHandle> RunTimers;
    TArray<FTimerHandle> DeferredActiveTimers;
    FMiniGameContext Context;
    EMiniGameLifecycleState Lifecycle = EMiniGameLifecycleState::Dormant;
    double Elapsed = 0;
    bool bPresentationReady = false;
    bool bCleaning = false;
};
