#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PinballSessionTypes.h"
#include "Data/TransitionTypes.h"
#include "GameFlowComponent.generated.h"

class APinballGameStateBase;
class UMiniGameDefinition;
class AMiniGameRuntimeBase;
class APinballPlayerController;
class UMinigameTriggerComponent;

/** Sole state writer. GameMode requests guarded edges after their prerequisites succeed. */
UCLASS()
class PINBALLBATTLE_API UGameFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Tick transaction clocks before physics; guarded requests remain the only state-writing boundary. */
    UGameFlowComponent();

    /** Read the single authoritative flow value. */
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    EArcadeGameFlowState GetCurrentState() const { return CurrentState; }
    /** Check ordinary legal edges without performing any world mutation. */
    static bool IsLegalTransition(EArcadeGameFlowState From, EArcadeGameFlowState To);
    /** Identify phases which can be saved under the pause overlay. */
    static bool CanPause(EArcadeGameFlowState State);
    /** Drive residency and transition phases only on unpaused pre-physics ticks. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    /** Start boot preload; no minigame can synchronously load at activation. */
    void BeginMiniGameBoot();
    /** Accept one registered objective with current session/ball/event identities. */
    bool RequestObjective(UMinigameTriggerComponent* Trigger, FGuid SessionId, FGuid BallId, FGuid EventId);
    /** Register a unique authored objective identity at BeginPlay. */
    bool RegisterObjective(UMinigameTriggerComponent* Trigger);
    /** Record a genuine physical exit without bypassing the separate return delay. */
    void NotifyObjectiveExit(UMinigameTriggerComponent* Trigger);
    /** Consume fresh confirmation only after technical readiness, before installing gameplay input. */
    bool ConfirmMiniGame();
    /** Retry a secured return explicitly, never through a recursive failure loop. */
    void RetryReturn();
    /** Cancel resources without restoring into a tearing-down world. */
    void CancelForTeardown();
    /** Clean an invalidated failed transaction before an explicit session restart. */
    void ResetForRecoveryRestart();
    /** Expose exact phase for presentation and acceptance diagnostics. */
    const FTransitionRecord& GetTransition() const { return Transition; }
    /** Read current root without owning its lifetime in the flow component. */
    AMiniGameRuntimeBase* GetRuntime() const { return Runtime.Get(); }
    /** Display a content instruction or local progress summary without widget clocks. */
    FText GetMiniGameStatus() const;
    /** Report a bounded boot failure so presentation can offer Retry/Quit. */
    bool HasBootFailed() const { return bBootFailed; }

private:
    friend class APinballGameModeBase;
    friend class FPinballFlowTest;
    /** Bind the GameState projection owned by this GameMode's world. */
    void InitializeProjection(APinballGameStateBase* InGameState);
    /** Commit a legal edge; session transactions can defer notification until all owners agree. */
    bool TransitionTo(EArcadeGameFlowState Next, bool bPublish = true);
    /** Save one underlying state or restore it, rejecting nested pause and repeated resume. */
    bool SetPaused(bool bPaused);
    /** Advance the internal phase and publish a fully initialized projection. */
    void SetPhase(ETransitionPhase Phase);
    /** Latch one current-generation terminal result and apply its central reward. */
    UFUNCTION() void HandleMiniGameEnded(const FMiniGameResult& Result);
    /** Converge all failures on a two-active-second zero-award recovery notice. */
    void Recover(EMiniGameEndReason Reason);
    /** Accept one subsystem cancellation; defer its flow continuation until native pause ends. */
    void HandleRunCancelled(const FMiniGameContext& Context, EMiniGameEndReason Reason);
    /** Restore controller before cleanup, then stage and commit table producers together. */
    bool TryReturn();
    /** Resolve only the world-local controller explicitly registered by GameMode. */
    APinballPlayerController* GetController() const;
    FTransitionRecord Transition;
    UPROPERTY(Transient) TObjectPtr<UMiniGameDefinition> Definition;
    UPROPERTY(Transient) TWeakObjectPtr<AMiniGameRuntimeBase> Runtime;
    UPROPERTY(Transient) TWeakObjectPtr<UMinigameTriggerComponent> ActiveTrigger;
    double ObjectiveProtectionRemaining = 0;
    double BootSeconds = 0;
    bool bBootLoading = false;
    bool bBootFailed = false;
    TMap<FName, TWeakObjectPtr<UMinigameTriggerComponent>> Objectives;
    bool bInvalidObjectives = false;
    TOptional<EMiniGameEndReason> PendingCancellation;

    UPROPERTY(Transient)
    TObjectPtr<APinballGameStateBase> GameState;

    UPROPERTY(VisibleInstanceOnly, Category = "Pinball|Flow")
    EArcadeGameFlowState CurrentState = EArcadeGameFlowState::BOOT;
};
