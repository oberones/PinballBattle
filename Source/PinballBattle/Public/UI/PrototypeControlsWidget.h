#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PrototypeControlsWidget.generated.h"

class UTextBlock;

/** Presentation-only prototype instructions; the test Blueprint supplies all player copy. */
UCLASS()
class PINBALLBATTLE_API UPrototypeControlsWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    UPROPERTY(EditDefaultsOnly, Category="Presentation") FText Heading;
    UPROPERTY(EditDefaultsOnly, Category="Presentation", meta=(MultiLine=true)) FText Instructions;
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
};
