#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ScoringTypes.h"
#include "PinballPresentationWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class APinballGameStateBase;

UENUM(BlueprintType)
enum class EPinballScreen : uint8 { Start, HUD, Pause, GameOver };

/** Delegate-driven presentation only: all buttons forward intents to the owning controller. */
UCLASS()
class PINBALLBATTLE_API UPinballPresentationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    /** Build a small native layout which content subclasses configure by screen and title. */
    virtual void NativeOnInitialized() override;
    /** Subscribe to state and score only while this widget is on screen. */
    virtual void NativeConstruct() override;
    /** Release both subscriptions when the controller replaces this screen. */
    virtual void NativeDestruct() override;
    /** Route keyboard confirmation through the same intent as the primary button. */
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    /** Expose displayed values for rendered acceptance checks without gameplay authority. */
    FString GetStatusText() const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EPinballScreen Screen = EPinballScreen::HUD;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText Heading;
private:
    /** Create a readable label in the menu column without a per-frame binding. */
    UTextBlock* AddLabel(UVerticalBox* Column, const FText& Text, int32 Size);
    /** Create a focusable button with a text child; callers connect intent delegates. */
    UButton* AddButton(UVerticalBox* Column, const FText& Text);
    /** Refresh projected score/ball information after a session notification. */
    UFUNCTION() void OnSession(const FSessionState& State);
    /** Refresh score presentation after a committed central award/reset. */
    UFUNCTION() void OnScore(const FScoreAward& Award);
    /** Send Start, Resume or Restart according to this content-selected screen. */
    UFUNCTION() void PrimaryIntent();
    /** Send Quit to the controller; this widget never tears down gameplay itself. */
    UFUNCTION() void QuitIntent();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    UPROPERTY(Transient) TObjectPtr<APinballGameStateBase> StateSource;
};
