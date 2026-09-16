#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PinballGameModeBase.generated.h"

class UGameFlowComponent;

UCLASS()
class PINBALLBATTLE_API APinballGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    APinballGameModeBase();
    virtual void InitGameState() override;

    UFUNCTION(BlueprintPure, Category = "Pinball|Flow")
    UGameFlowComponent* GetGameFlow() const { return GameFlow; }

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Flow", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UGameFlowComponent> GameFlow;
};
