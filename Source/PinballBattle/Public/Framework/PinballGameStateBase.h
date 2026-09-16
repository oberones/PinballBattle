#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/PinballSessionTypes.h"
#include "PinballGameStateBase.generated.h"

/** Read-only world-scoped projection. Only the GameMode's flow component may publish flow. */
UCLASS()
class PINBALLBATTLE_API APinballGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    EArcadeGameFlowState GetFlowState() const { return FlowState; }

private:
    friend class UGameFlowComponent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    EArcadeGameFlowState FlowState = EArcadeGameFlowState::BOOT;
};
