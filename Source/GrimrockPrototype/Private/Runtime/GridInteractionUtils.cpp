#include "Runtime/GridInteractionUtils.h"

#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridInteractionUtils
{
    AGrimrockPartyPawn* ResolvePartyPawn (APawn* InstigatorPawn)
    {
        return Cast<AGrimrockPartyPawn> (InstigatorPawn);
    }

    AGridLevelRuntimeActor* ResolveRuntimeActor (APawn* InstigatorPawn, const AActor* ContextActor)
    {
        if (const AGrimrockPartyPawn* PartyPawn = ResolvePartyPawn (InstigatorPawn))
        {
            if (PartyPawn->LevelRuntimeActor)
            {
                return PartyPawn->LevelRuntimeActor;
            }
        }

        if (ContextActor)
        {
            if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (ContextActor->GetOwner ()))
            {
                return RuntimeActor;
            }

            if (UWorld* World = ContextActor->GetWorld ())
            {
                for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
                {
                    return *It;
                }
            }
        }

        return nullptr;
    }
}
