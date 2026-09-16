#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/PinballSessionTypes.h"
#include "Data/TransitionTypes.h"
#include "PinballPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APinballTable;
class UPinballPresentationWidget;
class AMiniGameRuntimeBase;
class UMiniGameDefinition;
struct FInputActionValue;

/** Owns local input contexts and explicit view selection, with content-supplied assets. */
UCLASS()
class PINBALLBATTLE_API APinballPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    /** Keep camera selection explicit across possession changes. */
    APinballPlayerController();
    /** Snapshot persistent pinball presentation before releasing control. */
    bool CaptureMiniGameMode(int64 Generation, FControllerModeSnapshot& Snapshot);
    /** Possess the prepared run pawn and cut to its registered camera. */
    bool SwitchToMiniGame(AMiniGameRuntimeBase* Runtime);
    /** Install the boot-resolved owned context with fresh Boolean/axis input guards. */
    bool EnableMiniGameInput(const UMiniGameDefinition* Definition);
    /** Remove only this controller's minigame mapping while preserving common Escape. */
    void DisableMiniGameInput();
    /** Restore the persistent pawn and view before outgoing actors are cleaned. */
    bool RestoreMiniGameMode(const FControllerModeSnapshot& Snapshot);
    /** Present flow-owned status without adding widget clocks. */
    void UpdateMiniGameStatus(const FText& Status);
    /** Track release-neutral readiness and consume confirmation separately from gameplay actions. */
    virtual void PlayerTick(float DeltaSeconds) override;
    /** Pawn axis handlers must use this gate until every carried input has returned to neutral. */
    bool IsMiniGameInputReady() const { return bFreshMiniGameInput && MiniGameMappingContext != nullptr; }
    /** Forward explicit secured recovery retry. */
    UFUNCTION(BlueprintCallable) void RequestRetryIntent();
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
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MiniGameMappingContext;
    UPROPERTY(Transient) TWeakObjectPtr<AMiniGameRuntimeBase> MiniGameRuntime;
    bool bModeSwap = false;
    bool bFreshMiniGameInput = false;
    bool bSpaceWasDown = false;
    bool bEnterWasDown = false;
    uint32 MiniGameActionBinding = 0;
    int64 ModeGeneration = 0;
    UPROPERTY(Transient) TWeakObjectPtr<APawn> PersistentPinballPawn;
    /** Route Enhanced Input's fresh action event through the active local lifecycle gate. */
    void MiniGameActionPressed();
    /** Check physical keyboard and mapped analog axes before allowing confirmation or fresh input. */
    bool AreMiniGameAxesNeutral() const;
};
