#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GrimrockSaveSlotLibrary.generated.h"

class AGrimrockPartyPawn;

/**
 * Blueprint helpers for saving the current party into explicit save slots.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockSaveSlotLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RPG|Save")
    static bool SetPartySaveSlot(
        AGrimrockPartyPawn* PartyPawn,
        const FString& SlotName,
        int32 UserIndex,
        UPARAM(ref) FText& OutError);

    UFUNCTION(BlueprintCallable, Category = "RPG|Save")
    static bool SavePartyGameToSlot(
        AGrimrockPartyPawn* PartyPawn,
        const FString& SlotName,
        int32 UserIndex,
        UPARAM(ref) FText& OutError);

    UFUNCTION(BlueprintCallable, Category = "RPG|Save")
    static bool SavePartyGameCopyToSlot(
        AGrimrockPartyPawn* PartyPawn,
        const FString& SlotName,
        int32 UserIndex,
        UPARAM(ref) FText& OutError);
};
