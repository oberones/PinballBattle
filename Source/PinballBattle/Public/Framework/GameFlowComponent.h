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
    UGameFlowComponent();

    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    EArcadeGameFlowState GetCurrentState() const { return CurrentState; }

private:
    friend class APinballGameModeBase;
    void InitializeProjection(APinballGameStateBase* InGameState);
    bool TransitionTo(EArcadeGameFlowState Next);

    UPROPERTY(Transient)
    TObjectPtr<APinballGameStateBase> GameState;

    UPROPERTY(VisibleInstanceOnly, Category = "Pinball|Flow")
    EArcadeGameFlowState CurrentState = EArcadeGameFlowState::BOOT;
};
