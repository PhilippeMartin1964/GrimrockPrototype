#pragma once

#include "CoreMinimal.h"
#include "GridObjectAudio.generated.h"

class USoundBase;

/**
 * Data-driven audio event available to any grid object archetype.
 *
 * Event names live in UGridObjectArchetypeAsset::AudioEvents and deliberately
 * use FName keys rather than an enum so custom/player-authored objects can add
 * new semantic events without extending C++.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridObjectAudioEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float Volume = 1.0f;

	/**
	 * Symmetric deterministic pitch variation around 1.0.
	 * Mechanical events that must align with an animation should normally keep 0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float PitchVariation = 0.0f;


	bool HasPlayableSound() const
	{
		for (const TObjectPtr<USoundBase>& Sound : Sounds)
		{
			if (Sound)
			{
				return true;
			}
		}
		return false;
	}
};
