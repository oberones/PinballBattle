#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PinballSessionTypes.h"
#include "GameFlowComponent.generated.h"

class APinballGameStateBase;

/** Sole state writer. GameMode requests guarded edges after their prerequisites succeed. */
UCLASS()
class PINBALLBATTLE_API UGameFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Disable component ticking; guarded requests are the only way to advance flow. */
    UGameFlowComponent();

    /** Read the single authoritative flow value. */
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    EArcadeGameFlowState GetCurrentState() const { return CurrentState; }
    /** Check ordinary legal edges without performing any world mutation. */
    static bool IsLegalTransition(EArcadeGameFlowState From, EArcadeGameFlowState To);
    /** Identify phases which can be saved under the pause overlay. */
    static bool CanPause(EArcadeGameFlowState State);

private:
    friend class APinballGameModeBase;
    friend class FPinballFlowTest;
    /** Bind the GameState projection owned by this GameMode's world. */
    void InitializeProjection(APinballGameStateBase* InGameState);
    /** Commit a legal edge; session transactions can defer notification until all owners agree. */
    bool TransitionTo(EArcadeGameFlowState Next, bool bPublish = true);
    /** Save one underlying state or restore it, rejecting nested pause and repeated resume. */
    bool SetPaused(bool bPaused);

    UPROPERTY(Transient)
    TObjectPtr<APinballGameStateBase> GameState;

    UPROPERTY(VisibleInstanceOnly, Category = "Pinball|Flow")
    EArcadeGameFlowState CurrentState = EArcadeGameFlowState::BOOT;
};
