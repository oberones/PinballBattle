#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/MiniGameTypes.h"
#include "MiniGameLifecycle.generated.h"

UINTERFACE(BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class PINBALLBATTLE_API UMiniGameLifecycle : public UInterface { GENERATED_BODY() };

/** Native guards remain authoritative; Blueprint content extends protected runtime hooks. */
class PINBALLBATTLE_API IMiniGameLifecycle
{
    GENERATED_BODY()
public:
    /** Prepare local resources without opening gameplay. */
    virtual bool Initialize(const FMiniGameContext& Context) = 0;
    /** Open gameplay once presentation has acknowledged readiness. */
    virtual bool StartMiniGame() = 0;
    /** Latch and publish one terminal value after closing local producers. */
    virtual bool EndMiniGame(const FMiniGameResult& Result) = 0;
    /** Release partial or completed resources safely on repeated calls. */
    virtual void Cleanup() = 0;
};
