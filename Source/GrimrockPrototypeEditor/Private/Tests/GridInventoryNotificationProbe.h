#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GridInventoryNotificationProbe.generated.h"

class AActor;

// Editor-only observer for synchronous transaction characterization. No authored asset uses this type.
UCLASS(Transient)
class UGridInventoryNotificationProbe : public UObject
{
	GENERATED_BODY()
public:
	TFunction<void(int32)> Callback;

	UFUNCTION()
	void Observe(int32 CharacterIndex)
	{
		if (Callback)
			Callback(CharacterIndex);
	}

	UFUNCTION()
	void ObserveDestroyed(AActor* Actor)
	{
		(void)Actor;
		if (Callback)
			Callback(INDEX_NONE);
	}
};
