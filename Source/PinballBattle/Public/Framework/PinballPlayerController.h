#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PinballPlayerController.generated.h"

class UInputMappingContext;

/** Owns local input contexts and explicit view selection, with content-supplied assets. */
UCLASS()
class PINBALLBATTLE_API APinballPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    APinballPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pinball|Input")
    TObjectPtr<UInputMappingContext> CommonMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pinball|Input")
    TObjectPtr<UInputMappingContext> PinballMappingContext;
};
