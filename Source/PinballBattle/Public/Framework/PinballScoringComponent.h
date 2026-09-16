#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ScoringTypes.h"
#include "PinballScoringComponent.generated.h"

class UScoringProfile;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPinballScoreChanged, const FScoreAward&, Award);

/** Sole total/ledger owner; only the GameMode supplies authoritative session/event context. */
UCLASS()
class PINBALLBATTLE_API UPinballScoringComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Read the one authoritative total without granting mutation rights. */
    UFUNCTION(BlueprintPure) int64 GetTotalScore() const { return TotalScore; }
    /** Read the last immutable committed award for presentation. */
    UFUNCTION(BlueprintPure) FScoreAward GetLatestAward() const { return LatestAward; }
    /** Multiply once, floor once, then reject conversion/addition overflow before changing output. */
    static bool CalculateAward(double BasePoints, int32 Multiplier, int64 Total, int64& Points, int64& NewTotal);
    UPROPERTY(BlueprintAssignable) FPinballScoreChanged OnScoreChanged;
private:
    friend class APinballGameModeBase;
    friend class FPinballScoreTest;
    friend class FPinballRestartTest;
    /** Capture validated profile values and invalidate all previous accepted-event identities. */
    bool ResetSession(FGuid SessionId, const UScoringProfile* Profile);
    /** Validate the authoritative phase/ball/epoch and commit ledger plus total before notification. */
    bool SubmitTableScore(const FScoringEvent& Event, const FSessionState& State, const FBallHandle& Ball, int64 Epoch);
    FGuid ActiveSession;
    TMap<EScoringCategory, double> CategoryPoints;
    TSet<FGuid> AcceptedEvents;
    TMap<FGuid, int64> SourceSequences;
    FGuid SequenceBall;
    FName ProfileRevision;
    int32 MinimumMultiplier = 1;
    int32 MaximumMultiplier = 10;
    int64 TotalScore = 0;
    FScoreAward LatestAward;
};
