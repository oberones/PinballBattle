#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AssaultShipPawn.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UInputAction;
class AAlienAssaultRuntime;
struct FInputActionValue;
/** Planar ship driven exclusively by the installed Enhanced Input context. */
UCLASS(Blueprintable)
class PINBALLBATTLE_API AAssaultShipPawn : public APawn
{
    GENERATED_BODY()
public:
    /** Construct a swept sphere and original primitive ship silhouette. */
    AAssaultShipPawn();
    /** Read current Enhanced Input values so pause cannot leave latched commands. */
    virtual void Tick(float DeltaSeconds) override;
    /** Bind movement actions on possession; controller retains context ownership. */
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    /** Reset motion at the runtime's explicitly configured spawn. */
    void Respawn();
    /** Retain run ownership explicitly because possession changes APawn's Actor Owner. */
    void ConfigureRun(AAlienAssaultRuntime* InRuntime);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> LeftAction;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> RightAction;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> FireAction;
    UPROPERTY(EditDefaultsOnly) float MovementSpeed = 650;
private:
    UPROPERTY(Transient) TWeakObjectPtr<AAlienAssaultRuntime> RunRuntime;
    FGuid RunId;
};
