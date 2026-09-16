#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PinballPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APinballTable;
struct FInputActionValue;

/** Owns local input contexts and explicit view selection, with content-supplied assets. */
UCLASS()
class PINBALLBATTLE_API APinballPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    APinballPlayerController();
    void ConfigureTable(APinballTable* InTable);
    void CancelActions();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnUnPossess() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pinball|Input")
    TObjectPtr<UInputMappingContext> CommonMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pinball|Input")
    TObjectPtr<UInputMappingContext> PinballMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Pinball|Input") TObjectPtr<UInputAction> LeftFlipperAction;
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Input") TObjectPtr<UInputAction> RightFlipperAction;
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Input") TObjectPtr<UInputAction> PlungerAction;
private:
    void LeftPressed();
    void LeftReleased();
    void RightPressed();
    void RightReleased();
    void PlungerPressed();
    void PlungerReleased();
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> ControlsWidget;
};
