#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PinballPlunger.generated.h"

class UPinballTuningData;
class UStaticMeshComponent;

UCLASS()
class PINBALLBATTLE_API APinballPlunger : public AActor
{
    GENERATED_BODY()
public:
    APinballPlunger();
    void Configure(UPinballTuningData* InTuning) { Tuning = InTuning; }
    void BeginCharge();
    float ReleaseCharge();
    void CancelActions();
    float GetChargeAlpha() const;
    bool IsCharging() const { return bCharging; }
    FVector GetLaunchDirection() const { return GetActorForwardVector(); }
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(Transient) TObjectPtr<UPinballTuningData> Tuning;
    float ChargeSeconds = 0.f;
    bool bCharging = false;
};
