#pragma once
#include "CoreMinimal.h"

/** Pure contact latch shared by the physical target and powered bumper. */
struct PINBALLBATTLE_API FContactEpisode
{
    FGuid BallId;
    bool bContact = false;
    /** Start one episode; callback repeats cannot start another until measured separation. */
    bool TryBegin(const FGuid& InBallId);
    /** Only a valid distance beyond radius plus hysteresis ends a sustained contact. */
    void ObserveSeparation(float Distance, float Radius, float Tolerance);
    /** Invalidate old geometry after a teleport, replacement or phase change. */
    void Reset();
};

/** Directed lane gate state, evaluated against the ball's swept centre path in lane space. */
struct PINBALLBATTLE_API FLaneTraversal
{
    bool bEntered = false;
    bool bLocked = false;
    /** Require forward entry then exit within the corridor; re-arm only after full departure. */
    bool Advance(const FVector& Previous, const FVector& Current, const FVector& Extent, float Radius);
    /** Discard partial traversal when the ball or physical epoch changes. */
    void Reset();
};
