#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectPaletteAsset.generated.h"

class UTexture2D;
class UGridObjectArchetypeAsset;

USTRUCT (BlueprintType)
struct FGridObjectPaletteEntry
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FName EntryId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FName Category = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TObjectPtr<UGridObjectArchetypeAsset> DefaultArchetype = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    bool bPlaceAtCellCenter = true;
};

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridObjectPaletteAsset : public UDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TArray<FGridObjectPaletteEntry> Entries;

    const FGridObjectPaletteEntry* FindEntryById (FName EntryId) const
    {
        return Entries.FindByPredicate (
            [EntryId] (const FGridObjectPaletteEntry& Entry)
        {
            return Entry.EntryId == EntryId;
        });
    }
};