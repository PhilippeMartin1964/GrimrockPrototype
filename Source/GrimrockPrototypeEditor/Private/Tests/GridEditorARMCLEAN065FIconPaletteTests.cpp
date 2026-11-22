#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "UI/GridCombatHudWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARM065FPalette,
    "Grimrock.RPG.ARM.CLEAN06_5G.DetachedIconPaletteSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARM065FPalette::RunTest(const FString&)
{
    // This is a permanent post-conversion gate: no loading of the old palette.
    const FName OldProperty(TEXT("EquippedWeaponActionIconPalette"));
    TestTrue(TEXT("Native widget no longer serializes legacy palette"),
        FindFProperty<FProperty>(UGridCombatHudActionWidget::StaticClass(), OldProperty) == nullptr);
    UClass* Widget = LoadObject<UClass>(nullptr,
        TEXT("/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction.WBP_GridCombatHudAction_C"));
    if (!TestNotNull(TEXT("Production action WBP loads"), Widget)) return false;
    TestTrue(TEXT("Production action widget class unchanged"),
        Widget->IsChildOf(UGridCombatHudActionWidget::StaticClass()));
    TestTrue(TEXT("Generated class has no legacy property"),
        FindFProperty<FProperty>(Widget, OldProperty) == nullptr);
    const auto* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!Root || !Root->WeaponVisualCatalogue)
    {
        AddError(TEXT("Canonical icon authority unavailable"));
        return false;
    }
    TestEqual(TEXT("All twelve historical categories remain canonical"),
        Root->WeaponVisualCatalogue->ActionIcons.Num(), 12);
    const auto* Dual = Root->WeaponVisualCatalogue->ActionIcons.Find(EGridEquippedWeaponActionIconKind::MeleeDualWield);
    TestTrue(TEXT("Canonical dual icon is assigned"), Dual && !Dual->IsNull());

    using namespace UE::AssetRegistry;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    const FName RetiredPalettePackage(
        TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons"));
    TArray<FAssetData> RetiredAssets;
    Registry.GetAssetsByPackageName(RetiredPalettePackage, RetiredAssets, true);
    TestEqual(TEXT("The historical icon palette remains retired in AssetRegistry"),
        RetiredAssets.Num(), 0);
    const FString RetiredPaletteFile = FPaths::ProjectContentDir() /
        TEXT("GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset");
    TestFalse(TEXT("The retired icon palette must be absent on disk"),
        IFileManager::Get().FileExists(*RetiredPaletteFile));
    TArray<FName> Refs;
    Registry.GetReferencers(RetiredPalettePackage,
        Refs, EDependencyCategory::Package, FDependencyQuery());
    TestEqual(TEXT("No package referencer to retired palette"), Refs.Num(), 0);
    return !HasAnyErrors();
}
#endif
