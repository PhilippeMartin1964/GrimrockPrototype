#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridObjectArchetypeAsset.h"
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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (ToolTip = "Optional explicit archetype id for entries backed by an archetype referenced elsewhere. DefaultArchetype takes precedence when set."))
    FName ArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (AdvancedDisplay, ToolTip = "Legacy compatibility flag only. Archetype PlacementKind is now the source of truth."))
    bool bPlaceOnEdge = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (AdvancedDisplay, ToolTip = "Legacy compatibility flag only. Archetype PlacementKind is now the source of truth."))
    bool bPlaceAtCellCenter = true;

    FName GetEffectiveArchetypeId () const
    {
        return DefaultArchetype ? DefaultArchetype->ArchetypeId : ArchetypeId;
    }

    EGridLevelObjectType GetEffectiveObjectType () const
    {
        return DefaultArchetype && DefaultArchetype->SupportedType != EGridLevelObjectType::None
            ? DefaultArchetype->SupportedType
            : ObjectType;
    }

    FName GetEffectiveCategory () const
    {
        return DefaultArchetype && !DefaultArchetype->Category.IsNone ()
            ? DefaultArchetype->Category
            : Category;
    }

    FText GetEffectiveDisplayName () const
    {
        if (!DisplayName.IsEmpty ())
        {
            return DisplayName;
        }

        if (DefaultArchetype && !DefaultArchetype->DisplayName.IsEmpty ())
        {
            return DefaultArchetype->DisplayName;
        }

        const FName EffectiveArchetypeId = GetEffectiveArchetypeId ();
        return !EffectiveArchetypeId.IsNone ()
            ? FText::FromName (EffectiveArchetypeId)
            : FText::FromName (EntryId);
    }
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
