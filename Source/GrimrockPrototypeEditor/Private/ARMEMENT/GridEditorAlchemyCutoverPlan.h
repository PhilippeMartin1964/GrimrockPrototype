#pragma once

#include "GridEditorAlchemyMigrationAudit.h"
#include "GridEditorAlchemyActionIdentity.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"

/** Joint transient projection from the exact X.62 native sources. No native
 * packages, saves or catalogues are written by this planner.
 */
namespace GridEditorAlchemyCutoverPlan
{
    namespace Audit = GridEditorAlchemyMigrationAudit;
    namespace Identity = GridEditorAlchemyActionIdentity;
    constexpr int32 BeforeCount = 38;
    constexpr int32 AfterCount = 48;
    constexpr int32 BeforeDetailedCount = 4;
    constexpr int32 AfterDetailedCount = 14;

    inline FName HistoricalAction(int32 Index)
    {
        static const FName Ids[] = {
            TEXT("Action_Alchemist_Antidote"), TEXT("Action_Alchemist_FireBomb"),
            TEXT("Action_Alchemist_ToxicBomb"), TEXT("Action_Alchemist_DefensiveElixir"),
            TEXT("Action_Alchemist_DefensiveElixir"), TEXT("Action_Alchemist_DefensiveElixir"),
            TEXT("Action_Alchemist_DefensiveElixir"), TEXT("Action_Alchemist_AcidFlask"),
            TEXT("Action_Alchemist_CorrosiveCloud"), TEXT("Action_Alchemist_Panacea"),
            TEXT("Action_Alchemist_OilSlick")
        };
        check(Index >= 0 && Index < Audit::TotalCount);
        return Ids[Index];
    }

    inline bool IsPotion(int32 Index)
    {
        return Index == 0 || (Index >= 3 && Index <= 6) || Index == 9;
    }

    inline bool SourceMatches(const Audit::FRow& Row, int32 Index)
    {
        const auto& Old = *Row.Source;
        const auto& Action = Row.Action;
        static const int32 AP[] = {1,2,2,1,1,1,1,2,3,2,2};
        static const int32 Cooldown[] = {0,0,0,0,0,0,0,1,2,3,0};
        const FName ElementAction = Identity::ActionForItem(Old.ItemDefinitionId);
        const bool bElixir = !ElementAction.IsNone();
        if (Old.ItemDefinitionId != Audit::Id(Index) ||
            Action.ActionId != HistoricalAction(Index) ||
            Action.ActionPointCost != AP[Index] ||
            Action.CooldownRounds != Cooldown[Index] ||
            Action.ResourceCosts.SourceItemQuantityCost != 1 ||
            Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
            !Action.PresentationProfileId.IsNone() || Old.bProvidesAttackPresentation ||
            Old.Weight != 0.f || Old.bStackable || Old.MaxStackSize != 1 ||
            Old.ItemType != (IsPotion(Index) ? EGridItemType::Potion : EGridItemType::Misc) ||
            GridCanonicalInventoryCategoryTags::Count(Old.ItemTags) != 0 ||
            Row.SameIdPeers.Num() != (bElixir ? 3 : 0))
            return false;
        if (bElixir)
        {
            const FName StatusId(*(TEXT("Status_") + Old.ItemDefinitionId.ToString().RightChop(5)));
            if (Action.StatusApplications.Num() != 1 ||
                Action.StatusApplications[0].StatusEffectId != StatusId ||
                Action.StatusApplications[0].DurationOverride != 3 ||
                Row.PeerDifferences.Num() != 1 ||
                !Row.PeerDifferences.Contains(TEXT("StatusApplications")))
                return false;
        }
        if (Index == Audit::LegacyCount)
            return Row.bRegistered && Row.bExistingCatalogueCollision &&
                Row.ExistingActionDifferences.IsEmpty() && Row.ProjectionReason.IsEmpty();
        return !Row.bRegistered && !Row.bExistingCatalogueCollision &&
            Row.bIsolatedParity && Old.Icon.IsNull() && Old.WorldMesh.IsNull() &&
            Old.EquippedMesh.IsNull();
    }

    inline bool Build(UGridItemSystemAsset& Root, UGridItemSystemAsset& Preview, FString& Error, bool bHistoricalView = false)
    {
        Error.Reset();
        // Never allow the destination to alias any persisted object.
        if (&Root == &Preview || Preview.GetOutermost() != GetTransientPackage() ||
            !Preview.ItemDefinitions.IsEmpty() || Preview.ActionCatalogue ||
            Preview.ActionPresentationCatalogue)
        {
            Error = TEXT("Expected a new empty transient preview root");
            return false;
        }
        TArray<Audit::FRow> Rows;
        if (!Audit::Build(Root, Rows, Error, bHistoricalView)) return false;
        if (Rows.Num() != Audit::TotalCount || Root.ItemDefinitions.Num() != BeforeCount ||
            Root.ActionCatalogue->DetailedActions.Num() != BeforeDetailedCount ||
            Root.ActionCatalogue->Actions.Num() != 19 ||
            Root.ActionPresentationCatalogue->Presentations.Num() != 5 ||
            Root.ActionPresentationCatalogue->DetailedPresentations.Num() != 1)
        {
            Error = TEXT("X.62 root/catalogue cardinality changed");
            return false;
        }
        for (int32 I = 0; I < Rows.Num(); ++I)
            if (!SourceMatches(Rows[I], I))
            {
                Error = TEXT("X.62 source evidence changed: ") + Audit::Id(I).ToString();
                return false;
            }

        Audit::CopyRoot(Root, Preview);
        auto* Actions = NewObject<UGridActionCatalogueAsset>(&Preview, NAME_None, RF_Transient);
        Actions->Actions = Root.ActionCatalogue->Actions;
        Actions->DetailedActions = Root.ActionCatalogue->DetailedActions;
        Preview.ActionCatalogue = Actions;
        auto* Presentations = NewObject<UGridActionPresentationCatalogueAsset>(&Preview, NAME_None, RF_Transient);
        Presentations->Presentations = Root.ActionPresentationCatalogue->Presentations;
        Presentations->DetailedPresentations = Root.ActionPresentationCatalogue->DetailedPresentations;
        Preview.ActionPresentationCatalogue = Presentations;

        TArray<FGridCombatActionDefinition> Expected;
        for (int32 I = 0; I < Audit::LegacyCount; ++I)
        {
            auto Action = Rows[I].Action;
            const auto& Old = *Rows[I].Source;
            const FName ElementAction = Identity::ActionForItem(Old.ItemDefinitionId);
            if (!ElementAction.IsNone()) Action.ActionId = ElementAction;
            auto Differences = Audit::DifferentActionFields(Rows[I].Action, Action);
            if ((!ElementAction.IsNone() && (Differences.Num() != 1 || !Differences.Contains(TEXT("ActionId")))) ||
                (ElementAction.IsNone() && !Differences.IsEmpty()))
            {
                Error = TEXT("Unexpected mechanical mutation: ") + Old.ItemDefinitionId.ToString();
                return false;
            }
            if (!Audit::AppendCandidate(Old, Action, Preview, Error)) return false;
            auto* Item = Cast<UGridConsumableDefinitionAsset>(Preview.ItemDefinitions.Last().Get());
            if (!Item) return false;
            if (IsPotion(I)) Item->Tags.AddUnique(GridCanonicalInventoryCategoryTags::Potion());
            Expected.Add(MoveTemp(Action));
        }
        if (Preview.ItemDefinitions.Num() != AfterCount ||
            Preview.ActionCatalogue->DetailedActions.Num() != AfterDetailedCount ||
            Preview.ActionCatalogue->Actions.Num() != Root.ActionCatalogue->Actions.Num() ||
            Preview.ActionPresentationCatalogue->Presentations.Num() != Root.ActionPresentationCatalogue->Presentations.Num() ||
            Preview.ActionPresentationCatalogue->DetailedPresentations.Num() != Root.ActionPresentationCatalogue->DetailedPresentations.Num() ||
            Preview.ActionCatalogue->Find(Identity::SharedLegacyAction()) ||
            Preview.ActionCatalogue->FindDetailed(Identity::SharedLegacyAction()))
        {
            Error = TEXT("Joint projection has wrong counts or retained shared alias");
            return false;
        }
        for (int32 I = 0; I < BeforeCount; ++I)
            if (Preview.ItemDefinitions[I] != Root.ItemDefinitions[I])
            {
                Error = TEXT("Existing ordered item identity changed");
                return false;
            }
        for (int32 I = 0; I < Audit::LegacyCount; ++I)
            if (!Audit::ResolveExact(*Rows[I].Source, Expected[I], Preview, Error,
                    IsPotion(I) ? GridCanonicalInventoryCategoryTags::Potion() : NAME_None))
                return false;
        TArray<FString> Validation;
        if (!Preview.Validate(Validation))
        {
            Error = FString::Join(Validation, TEXT("; "));
            return false;
        }
        return true;
    }
    inline FString TargetName(int32 I) { return TEXT("DA_Canonical_") + Audit::Id(I).ToString(); }
    inline FString TargetPackage(int32 I)
    {
        return TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/") + TargetName(I);
    }
    inline FString TargetPath(int32 I) { return TargetPackage(I) + TEXT(".") + TargetName(I); }

    // Compare every reflected item property and every appended action, never
    // merely accept a root with 48 items. The prefix must stay in order.
    inline bool MatchesProjected(const UGridItemSystemAsset& Actual,
        const UGridItemSystemAsset& Expected, FString& Error, bool bNativePaths)
    {
        if (Actual.ItemDefinitions.Num() != AfterCount || Expected.ItemDefinitions.Num() != AfterCount ||
            !Actual.ActionCatalogue || !Expected.ActionCatalogue ||
            Actual.ActionCatalogue->DetailedActions.Num() != AfterDetailedCount ||
            Expected.ActionCatalogue->DetailedActions.Num() != AfterDetailedCount ||
            Actual.WeaponCatalogue != Expected.WeaponCatalogue ||
            Actual.WeaponVisualCatalogue != Expected.WeaponVisualCatalogue)
        { Error = TEXT("Wrong projected root cardinality or catalogue identity"); return false; }
        for (int32 I = 0; I < BeforeCount; ++I)
            if (Actual.ItemDefinitions[I] != Expected.ItemDefinitions[I])
            { Error = TEXT("Original definition prefix changed"); return false; }
        for (int32 I = 0; I < Audit::LegacyCount; ++I)
        {
            const auto* A = Cast<UGridConsumableDefinitionAsset>(Actual.ItemDefinitions[BeforeCount+I].Get());
            const auto* E = Cast<UGridConsumableDefinitionAsset>(Expected.ItemDefinitions[BeforeCount+I].Get());
            if (!IsValid(A) || !IsValid(E) || (bNativePaths && A->GetPathName() != TargetPath(I)) ||
                !FGridEditorLootDefinitionProjection::SameDefinition(*A, *E))
            { Error = TEXT("Projected native item differs: ") + Audit::Id(I).ToString(); return false; }
        }
        for (int32 I = 0; I < AfterDetailedCount; ++I)
            if (!Audit::DifferentActionFields(Actual.ActionCatalogue->DetailedActions[I].Definition,
                    Expected.ActionCatalogue->DetailedActions[I].Definition).IsEmpty())
            { Error = TEXT("Projected detailed action differs"); return false; }
        TArray<FString> Validation;
        if (!Actual.Validate(Validation))
        { Error = FString::Join(Validation, TEXT("; ")); return false; }
        return true;
    }

    inline bool HistoricalView(UGridItemSystemAsset& Root, UGridItemSystemAsset& Historical, FString& Error)
    {
        if (&Root == &Historical || Historical.GetOutermost() != GetTransientPackage() ||
            !Historical.ItemDefinitions.IsEmpty() || Historical.ActionCatalogue ||
            !Root.ActionCatalogue || Root.ItemDefinitions.Num() != AfterCount ||
            Root.ActionCatalogue->DetailedActions.Num() != AfterDetailedCount)
        { Error = TEXT("Only an exact post-cutover root may produce a historical view"); return false; }
        Audit::CopyRoot(Root, Historical);
        Historical.ItemDefinitions.SetNum(BeforeCount);
        auto* Actions = NewObject<UGridActionCatalogueAsset>(&Historical, NAME_None, RF_Transient);
        Actions->Actions = Root.ActionCatalogue->Actions;
        Actions->DetailedActions = Root.ActionCatalogue->DetailedActions;
        Actions->DetailedActions.SetNum(BeforeDetailedCount);
        Historical.ActionCatalogue = Actions;
        return true;
    }

    inline bool BuildForCurrentState(UGridItemSystemAsset& Root, UGridItemSystemAsset& Preview, FString& Error)
    {
        if (Root.GetPathName() != FString(Audit::RootPath()) || Root.GetOutermost()->IsDirty() ||
            !Root.ActionCatalogue || Root.ActionCatalogue->GetOutermost()->IsDirty())
        { Error = TEXT("Expected clean, exact native alchemy source root"); return false; }
        if (Root.ItemDefinitions.Num() == BeforeCount) return Build(Root, Preview, Error);
        TStrongObjectPtr<UGridItemSystemAsset> Historical(NewObject<UGridItemSystemAsset>());
        if (!HistoricalView(Root, *Historical.Get(), Error) ||
            !Build(*Historical.Get(), Preview, Error, true)) return false;
        for (int32 I = BeforeCount; I < AfterCount; ++I)
            if (!IsValid(Root.ItemDefinitions[I].Get()) || Root.ItemDefinitions[I]->GetOutermost()->IsDirty())
            { Error = TEXT("Dirty or missing persisted alchemy item"); return false; }
        return MatchesProjected(Root, Preview, Error, true);
    }
}
