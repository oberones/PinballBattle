#include "Pinball/InteractionState.h"

bool FContactEpisode::TryBegin(const FGuid& InBallId)
{
    if (!InBallId.IsValid()) return false;
    if (BallId != InBallId) { Reset(); BallId = InBallId; }
    if (bContact) return false;
    bContact = true;
    return true;
}

void FContactEpisode::ObserveSeparation(float Distance, float Radius, float Tolerance)
{
    if (FMath::IsFinite(Distance) && Distance > Radius + Tolerance) bContact = false;
}

void FContactEpisode::Reset()
{
    BallId.Invalidate();
    bContact = false;
}

bool FLaneTraversal::Advance(const FVector& Previous, const FVector& Current, const FVector& Extent, float Radius)
{
    const auto InCrossSection = [&](const FVector& Point)
    {
        // Require the complete sphere to fit between the lane guides and under its gate.
        return FMath::Abs(Point.X) <= Extent.X - Radius && FMath::Abs(Point.Z) <= Extent.Z - Radius;
    };
    const bool bDeparted = FMath::Abs(Current.X) > Extent.X + Radius ||
        FMath::Abs(Current.Y) > Extent.Y + Radius || FMath::Abs(Current.Z) > Extent.Z + Radius;
    if (bLocked)
    {
        if (bDeparted) Reset();
        return false;
    }
    const double Travel = Current.Y - Previous.Y;
    if (Travel <= 0)
    {
        if (Current.Y < -Extent.Y || !InCrossSection(Current)) bEntered = false;
        return false;
    }
    const auto Crosses = [&](double GateY)
    {
        return Previous.Y < GateY && Current.Y >= GateY &&
            InCrossSection(FMath::Lerp(Previous, Current, (GateY - Previous.Y) / Travel));
    };
    if (Crosses(-Extent.Y)) bEntered = true;
    if (bEntered && Crosses(Extent.Y))
    {
        bEntered = false;
        bLocked = true;
        return true;
    }
    if (!InCrossSection(Current) || bDeparted) bEntered = false;
    return false;
}

void FLaneTraversal::Reset()
{
    bEntered = false;
    bLocked = false;
}
