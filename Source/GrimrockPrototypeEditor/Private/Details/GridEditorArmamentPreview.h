#pragma once
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace GridEditorArmamentPreview
{
    /** A transient registry with the configured root, never a second data source.
     * Refuse an unindexed new/duplicate asset rather than displaying another
     * weapon's values based only on a matching name or archetype ID. */
    inline bool Resolve(UGridItemSystemAsset* Root, const UGridArmamentDefinitionAsset* Item,
        FGridResolvedItem& N1, FGridResolvedItem& N20, FString& Error)
    {
        N1 = FGridResolvedItem();
        N20 = FGridResolvedItem();
        Error.Reset();
        if (!IsValid(Root) || !IsValid(Item))
        {
            Error = TEXT("Registre canonique indisponible.");
            return false;
        }
        int32 MatchingIds = 0;
        bool bExactObject = false;
        for (const auto& Definition : Root->ItemDefinitions)
        {
            if (Definition && Definition->ItemId == Item->ItemId)
            {
                ++MatchingIds;
                bExactObject |= Definition.Get() == Item;
            }
        }
        if (MatchingIds != 1 || !bExactObject)
        {
            Error = TEXT("Définition non enregistrée ou ItemId en conflit : vérifier DA_ItemSystem.");
            return false;
        }
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
        TArray<FString> Errors;
        FGridResolvedItem Low, High;
        if (!Registry->RegisterItemSystem(Root, Errors) ||
            !Registry->ResolveItem(Item->ItemId, 1, Low) ||
            !Registry->ResolveItem(Item->ItemId, 20, High) ||
            !Low.bIsArmament || !High.bIsArmament ||
            Low.ArchetypeId != Item->ArchetypeId || High.ArchetypeId != Item->ArchetypeId)
        {
            Error = TEXT("Configuration canonique invalide : vérifier les quatre catalogues.");
            return false;
        }
        N1 = MoveTemp(Low);
        N20 = MoveTemp(High);
        return true;
    }
}
