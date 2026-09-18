#pragma once
#include "CoreMinimal.h"
#include "Data/MiniGameTypes.h"
#include "Data/ScoringTypes.h"
#include "TransitionTypes.generated.h"
class UPrimitiveComponent;
class UActorComponent;
class APawn;
class UInputMappingContext;
class SWidget;

UENUM(BlueprintType)
enum class ETransitionPhase : uint8 { None, Securing, Preparing, AwaitingConfirmation, Playing, Results, Recovering, Returning, RecoveryMenu };

/** Physics state uses world transforms and angular velocity in radians throughout. */
struct FBodySnapshot
{
    TWeakObjectPtr<UPrimitiveComponent> Body;
    FTransform Transform;
    FVector LinearVelocity = FVector::ZeroVector;
    FVector AngularVelocityRadians = FVector::ZeroVector;
    ECollisionEnabled::Type Collision = ECollisionEnabled::NoCollision;
    bool bSimulating = false;
    bool bGravity = false;
    bool bAwake = false;
    bool bIsBall = false;
};
struct FOwnedTimerSnapshot
{
    FTimerHandle Handle;
    bool bWasActive = false;
    bool bWasPaused = false;
    float Remaining = 0;
};
struct FParticipantSnapshot
{
    TWeakObjectPtr<UObject> Participant;
    bool bTickEnabled = false;
    TArray<TPair<TWeakObjectPtr<UActorComponent>, bool>> ComponentTicks;
};
struct FTableSuspendSnapshot
{
    FGuid SessionId;
    FGuid BallId;
    int64 Generation = 0;
    TWeakObjectPtr<AActor> BallActor;
    TArray<FBodySnapshot> Bodies;
    TArray<FParticipantSnapshot> Participants;
    TArray<FOwnedTimerSnapshot> Timers;
    bool bPrepared = false;
};
struct FControllerModeSnapshot
{
    TWeakObjectPtr<APawn> Pawn;
    TWeakObjectPtr<AActor> ViewTarget;
    TWeakObjectPtr<UInputMappingContext> GameplayContext;
    TWeakPtr<SWidget> Focus;
    bool bShowCursor = false;
    int64 Generation = 0;
};
struct FTransitionRecord
{
    FMiniGameContext Context;
    FGuid BallId;
    FName ObjectiveId;
    ETransitionPhase Phase = ETransitionPhase::None;
    double PhaseSeconds = 0;
    double WatchdogSeconds = 0;
    bool bTerminal = false;
    FTableSuspendSnapshot Table;
    FControllerModeSnapshot Controller;
    FScoreAward Award;
};
