#pragma once
#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/PanelSlot.h"
#include "Components/Widget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/Blueprint.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridCombatHudWidget.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace GridEditorArmPaletteDetachPlan
{
    struct FPlan
    {
        UBlueprint* Blueprint = nullptr;
        UGridCombatHudActionWidget* Defaults = nullptr;
        bool bAssigned = false;
        bool bDiskReference = false;
        int32 IconCount = 0;
        FString Manifest;
    };

    inline void AddEditableProperties(const UObject* Object, TArray<FString>& Lines)
    {
        if (!Object) return;
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            const FProperty* Property = *It;
            if (!Property->HasAnyPropertyFlags(CPF_Edit) ||
                Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated) ||
                Property->GetFName() == FName(TEXT("EquippedWeaponActionIconPalette"))) continue;
            for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
            {
                FString Value;
                Property->ExportText_InContainer(Index, Value, Object, nullptr,
                    const_cast<UObject*>(Object), PPF_None);
                Lines.Add(FString::Printf(TEXT("P|%s|%s|%d|%s"), *Object->GetPathName(),
                    *Property->GetName(), Index, *Value));
            }
        }
    }

    // Only CoreUObject reflection and exported native class metadata are
    // needed. Avoid linking MinimalAPI UWidgetTree methods or editor K2 helpers.
    inline UObject* ObjectField(const UObject* Object, FName Name)
    {
        const FObjectPropertyBase* Field = Object
            ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
        return Field ? Field->GetObjectPropertyValue_InContainer(Object) : nullptr;
    }

    inline bool ExportField(const UObject* Object, FName Name, FString& Out)
    {
        const FProperty* Field = Object ? FindFProperty<FProperty>(Object->GetClass(), Name) : nullptr;
        if (!Field) return false;
        Out.Reset();
        Field->ExportText_InContainer(0, Out, Object, nullptr, const_cast<UObject*>(Object), PPF_None);
        return true;
    }

    inline bool Fingerprint(UBlueprint* BP, const UGridCombatHudActionWidget* Defaults,
        FString& Out, FString& Error)
    {
        UObject* Tree = ObjectField(BP, FName(TEXT("WidgetTree")));
        UObject* RootWidget = ObjectField(Tree, FName(TEXT("RootWidget")));
        if (!BP || !Defaults || !Tree || !Cast<UWidget>(RootWidget))
        {
            Error = TEXT("Missing Designer tree or root");
            return false;
        }
        TArray<FString> Lines;
        AddEditableProperties(Defaults, Lines);
        Lines.Add(TEXT("ROOT|") + RootWidget->GetPathName());
        TArray<UObject*> Descendants;
        GetObjectsWithOuter(Tree, Descendants, true, RF_Transient);
        bool bActionIconFound = false;
        int32 WidgetCount = 0;
        for (UObject* Child : Descendants)
        {
            UWidget* Widget = Cast<UWidget>(Child);
            if (!Widget) continue;
            ++WidgetCount;
            bActionIconFound |= Widget->GetFName() == FName(TEXT("Image_ActionIcon"));
            Lines.Add(FString::Printf(TEXT("W|%s|%s"), *Widget->GetPathName(),
                *Widget->GetClass()->GetPathName()));
            AddEditableProperties(Widget, Lines);
            // Reflected Slot, Parent, Content and ordered Slots encode the
            // actual hierarchy/order even when UMG methods are not exported.
            UObject* Slot = ObjectField(Widget, FName(TEXT("Slot")));
            Lines.Add(TEXT("S|") + Widget->GetPathName() + TEXT("|") + GetPathNameSafe(Slot));
            if (Slot)
            {
                AddEditableProperties(Slot, Lines);
                Lines.Add(TEXT("PARENT|") + Widget->GetPathName() + TEXT("|") +
                    GetPathNameSafe(ObjectField(Slot, FName(TEXT("Parent")))));
                Lines.Add(TEXT("CONTENT|") + Widget->GetPathName() + TEXT("|") +
                    GetPathNameSafe(ObjectField(Slot, FName(TEXT("Content")))));
            }
            FString OrderedSlots;
            if (ExportField(Widget, FName(TEXT("Slots")), OrderedSlots))
                Lines.Add(TEXT("ORDER|") + Widget->GetPathName() + TEXT("|") + OrderedSlots);
        }
        if (!bActionIconFound || WidgetCount <= 0 || !Descendants.Contains(RootWidget))
        {
            Error = TEXT("Missing authored Image_ActionIcon or root in Designer descendants");
            return false;
        }
        TArray<UObject*> BlueprintObjects;
        GetObjectsWithOuter(BP, BlueprintObjects, true, RF_Transient);
        for (UObject* Child : BlueprintObjects)
        {
            UEdGraph* Graph = Cast<UEdGraph>(Child);
            if (!Graph) continue;
            Lines.Add(TEXT("G|") + Graph->GetPathName());
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (!Node) return false;
                FString VariableReference;
                if (ExportField(Node, FName(TEXT("VariableReference")), VariableReference) &&
                    VariableReference.Contains(TEXT("EquippedWeaponActionIconPalette"), ESearchCase::CaseSensitive))
                {
                    Error = TEXT("Graph still reads historical palette");
                    return false;
                }
                Lines.Add(FString::Printf(TEXT("N|%s|%s|%s|%s"), *Graph->GetPathName(),
                    *Node->GetClass()->GetPathName(), *Node->NodeGuid.ToString(), *VariableReference));
                AddEditableProperties(Node, Lines);
            }
        }
        Lines.Sort();
        Out = FString::Join(Lines, TEXT("\n"));
        return !Out.IsEmpty();
    }

    inline bool Build(FPlan& Out, FString& Error)
    {
        Out = FPlan();
        Error.Reset();
        auto* Root = LoadObject<UGridItemSystemAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
        auto* Legacy = LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr, TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
        Out.Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction.WBP_GridCombatHudAction"));
        if (!Root || !Root->WeaponVisualCatalogue || !Legacy || !Out.Blueprint ||
            Out.Blueprint->GetClass()->GetPathName() != TEXT("/Script/UMGEditor.WidgetBlueprint") ||
            !Out.Blueprint->GeneratedClass ||
            !Out.Blueprint->GeneratedClass->IsChildOf(UGridCombatHudActionWidget::StaticClass()) ||
            Root->GetOutermost()->IsDirty() || Root->WeaponVisualCatalogue->GetOutermost()->IsDirty() ||
            Legacy->GetOutermost()->IsDirty() || Out.Blueprint->GetOutermost()->IsDirty() ||
            !GridArmClean06ActionIconsProjection::IsIdentical(*Legacy, *Root->WeaponVisualCatalogue))
        {
            Error = TEXT("Root, palette parity or WBP precondition failed");
            return false;
        }
        Out.IconCount = Legacy->Icons.Num();
        const auto* Dual = Root->WeaponVisualCatalogue->ActionIcons.Find(EGridEquippedWeaponActionIconKind::MeleeDualWield);
        if (Out.IconCount != 12 || !Dual || Dual->IsNull())
        {
            Error = TEXT("Expected twelve categories and actual dual texture");
            return false;
        }
        Out.Defaults = Cast<UGridCombatHudActionWidget>(Out.Blueprint->GeneratedClass->GetDefaultObject());
        if (!Out.Defaults || (Out.Defaults->EquippedWeaponActionIconPalette &&
            Out.Defaults->EquippedWeaponActionIconPalette.Get() != Legacy))
        {
            Error = TEXT("Unexpected WBP default palette");
            return false;
        }
        Out.bAssigned = Out.Defaults->EquippedWeaponActionIconPalette != nullptr;
        using namespace UE::AssetRegistry;
        IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        Registry.SearchAllAssets(true);
        const FName WBP = Out.Blueprint->GetOutermost()->GetFName();
        const FName OldPackage = Legacy->GetOutermost()->GetFName();
        TArray<FName> Hard, Soft, Referencers;
        Registry.GetDependencies(WBP, Hard, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Hard));
        Registry.GetDependencies(WBP, Soft, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Soft));
        Registry.GetReferencers(OldPackage, Referencers, EDependencyCategory::Package, FDependencyQuery());
        Out.bDiskReference = Hard.Contains(OldPackage) || Soft.Contains(OldPackage);
        for (FName Ref : Referencers)
        {
            if (Ref.ToString().StartsWith(TEXT("/Game/")) && Ref != WBP)
            {
                Error = TEXT("Unexpected palette referencer");
                return false;
            }
        }
        if (Out.bAssigned != Out.bDiskReference || (Out.bAssigned && !Hard.Contains(OldPackage)))
        {
            Error = TEXT("CDO and on-disk registry disagree");
            return false;
        }
        return Fingerprint(Out.Blueprint, Out.Defaults, Out.Manifest, Error);
    }
}
