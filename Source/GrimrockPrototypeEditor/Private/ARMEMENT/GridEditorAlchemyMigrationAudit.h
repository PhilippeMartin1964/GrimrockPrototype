#pragma once

#include "GridEditorLootDefinitionProjection.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "UObject/StrongObjectPtr.h"

/** Read-only evidence for the ten unregistered alchemy items and the already
 * canonical Oil. No action identities are renamed and no packages are saved.
 * A valid report is not authorization for migration.
 */
namespace GridEditorAlchemyMigrationAudit
{
    constexpr int32 LegacyCount = 10;
    constexpr int32 TotalCount = 11;
    inline const TCHAR* RootPath()
    {
        return TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    }
    inline FName Id(int32 Index)
    {
        static const FName Ids[] = {
            TEXT("Item_Antidote"), TEXT("Item_Bomb_Fire"), TEXT("Item_Bomb_Toxic"),
            TEXT("Item_DefensiveElixir_Fire"), TEXT("Item_DefensiveElixir_Ice"),
            TEXT("Item_DefensiveElixir_Lightning"), TEXT("Item_DefensiveElixir_Poison"),
            TEXT("Item_Flask_Acid"), TEXT("Item_Flask_CorrosiveCloud"), TEXT("Item_Panacea"),
            TEXT("Item_Flask_Oil")
        };
        check(Index >= 0 && Index < TotalCount);
        return Ids[Index];
    }

    inline TArray<FString> DifferentActionFields(const FGridCombatActionDefinition& A,
        const FGridCombatActionDefinition& B)
    {
        TArray<FString> Differences;
        for (TFieldIterator<FProperty> Field(FGridCombatActionDefinition::StaticStruct()); Field; ++Field)
            if (!Field->Identical_InContainer(&A, &B))
                Differences.Add(Field->GetName());
        return Differences;
    }

    inline FString ActionExport(const FGridCombatActionDefinition& Action)
    {
        FString Result;
        for (TFieldIterator<FProperty> Field(FGridCombatActionDefinition::StaticStruct()); Field; ++Field)
        {
            FString Value;
            Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(&Action),
                nullptr, nullptr, PPF_None);
            const FString Name = Field->GetName();
            Result += FString::Printf(TEXT("%d:%s=%d:%s\n"), Name.Len(), *Name, Value.Len(), *Value);
        }
        return Result;
    }

    inline void CopyRoot(const UGridItemSystemAsset& Source, UGridItemSystemAsset& Copy)
    {
        Copy.WeaponCatalogue = Source.WeaponCatalogue;
        Copy.WeaponVisualCatalogue = Source.WeaponVisualCatalogue;
        Copy.ActionCatalogue = Source.ActionCatalogue;
        Copy.ActionPresentationCatalogue = Source.ActionPresentationCatalogue;
        Copy.ItemDefinitions = Source.ItemDefinitions;
    }

    /** Build a single isolated candidate. An ID collision is NOT repaired
     * by guessing a new action id, reusing another item's effect or adding
     * an alternate runtime authority. Full reflected action comparison.
     */
    inline bool AppendCandidate(const UGridItemDefinitionAsset& Old,
        const FGridCombatActionDefinition& Action, UGridItemSystemAsset& Preview,
        FString& Reason)
    {
        using FCopy = FGridEditorLootDefinitionProjection;
        Reason.Reset();
        if (Old.bUseWeaponCatalogue || Old.bUseWeaponVisualCatalogue ||
            !Old.WeaponArchetypeId.IsNone() || !Old.RequiredAmmunitionItemDefinitionId.IsNone() ||
            Old.GetEffectiveHandUsage() != EGridItemHandUsage::NotHandHeld ||
            !Old.CompatibleEquipmentSlots.IsEmpty() || Old.IsPhysicallyThrowable() ||
            Old.IsCombatThrowable() || !Old.CombatActions.IsEmpty() ||
            !Old.bProvidesQuickItemCombatAction ||
            (Old.ItemType != EGridItemType::Potion && Old.ItemType != EGridItemType::Misc) ||
            (Old.bStackable && Old.MaxStackSize < 1))
        {
            Reason = TEXT("Source has non-QuickItem or unsupported physical semantics");
            return false;
        }
        if (Preview.ActionCatalogue->Find(Action.ActionId) ||
            Preview.ActionCatalogue->FindDetailed(Action.ActionId))
        {
            Reason = TEXT("ActionId already exists in candidate catalogue");
            return false;
        }

        FName PresentationId = Action.PresentationProfileId;
        // For the audit only, an unbound no-FX action uses the already
        // existing canonical no-FX presentation. The action itself is
        // NOT edited, including a genuinely empty PresentationProfileId.
        if (Old.bProvidesAttackPresentation)
        {
            if (!Old.HasValidPlayerAttackPresentation())
            {
                Reason = TEXT("Invalid authored attack presentation");
                return false;
            }
            if (PresentationId.IsNone())
                PresentationId = FName(*(TEXT("PRES_AuditOnly_") + Old.ItemDefinitionId.ToString()));
            if (Preview.ActionPresentationCatalogue->Find(PresentationId))
            {
                Reason = TEXT("Authored detailed presentation collides with compact profile");
                return false;
            }
            const auto* Present = Preview.ActionPresentationCatalogue->FindDetailed(PresentationId);
            if (Present && !FGridPlayerAttackPresentationProfile::StaticStruct()->CompareScriptStruct(
                    Present, &Old.PlayerAttackPresentationProfile, 0))
            {
                Reason = TEXT("PresentationId has different authored profile");
                return false;
            }
            if (!Present)
            {
                FGridCanonicalRuntimePresentation New;
                New.PresentationId = PresentationId;
                New.Profile = Old.PlayerAttackPresentationProfile;
                Preview.ActionPresentationCatalogue->DetailedPresentations.Add(MoveTemp(New));
            }
        }
        else if (PresentationId.IsNone())
        {
            PresentationId = TEXT("PRES_AttackNoFX");
        }
        if (!Preview.ActionPresentationCatalogue->Find(PresentationId) &&
            !Preview.ActionPresentationCatalogue->FindDetailed(PresentationId))
        {
            Reason = TEXT("Presentation reference not registered");
            return false;
        }

        auto* New = NewObject<UGridConsumableDefinitionAsset>(&Preview, NAME_None, RF_Transient);
        New->ItemId = Old.ItemDefinitionId;
        New->DisplayName = Old.DisplayName;
        New->Description = Old.Description;
        New->WeightKg = Old.Weight;
        New->Tags = Old.ItemTags;
        New->Icon = Old.Icon;
        New->WorldMesh = Old.WorldMesh;
        New->MaxStack = Old.bStackable ? Old.MaxStackSize : 1;
        New->bReadable = Old.ItemTags.Contains(FName(TEXT("Readable"))) ||
            Old.ItemTags.Contains(FName(TEXT("Lisible")));
        const TCHAR* Fields[] = {
            TEXT("ReadText"), TEXT("LightEmitter"), TEXT("VisualEffects"),
            TEXT("bEnableWorldSparkle"), TEXT("WorldSparkleMaterial"),
            TEXT("WorldSparkleColor"), TEXT("WorldSparkleIntensity"), TEXT("WorldSparkleSpeed"),
            TEXT("WorldSparkleVariation"), TEXT("bUseItemWeightAsWorldPhysicsMass"),
            TEXT("WorldPhysicsInitialTiltDegrees")
        };
        for (const TCHAR* Field : Fields)
            if (!FCopy::CopyExact(Old, *New, Field, Field))
            {
                Reason = FString::Printf(TEXT("Cannot copy reflected field %s"), Field);
                return false;
            }
        if (!FCopy::CopyThrow(Old, New->GenericThrow))
        {
            Reason = TEXT("Unmapped throw policy");
            return false;
        }
        FGridItemActionBinding Binding;
        Binding.ActionId = Action.ActionId;
        Binding.PresentationId = PresentationId;
        New->UseActions.Add(Binding);
        FGridCanonicalRuntimeAction Entry;
        Entry.Definition = Action;
        Preview.ActionCatalogue->DetailedActions.Add(MoveTemp(Entry));
        Preview.ItemDefinitions.Add(New);
        return true;
    }

    inline bool ResolveExact(const UGridItemDefinitionAsset& Old,
        const FGridCombatActionDefinition& Action, UGridItemSystemAsset& Candidate,
        FString& Reason)
    {
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
        TArray<FString> Errors;
        if (!Registry->RegisterItemSystem(&Candidate, Errors))
        {
            Reason = FString::Join(Errors, TEXT("; "));
            return false;
        }
        FGridResolvedItem Item;
        FGridCombatActionDefinition Actual;
        if (!Registry->ResolveItem(Old.ItemDefinitionId, 0, Item) || !Item.IsValid() ||
            Item.bIsArmament || Item.ItemLevel != 0 ||
            Item.ItemId != Old.ItemDefinitionId || Item.WeightKg != Old.Weight ||
            !Item.DisplayName.EqualTo(Old.DisplayName) || !Item.Description.EqualTo(Old.Description) ||
            Item.Tags != Old.ItemTags || Item.MaxStack != (Old.bStackable ? Old.MaxStackSize : 1) ||
            Item.ItemIcon.ToSoftObjectPath() != Old.Icon.ToSoftObjectPath() ||
            Item.WorldMesh.ToSoftObjectPath() != Old.WorldMesh.ToSoftObjectPath() ||
            !Item.NonArmEquipmentSlots.IsEmpty() || Item.bNonArmTwoHanded ||
            !Item.ReadText.EqualTo(Old.ReadText) ||
            Item.bEnableWorldSparkle != Old.bEnableWorldSparkle ||
            Item.bUseItemWeightAsWorldPhysicsMass != Old.bUseItemWeightAsWorldPhysicsMass ||
            Item.WorldPhysicsInitialTiltDegrees != Old.WorldPhysicsInitialTiltDegrees ||
            Item.AvailableActions.Num() != 1 ||
            !Registry->ResolveDetailedNonArmAction(Old.ItemDefinitionId, 0, Action.ActionId, Actual))
        {
            Reason = TEXT("Canonical N0 identity/action/metadata did not resolve exactly");
            return false;
        }
        const auto Differences = DifferentActionFields(Action, Actual);
        if (!Differences.IsEmpty())
        {
            Reason = FString::Join(Differences, TEXT(", "));
            return false;
        }
        return true;
    }

    inline bool Probe(const UGridItemSystemAsset& Root, const UGridItemDefinitionAsset& Old,
        const FGridCombatActionDefinition& Action, FString& Reason)
    {
        TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
        CopyRoot(Root, *Preview.Get());
        auto* Actions = NewObject<UGridActionCatalogueAsset>(Preview.Get(), NAME_None, RF_Transient);
        Actions->Actions = Root.ActionCatalogue->Actions;
        Actions->DetailedActions = Root.ActionCatalogue->DetailedActions;
        Preview->ActionCatalogue = Actions;
        auto* Presentations = NewObject<UGridActionPresentationCatalogueAsset>(
            Preview.Get(), NAME_None, RF_Transient);
        Presentations->Presentations = Root.ActionPresentationCatalogue->Presentations;
        Presentations->DetailedPresentations = Root.ActionPresentationCatalogue->DetailedPresentations;
        Preview->ActionPresentationCatalogue = Presentations;
        return AppendCandidate(Old, Action, *Preview.Get(), Reason) &&
            ResolveExact(Old, Action, *Preview.Get(), Reason);
    }

    struct FRow
    {
        const UGridItemDefinitionAsset* Source = nullptr;
        FGridCombatActionDefinition Action;
        bool bRegistered = false;
        bool bIsolatedParity = false;
        bool bExistingCatalogueCollision = false;
        FString ProjectionReason;
        TArray<FString> ExistingActionDifferences;
        TArray<int32> SameIdPeers;
        TArray<FString> PeerDifferences;
    };

    inline bool Build(UGridItemSystemAsset& Root, TArray<FRow>& Rows, FString& Error)
    {
        Rows.Reset();
        Error.Reset();
        TArray<FString> Validation;
        if (Root.GetPathName() != FString(RootPath()) ||
            Root.ItemDefinitions.Num() != 38 || !Root.Validate(Validation))
        {
            Error = TEXT("Expected valid 38-definition X.61B root: ") + FString::Join(Validation, TEXT("; "));
            return false;
        }
        const UObject* Protected[] = {&Root, Root.WeaponCatalogue.Get(),
            Root.WeaponVisualCatalogue.Get(), Root.ActionCatalogue.Get(),
            Root.ActionPresentationCatalogue.Get()};
        for (const UObject* Object : Protected)
            if (!IsValid(Object) || Object->GetOutermost()->IsDirty())
            {
                Error = TEXT("Dirty or missing canonical root/catalogue");
                return false;
            }
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
        if (!Registry->RegisterItemSystem(&Root, Validation))
        {
            Error = FString::Join(Validation, TEXT("; "));
            return false;
        }
        for (int32 I = 0; I < TotalCount; ++I)
        {
            FRow Row;
            const FString Path = FRPGAlchemistAuthoring::GetItemObjectPath(Id(I));
            Row.Source = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
            if (!IsValid(Row.Source) || Row.Source->GetPathName() != Path ||
                Row.Source->ItemDefinitionId != Id(I) || Row.Source->GetOutermost()->IsDirty() ||
                !Row.Source->IsValidDefinition() ||
                !Row.Source->BuildQuickItemCombatActionDefinition(Row.Action) ||
                Row.Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
            {
                Error = TEXT("Missing, dirty or invalid exact source/action: ") + Path;
                return false;
            }
            Row.bRegistered = Registry->HasCanonicalItem(Id(I));
            if (Row.bRegistered != (I == LegacyCount))
            {
                Error = TEXT("Unexpected pre-existing alchemy registration: ") + Id(I).ToString();
                return false;
            }
            Row.bExistingCatalogueCollision = Root.ActionCatalogue->Find(Row.Action.ActionId) ||
                Root.ActionCatalogue->FindDetailed(Row.Action.ActionId);
            if (const auto* Existing = Root.ActionCatalogue->FindDetailed(Row.Action.ActionId))
                Row.ExistingActionDifferences = DifferentActionFields(Row.Action, *Existing);
            if (!Row.bRegistered)
                Row.bIsolatedParity = Probe(Root, *Row.Source, Row.Action, Row.ProjectionReason);
            else
            {
                FGridCombatActionDefinition Actual;
                FGridResolvedItem Item;
                if (Registry->ResolveItem(Id(I), 0, Item) && Item.AvailableActions.Num() == 1 &&
                    Registry->ResolveDetailedNonArmAction(Id(I), 0, Item.AvailableActions[0].ActionId, Actual))
                    Row.ExistingActionDifferences = DifferentActionFields(Row.Action, Actual);
                else
                    Row.ProjectionReason = TEXT("Existing Oil action needs review");
            }
            Rows.Add(MoveTemp(Row));
        }
        for (int32 I = 0; I < Rows.Num(); ++I)
            for (int32 J = 0; J < Rows.Num(); ++J)
                if (I != J && Rows[I].Action.ActionId == Rows[J].Action.ActionId)
                {
                    Rows[I].SameIdPeers.Add(J);
                    for (const FString& Field : DifferentActionFields(Rows[I].Action, Rows[J].Action))
                        Rows[I].PeerDifferences.AddUnique(Field);
                }
        for (const UObject* Object : Protected)
            if (Object->GetOutermost()->IsDirty())
            {
                Error = TEXT("Audit dirtied a canonical package");
                return false;
            }
        for (const FRow& Row : Rows)
            if (Row.Source->GetOutermost()->IsDirty())
            {
                Error = TEXT("Audit dirtied a source package");
                return false;
            }
        return true;
    }
}
