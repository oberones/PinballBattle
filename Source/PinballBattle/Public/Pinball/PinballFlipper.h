#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PinballFlipper.generated.h"

class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class UPinballTuningData;

UCLASS()
class PINBALLBATTLE_API APinballFlipper : public AActor
{
    GENERATED_BODY()
public:
    APinballFlipper();
    void Configure(UPinballTuningData* Tuning);
    void SetHeld(bool bInHeld);
    UStaticMeshComponent* GetBody() const { return Body; }
    bool IsHeld() const { return bHeld; }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pinball") bool bReverseDrive = false;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> Hinge;
    float HalfTravel = 27.5f;
    bool bHeld = false;
};
