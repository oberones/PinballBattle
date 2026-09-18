#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TableSuspendParticipant.generated.h"
UINTERFACE()
class PINBALLBATTLE_API UTableSuspendParticipant : public UInterface { GENERATED_BODY() };
/** Opt-in hooks for owned behavior beyond the table's body, tick and timer manifests. */
class PINBALLBATTLE_API ITableSuspendParticipant
{
    GENERATED_BODY()
public:
    /** Capture local progress before cancellation or body mutation. */
    virtual bool CaptureState(int64 Generation) { return Generation > 0; }
    /** Stop owned forces or emitters after capture. */
    virtual bool SuspendForMinigame(int64 Generation) { return Generation > 0; }
    /** Stage neutral behavior without opening gates. */
    virtual bool PrepareRestore(int64 Generation) { return Generation > 0; }
    /** Commit only prevalidated behavior at the pre-physics boundary. */
    virtual void CommitRestore(int64 Generation) {}
};
