#pragma once

#include "GridEditorRetirementAudit.h"
#include "Runtime/Items/GridCanonicalPuzzleCategoryTags.h"

namespace GridEditorPuzzleCategoryMigration
{
    // Frozen native evidence: issue #208, validated SHA eb22e645fd12.
    // This manifest is editor migration input only, never a runtime lookup.
    inline const TMap<FName, FName>& Manifest()
    {
        static const TMap<FName, FName> Rows = {
            { TEXT("EmptyVial"), TEXT("Component") },
            { TEXT("Gem_Blue"), TEXT("Gem") },
            { TEXT("GoblinKnife"), TEXT("Weapon") },
            { TEXT("Item_Antidote"), TEXT("Potion") },
            { TEXT("Item_Arrows_01"), TEXT("Misc") },
            { TEXT("Item_Bomb_Fire"), TEXT("Misc") },
            { TEXT("Item_Bomb_Toxic"), TEXT("Misc") },
            { TEXT("Item_DefensiveElixir_Fire"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Ice"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Lightning"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Poison"), TEXT("Potion") },
            { TEXT("Item_DisarmTools_01"), TEXT("Misc") },
            { TEXT("Item_Flask_Acid"), TEXT("Misc") },
            { TEXT("Item_Flask_CorrosiveCloud"), TEXT("Misc") },
            { TEXT("Item_Flask_Oil"), TEXT("Misc") },
            { TEXT("Item_Panacea"), TEXT("Potion") },
            { TEXT("Item_Potion_Healing_01"), TEXT("Potion") },
            { TEXT("Item_RatMeat"), TEXT("Food") },
            { TEXT("Item_RatTooth"), TEXT("Component") },
            { TEXT("Item_SurfaceCatalyst_01"), TEXT("Misc") },
            { TEXT("Item_TestNote"), TEXT("Scroll") },
            { TEXT("Item_Torch"), TEXT("Torch") },
            { TEXT("Key_Copper"), TEXT("Key") },
            { TEXT("Key_Iron"), TEXT("Key") },
            { TEXT("Shuriken"), TEXT("Weapon") },
            { TEXT("Stone"), TEXT("Misc") },
        };
        return Rows;
    }

    inline bool PlanTags(const TArray<FName>& Before, FName Type, TArray<FName>& After)
    {
        After.Reset();
        const FName Tag = GridCanonicalPuzzleCategoryTags::ForType(Type);
        if (Tag.IsNone() || GridCanonicalPuzzleCategoryTags::CountReserved(Before) != 0) return false;
        After = Before;
        After.Add(Tag);
        return true;
    }

    inline bool BuildPreview(UGridItemSystemAsset& Root, UGridItemSystemAsset& Preview, FString& Error)
    {
        auto Fail = [&](const FString& Message) { Error = Message; return false; };
        if (&Root == &Preview || Preview.GetOutermost() != GetTransientPackage() ||
            !Preview.ItemDefinitions.IsEmpty() || Preview.WeaponCatalogue || Preview.WeaponVisualCatalogue ||
            Preview.ActionCatalogue || Preview.ActionPresentationCatalogue)
            return Fail(TEXT("Preview must be an empty transient destination"));
        GridEditorRetirementAudit::FNativeAudit Audit;
        if (!GridEditorRetirementAudit::CollectNative(Audit, Error)) return false;
        if (Root.GetPathName() != GridEditorRetirementAudit::RootPath() ||
            Root.ItemDefinitions.Num() != 48 ||
            Audit.Report->GetNumberField(TEXT("puzzle_level_assets_scanned")) != 4 ||
            Audit.Report->GetArrayField(TEXT("puzzle_type_links")).Num() != 0)
            return Fail(TEXT("Native root/level baseline changed"));
        TSet<FName> Seen;
        int32 Arms = 0;
        for (const TSharedPtr<FJsonValue>& Value : Audit.Report->GetArrayField(TEXT("puzzle_category_sources")))
        {
            const auto Row = Value->AsObject();
            const FName Id(*Row->GetStringField(TEXT("item_id")));
            if (Seen.Contains(Id) || !Row->GetBoolField(TEXT("canonical_id_exists")))
                return Fail(TEXT("Duplicate or uncovered category evidence: ") + Id.ToString());
            Seen.Add(Id);
            if (Row->GetBoolField(TEXT("armament")))
            {
                if (!Row->GetBoolField(TEXT("arm_type_matches")) || Manifest().Contains(Id))
                    return Fail(TEXT("Armament category changed: ") + Id.ToString());
                ++Arms;
            }
            else
            {
                const FName* Type = Manifest().Find(Id);
                if (!Type || *Type != FName(*Row->GetStringField(TEXT("legacy_type"))))
                    return Fail(TEXT("Non-ARM category evidence changed: ") + Id.ToString());
            }
        }
        if (Seen.Num() != 48 || Arms != 22 || Manifest().Num() != 26)
            return Fail(TEXT("Unexpected native evidence counts"));
        IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        FARFilter Filter;
        Filter.PackagePaths.Add(FName(TEXT("/Game")));
        Filter.bRecursivePaths = true;
        Filter.ClassPaths.Add(UGridLevelAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Levels;
        Registry.GetAssets(Filter, Levels);
        for (const FAssetData& Asset : Levels)
        {
            const auto* Level = Cast<UGridLevelAsset>(Asset.GetAsset());
            if (!IsValid(Level)) return Fail(TEXT("Level unavailable"));
            for (const FGridObjectLink& Link : Level->Links)
                if (Link.Condition == EGridObjectCondition::ReceptacleContainsItemTag &&
                    GridCanonicalPuzzleCategoryTags::IsReserved(Link.ConditionItemTag))
                    return Fail(TEXT("Existing puzzle tag condition requires explicit review"));
        }

        Preview.WeaponCatalogue = Root.WeaponCatalogue;
        Preview.WeaponVisualCatalogue = Root.WeaponVisualCatalogue;
        Preview.ActionCatalogue = Root.ActionCatalogue;
        Preview.ActionPresentationCatalogue = Root.ActionPresentationCatalogue;
        Preview.ItemDefinitions.Reset();
        int32 Added = 0;
        int32 Present = 0;
        for (const auto& Source : Root.ItemDefinitions)
        {
            if (!IsValid(Source.Get()) || Source->GetOutermost()->IsDirty() || !Seen.Contains(Source->ItemId))
                return Fail(TEXT("Invalid, dirty or unknown source definition"));
            const FName* Type = Manifest().Find(Source->ItemId);
            if (!Type)
            {
                if (!Source->IsA<UGridArmamentDefinitionAsset>() ||
                    GridCanonicalPuzzleCategoryTags::CountReserved(Source->Tags) != 0)
                    return Fail(TEXT("Unexpected ARM category metadata"));
                Preview.ItemDefinitions.Add(Source);
                continue;
            }
            const FString Name = TEXT("DA_Canonical_") + Source->ItemId.ToString();
            if (!Source->IsA<UGridNonArmamentDefinitionAsset>() ||
                Source->GetPathName() != TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/") + Name + TEXT(".") + Name)
                return Fail(TEXT("Unexpected native target: ") + Source->GetPathName());
            TArray<FName> Tags;
            if (GridCanonicalPuzzleCategoryTags::CountReserved(Source->Tags) == 0)
            {
                if (!PlanTags(Source->Tags, *Type, Tags)) return Fail(TEXT("Invalid planned category: ") + Name);
                ++Added;
            }
            else
            {
                const TArray<FName> Original = GridCanonicalPuzzleCategoryTags::WithoutReserved(Source->Tags);
                if (!GridCanonicalPuzzleCategoryTags::MatchesBeforeOrAfter(Source->Tags, Original, *Type))
                    return Fail(TEXT("Invalid native category tag: ") + Name);
                Tags = Source->Tags;
                ++Present;
            }
            auto* Copy = DuplicateObject<UGridCanonicalItemDefinitionAsset>(Source.Get(), &Preview);
            if (!Copy) return Fail(TEXT("Preview duplication failed"));
            Copy->Tags = MoveTemp(Tags);
            Preview.ItemDefinitions.Add(Copy);
        }
        if (!((Added == 26 && Present == 0) || (Added == 0 && Present == 26)) ||
            Root.GetOutermost()->IsDirty())
            return Fail(TEXT("Incomplete or dirty preview"));
        return true;
    }
}
