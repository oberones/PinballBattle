#include "Pinball/TableSessionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Pinball/PinballTable.h"
#include "Pinball/PinballBall.h"
#include "Pinball/TableSuspendParticipant.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Suspension is driven by the sole flow writer at a pre-physics boundary.
UTableSessionComponent::UTableSessionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// Prune dead entitlements before adding one explicit physical body.
void UTableSessionComponent::RegisterBody(UPrimitiveComponent* Body)
{
    Bodies.RemoveAll([](const auto& Item) { return !Item.IsValid(); });
    if (IsValid(Body)) Bodies.AddUnique(Body);
}

// Removing a ball must not leave it in the next suspension manifest.
void UTableSessionComponent::UnregisterBody(UPrimitiveComponent* Body)
{
    Bodies.RemoveAll([Body](const auto& Item) { return !Item.IsValid() || Item.Get() == Body; });
}

// Actor registration covers all component ticks while behavior hooks remain opt-in.
void UTableSessionComponent::RegisterParticipant(UObject* Participant)
{
    Participants.RemoveAll([](const auto& P) { return !P.IsValid(); });
    if (IsValid(Participant)) Participants.AddUnique(Participant);
}
// Explicit timer ownership avoids pausing unrelated minigame/world timers.
void UTableSessionComponent::RegisterTimer(FTimerHandle Timer) { Timers.AddUnique(Timer); }

// Capture every body and tick before the first physical mutation, preserving partial recovery data.
bool UTableSessionComponent::CaptureAndSuspend(int64 Generation, FTableSuspendSnapshot& S)
{
    auto* Table = Cast<APinballTable>(GetOwner());
    if (!Table || Generation <= 0 || SuspendedGeneration || !Table->EnsureReturnBall()) return false;
    bool Acknowledged = true;
    S = FTableSuspendSnapshot(); S.Generation = Generation;
    S.SessionId = Table->GetBallHandle().SessionId; S.BallId = Table->GetBallHandle().BallId;
    S.BallActor = Table->GetBall();
    for (const auto& Weak : Bodies)
    {
        auto* Body = Weak.Get(); if (!Body) { Acknowledged = false; continue; }
        FBodySnapshot B; B.Body = Body; B.Transform = Body->GetComponentTransform();
        B.bIsBall = Body->GetOwner() == Table->GetBall();
        B.LinearVelocity = Body->GetPhysicsLinearVelocity(); B.AngularVelocityRadians = Body->GetPhysicsAngularVelocityInRadians();
        B.Collision = Body->GetCollisionEnabled(); B.bSimulating = Body->IsSimulatingPhysics();
        B.bGravity = Body->IsGravityEnabled(); B.bAwake = Body->IsAnyRigidBodyAwake(); S.Bodies.Add(B);
    }
    RegisterParticipant(Table->GetBall());
    for (const auto& Weak : Participants)
    {
        UObject* Object = Weak.Get(); if (!Object) { Acknowledged = false; continue; }
        if (auto* Hook = Cast<ITableSuspendParticipant>(Object)) Acknowledged &= Hook->CaptureState(Generation);
        FParticipantSnapshot P; P.Participant = Object;
        if (auto* Actor = Cast<AActor>(Object))
        {
            P.bTickEnabled = Actor->IsActorTickEnabled();
            TInlineComponentArray<UActorComponent*> Components(Actor);
            for (auto* Component : Components) P.ComponentTicks.Add({Component, Component->IsComponentTickEnabled()});
        }
        else if (auto* Component = Cast<UActorComponent>(Object)) P.ComponentTicks.Add({Component, Component->IsComponentTickEnabled()});
        S.Participants.Add(P);
    }
    auto& Manager = GetWorld()->GetTimerManager();
    for (auto Handle : Timers)
    {
        FOwnedTimerSnapshot T; T.Handle = Handle; T.bWasActive = Manager.IsTimerActive(Handle);
        T.bWasPaused = Manager.IsTimerPaused(Handle); T.Remaining = Manager.GetTimerRemaining(Handle); S.Timers.Add(T);
    }
    SuspendedGeneration = Generation; Table->CancelActions();
    for (auto& B : S.Bodies)
    {
        auto* Body = B.Body.Get(); Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        Body->SetSimulatePhysics(false); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->SetWorldTransform(B.Transform, false, nullptr, ETeleportType::TeleportPhysics);
    }
    for (const auto& P : S.Participants)
    {
        if (auto* Actor = Cast<AActor>(P.Participant.Get())) Actor->SetActorTickEnabled(false);
        for (const auto& Tick : P.ComponentTicks) if (Tick.Key.IsValid()) Tick.Key->SetComponentTickEnabled(false);
    }
    for (const auto& T : S.Timers) if (T.bWasActive) Manager.PauseTimer(T.Handle);
    for (const auto& P : S.Participants)
        if (auto* Hook = Cast<ITableSuspendParticipant>(P.Participant.Get())) Acknowledged &= Hook->SuspendForMinigame(Generation);
    return Acknowledged;
}

// A blocked preferred and backup release leaves the table secured for explicit recovery.
bool UTableSessionComponent::PrepareRestore(FTableSuspendSnapshot& S)
{
    auto* Table = Cast<APinballTable>(GetOwner());
    if (!Table || S.Generation != SuspendedGeneration || S.SessionId != Table->GetBallHandle().SessionId ||
        S.BallId != Table->GetBallHandle().BallId) return false;
    if (!S.BallActor.IsValid())
    {
        if (!Table->EnsureReturnBall()) return false;
        S.Participants.RemoveAll([&S](const auto& P) { return P.Participant == S.BallActor; });
        S.Bodies.RemoveAll([](const auto& B) { return B.bIsBall; });
        FBodySnapshot B; B.Body = Table->GetBall()->GetBody(); B.Transform = B.Body->GetComponentTransform();
        B.bIsBall = B.bSimulating = B.bGravity = B.bAwake = true; B.Collision = ECollisionEnabled::QueryAndPhysics; S.Bodies.Add(B);
        FParticipantSnapshot P; P.Participant = Table->GetBall(); P.bTickEnabled = true; S.Participants.Add(P);
        S.BallActor = Table->GetBall(); RegisterParticipant(Table->GetBall());
    }
    for (const auto& P : S.Participants)
    {
        if (!P.Participant.IsValid()) return false;
        if (auto* Hook = Cast<ITableSuspendParticipant>(P.Participant.Get())) if (!Hook->PrepareRestore(S.Generation)) return false;
    }
    for (const auto& B : S.Bodies) if (!B.Body.IsValid()) return false;
    FVector Location, Velocity;
    if (!Table->FindSafeReturn(Location, Velocity)) return false;
    Table->CancelActions();
    for (auto& B : S.Bodies)
    {
        if (B.Body == Table->GetBall()->GetBody())
        { B.Transform.SetLocation(Location); B.LinearVelocity = Velocity; B.AngularVelocityRadians = FVector::ZeroVector; B.bAwake = true; }
        else { B.Transform = B.Body->GetComponentTransform(); B.LinearVelocity = FVector::ZeroVector; B.AngularVelocityRadians = FVector::ZeroVector; }
        B.Body->SetWorldTransform(B.Transform, false, nullptr, ETeleportType::TeleportPhysics);
    }
    S.bPrepared = true; return true;
}

// No failure-prone discovery occurs here; setup overlaps remain gated until flow publishes playback.
void UTableSessionComponent::CommitRestore(FTableSuspendSnapshot& S)
{
    check(S.bPrepared && S.Generation == SuspendedGeneration);
    for (const auto& B : S.Bodies)
    {
        auto* Body = B.Body.Get(); Body->SetEnableGravity(B.bGravity); Body->SetCollisionEnabled(B.Collision);
        Body->SetSimulatePhysics(B.bSimulating);
        if (B.bSimulating)
        {
            Body->SetPhysicsLinearVelocity(B.LinearVelocity); Body->SetPhysicsAngularVelocityInRadians(B.AngularVelocityRadians);
            if (B.bAwake) Body->WakeAllRigidBodies(); else Body->PutAllRigidBodiesToSleep();
        }
    }
    for (const auto& P : S.Participants)
    {
        if (auto* Hook = Cast<ITableSuspendParticipant>(P.Participant.Get())) Hook->CommitRestore(S.Generation);
        if (auto* Actor = Cast<AActor>(P.Participant.Get())) Actor->SetActorTickEnabled(P.bTickEnabled);
        for (const auto& Tick : P.ComponentTicks) if (Tick.Key.IsValid()) Tick.Key->SetComponentTickEnabled(Tick.Value);
    }
    for (const auto& T : S.Timers) if (T.bWasActive && !T.bWasPaused) GetWorld()->GetTimerManager().UnPauseTimer(T.Handle);
    SuspendedGeneration = 0;
}

// Restart restores surviving participants only after all old identities have been invalidated.
void UTableSessionComponent::ResetSuspension(FTableSuspendSnapshot& S)
{
    if (!SuspendedGeneration) return;
    auto* Table = Cast<APinballTable>(GetOwner());
    for (const auto& B : S.Bodies) if (B.Body.IsValid() && (!Table || B.Body->GetOwner() != Table->GetBall()))
    {
        B.Body->SetCollisionEnabled(B.Collision); B.Body->SetSimulatePhysics(B.bSimulating);
        B.Body->SetPhysicsLinearVelocity(FVector::ZeroVector); B.Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    }
    for (const auto& P : S.Participants) if (P.Participant.IsValid())
    {
        if (auto* Hook = Cast<ITableSuspendParticipant>(P.Participant.Get())) Hook->CommitRestore(S.Generation);
        if (auto* Actor = Cast<AActor>(P.Participant.Get())) Actor->SetActorTickEnabled(P.bTickEnabled);
        for (const auto& Tick : P.ComponentTicks) if (Tick.Key.IsValid()) Tick.Key->SetComponentTickEnabled(Tick.Value);
    }
    for (auto& T : S.Timers) GetWorld()->GetTimerManager().ClearTimer(T.Handle);
    Timers.Reset(); SuspendedGeneration = 0; S = FTableSuspendSnapshot();
}
