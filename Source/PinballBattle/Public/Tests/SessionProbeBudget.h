#pragma once
#include "CoreMinimal.h"

/** Wall-clock bounds for the rendered acceptance scenario, including its paused intervals. */
struct FSessionProbeBudget
{
    int32 Sessions = 3;
    int32 BallsPerSession = 3;
    double PerBallSeconds = 65;
    double SetupAndMenuSeconds = 60;
    double TotalSeconds = 645;

    /** Calculate the minimum overall allowance from the individual scenario bounds. */
    double RequiredSeconds() const { return Sessions * BallsPerSession * PerBallSeconds + SetupAndMenuSeconds; }
    /** Reject unsafe overrides and budgets that would terminate a permitted scenario early. */
    bool IsValid() const
    {
        return Sessions >= 3 && Sessions <= 100 && BallsPerSession == 3 &&
            FMath::IsFinite(PerBallSeconds) && PerBallSeconds > 0 &&
            FMath::IsFinite(SetupAndMenuSeconds) && SetupAndMenuSeconds >= 20 * Sessions &&
            FMath::IsFinite(TotalSeconds) && TotalSeconds >= RequiredSeconds();
    }
    /** Keep the exact limit inclusive so both per-ball and overall deadlines share one boundary. */
    static bool HasExpired(double Elapsed, double Limit) { return Elapsed > Limit; }
};
