#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PinballControlPawn.generated.h"

class UCameraComponent;

/** Persistent control target; the physical ball will be a separate, unpossessed Actor. */
UCLASS()
class PINBALLBATTLE_API APinballControlPawn : public APawn
{
    GENERATED_BODY()

public:
    APinballControlPawn();

private:
    // Foundation view until Phase 2 registers the table's explicitly authored camera.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pinball|Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FoundationCamera;
};
