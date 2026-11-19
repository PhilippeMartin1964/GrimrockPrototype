#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Items/GridEquippedWeaponActionIconTypes.h"
#include "GridEquippedWeaponActionIconPalette.generated.h"

class UTexture2D;

/** Historical serialized map, retained only for migration evidence until 5G final. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridEquippedWeaponActionIconPalette : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon Action Icons")
	TMap<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>> Icons;

};
