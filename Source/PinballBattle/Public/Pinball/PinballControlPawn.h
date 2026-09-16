#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PinballControlPawn.generated.h"

class UCameraComponent;
class APinballTable;

/** Persistent control target; the physical ball will be a separate, unpossessed Actor. */
UCLASS()
class PINBALLBATTLE_API APinballControlPawn : public APawn
{
    GENERATED_BODY()

public:
    APinballControlPawn();
    void SetTable(APinballTable* InTable) { Table = InTable; }
    void SetLeftHeld(bool bHeld);
    void SetRightHeld(bool bHeld);
    void BeginPlunger();
    void ReleasePlunger();
    void CancelActions();

private:
    UPROPERTY(Transient) TObjectPtr<APinballTable> Table;
    // Foundation view until Phase 2 registers the table's explicitly authored camera.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FoundationCamera;
};
