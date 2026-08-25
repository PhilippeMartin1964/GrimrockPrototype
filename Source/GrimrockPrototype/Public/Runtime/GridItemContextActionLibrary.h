#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/GridItemActionTypes.h"
#include "GridItemContextActionLibrary.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API UGridItemContextActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	static bool BuildInventorySlotContextActions(AGrimrockPartyPawn* PartyPawn, int32 CharacterIndex, int32 InventorySlotIndex,
		FGridFacingTargetContext& OutFacingTarget, TArray<FGridItemContextAction>& OutActions);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	static bool BuildItemContextActions(
		const FGridItemActionContext& ItemContext, FGridFacingTargetContext& OutFacingTarget, TArray<FGridItemContextAction>& OutActions);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	static bool ResolveFacingTarget(AGrimrockPartyPawn* PartyPawn, const FGridItemInstance& CurrentItem, UGridItemDefinitionAsset* ItemDefinition,
		FGridFacingTargetContext& OutFacingTarget, float TraceDistance = 300.f);
};
