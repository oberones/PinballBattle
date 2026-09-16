#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/PinballSessionTypes.h"
#include "PinballPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APinballTable;
class UPinballPresentationWidget;
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
    /** Forward Start/Restart intents to the authoritative GameMode. */
    UFUNCTION(BlueprintCallable) void RequestStartIntent();
    /** Forward Escape/Resume intent while remaining callable during native pause. */
    UFUNCTION(BlueprintCallable) void RequestPauseIntent();
    /** Invalidate gameplay before asking Unreal to exit the standalone game. */
    UFUNCTION(BlueprintCallable) void RequestQuitIntent();
    /** Expose the current presentation for development acceptance observations. */
    UPinballPresentationWidget* GetPresentation() const { return Presentation; }

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
    UPROPERTY(EditDefaultsOnly, Category="Pinball|Input") TObjectPtr<UInputAction> PauseAction;
private:
    /** Switch content-selected screens and install gameplay input only in ready/playing states. */
    UFUNCTION() void RefreshPresentation(const FSessionState& State);
    /** Cancel logical actions and rebuild owned mappings with held-key suppression. */
    void SetGameplayInput(bool bEnabled);
    void LeftPressed();
    void LeftReleased();
    void RightPressed();
    void RightReleased();
    void PlungerPressed();
    void PlungerReleased();
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> ControlsWidget;
    UPROPERTY(Transient) TObjectPtr<UPinballPresentationWidget> Presentation;
    EArcadeGameFlowState LastPresentedState = EArcadeGameFlowState::BOOT;
};
