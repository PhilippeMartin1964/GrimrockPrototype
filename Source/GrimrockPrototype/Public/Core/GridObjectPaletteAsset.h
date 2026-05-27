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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (ToolTip = "Optional label override for this palette tile. Leave empty to use the archetype display name."))
    FText DisplayNameOverride;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette",
        meta = (ToolTip = "Optional grouping override for this palette tile. Leave empty to use the archetype category."))
    FName CategoryOverride = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TObjectPtr<UGridObjectArchetypeAsset> DefaultArchetype = nullptr;

    FName GetEffectiveArchetypeId () const
    {
        return DefaultArchetype ? DefaultArchetype->ArchetypeId : NAME_None;
    }

    EGridLevelObjectType GetEffectiveObjectType () const
    {
        return DefaultArchetype
            ? DefaultArchetype->SupportedType
            : EGridLevelObjectType::None;
    }

    FName GetEffectiveCategory () const
    {
        if (!CategoryOverride.IsNone ())
        {
            return CategoryOverride;
        }

        if (DefaultArchetype && !DefaultArchetype->Category.IsNone ())
        {
            return DefaultArchetype->Category;
        }

        return FName (TEXT ("Uncategorized"));
    }

    FText GetEffectiveDisplayName () const
    {
        if (!DisplayNameOverride.IsEmpty ())
        {
            return DisplayNameOverride;
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

    bool IsValidEntry () const
    {
        return !EntryId.IsNone () &&
            DefaultArchetype &&
            !DefaultArchetype->ArchetypeId.IsNone () &&
            DefaultArchetype->SupportedType != EGridLevelObjectType::None;
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

    bool ValidatePalette (TArray<FGridArchetypeValidationMessage>& OutMessages) const;
};
