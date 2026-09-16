#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PinballSessionTypes.h"
#include "GameFlowComponent.generated.h"

class APinballGameStateBase;

/** Sole state writer; Phase 1 establishes BOOT without enabling an unfinished session. */
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

    UPROPERTY(Transient)
    TObjectPtr<APinballGameStateBase> GameState;

    UPROPERTY(VisibleInstanceOnly, Category = "Pinball|Flow")
    EArcadeGameFlowState CurrentState = EArcadeGameFlowState::BOOT;
};
