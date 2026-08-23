#include "RPG/RPGStoryCompanionAsset.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"

namespace RPGStoryCompanionAssetPrivate
{
    bool AreStartingEquipmentIdsValid (const TArray<FName>& ItemIds)
    {
        TSet<FName> Seen;
        for (const FName ItemId : ItemIds)
        {
            if (ItemId.IsNone () || Seen.Contains (ItemId))
            {
                return false;
            }
            Seen.Add (ItemId);
        }
        return true;
    }
}

bool URPGStoryCompanionAsset::IsValidDefinition () const
{
    if (CompanionId.IsNone () || !CharacterId.IsValid ())
    {
        return false;
    }

    FString NormalizedName = DisplayName.ToString ();
    NormalizedName.TrimStartAndEndInline ();
    if (NormalizedName.Len () < 1 || NormalizedName.Len () > 24)
    {
        return false;
    }

    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () ||
        !ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    if (Level < URPGCharacterRulesLibrary::GetMinimumLevel () ||
        Level > URPGCharacterRulesLibrary::GetMaximumLevel ())
    {
        return false;
    }

    const FRPGAttributes FinalAttributes =
        URPGCharacterRulesLibrary::AddAttributes (
            ClassDefinition->BaseAttributes,
            RaceDefinition->AttributeBonuses);
    if (!URPGCharacterRulesLibrary::AreAttributesInRange (FinalAttributes))
    {
        return false;
    }

    return RPGStoryCompanionAssetPrivate::AreStartingEquipmentIdsValid (
        StartingEquipmentIds);
}

void URPGStoryCompanionAsset::GenerateCharacterId ()
{
    if (CharacterId.IsValid ())
    {
        return;
    }

    Modify ();
    CharacterId = FGuid::NewGuid ();
    MarkPackageDirty ();
}
