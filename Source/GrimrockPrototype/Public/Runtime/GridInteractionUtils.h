#pragma once

#include "CoreMinimal.h"

class AActor;
class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;
class APawn;

namespace GridInteractionUtils
{
	AGridLevelRuntimeActor* ResolveRuntimeActor(APawn* InstigatorPawn, const AActor* ContextActor);
	AGrimrockPartyPawn* ResolvePartyPawn(APawn* InstigatorPawn);
}
