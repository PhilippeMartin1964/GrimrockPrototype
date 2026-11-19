#pragma once
#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Variable.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridCombatHudWidget.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace GridEditorArmPaletteDetachPlan
{
    struct FPlan
    {
        UWidgetBlueprint* Blueprint = nullptr;
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

    inline bool Fingerprint(UWidgetBlueprint* BP, const UGridCombatHudActionWidget* Defaults,
        FString& Out, FString& Error)
    {
        if (!BP || !Defaults || !BP->WidgetTree || !BP->WidgetTree->FindWidget(FName(TEXT("Image_ActionIcon"))))
        {
            Error = TEXT("Missing Designer action icon");
            return false;
        }
        TArray<FString> Lines;
        AddEditableProperties(Defaults, Lines);
        TArray<UWidget*> Widgets;
        BP->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (!Widget) return false;
            UPanelWidget* Parent = Widget->GetParent();
            Lines.Add(FString::Printf(TEXT("W|%s|%s|%s|%d"), *Widget->GetPathName(),
                *Widget->GetClass()->GetPathName(), *GetNameSafe(Parent),
                Parent ? Parent->GetChildIndex(Widget) : INDEX_NONE));
            AddEditableProperties(Widget, Lines);
            AddEditableProperties(Widget->Slot, Lines);
        }
        TArray<UEdGraph*> Graphs;
        BP->GetAllGraphs(Graphs);
        for (UEdGraph* Graph : Graphs)
        {
            if (!Graph) return false;
            Lines.Add(TEXT("G|") + Graph->GetPathName());
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (!Node) return false;
                if (const UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node))
                {
                    if (Variable->GetVarName() == FName(TEXT("EquippedWeaponActionIconPalette")))
                    {
                        Error = TEXT("Graph still reads historical palette");
                        return false;
                    }
                }
                Lines.Add(FString::Printf(TEXT("N|%s|%s|%s"), *Graph->GetPathName(),
                    *Node->GetClass()->GetPathName(), *Node->NodeGuid.ToString()));
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
        Out.Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction.WBP_GridCombatHudAction"));
        if (!Root || !Root->WeaponVisualCatalogue || !Legacy || !Out.Blueprint ||
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
