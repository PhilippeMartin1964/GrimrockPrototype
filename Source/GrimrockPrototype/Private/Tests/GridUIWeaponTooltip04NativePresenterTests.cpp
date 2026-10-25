#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/GridItemTooltipWidget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/UILocalizationText.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridNativeItemTooltipLocalization04,
    "Grimrock.UI.WeaponTooltip04.NativePresenterAndLocalization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridNativeItemTooltipLocalization04::RunTest(const FString&)
{
    // These MUST be in the actual generated ST_UI.uasset, not only UI.csv.
    // Run Scripts/Localization/RunLocalizationSmoke.ps1 before this suite.
    const TCHAR* Keys[] = {
        TEXT("Inventory.Tooltip.Weapon.Title"),
        TEXT("Inventory.Tooltip.Weapon.ComparisonTitle"),
        TEXT("Inventory.Tooltip.ItemLevelFormat"),
        TEXT("Inventory.Tooltip.Weapon.Stat.Dice"),
        TEXT("Inventory.Tooltip.Weapon.Stat.AP"),
        TEXT("Inventory.Tooltip.Weapon.Stat.ArmorPhysical"),
        TEXT("Inventory.Tooltip.Weapon.Damage.Slashing"),
        TEXT("Inventory.Tooltip.Weapon.Damage.Fire"),
        TEXT("Inventory.Tooltip.Fallback.Quantity"),
        TEXT("Inventory.Tooltip.Fallback.WeaponFact"),
        TEXT("Inventory.Tooltip.Fallback.ComparisonDelta")
    };
    for (const TCHAR* Key : Keys)
    {
        const FText Text = FUILocalizationText::Get(Key);
        TestFalse(*FString::Printf(TEXT("Materialized ST_UI key %s"), Key), Text.IsEmpty());
    }
    const FText N9 = UGridItemTooltipWidget::FormatItemLevel(9);
    TestTrue(TEXT("ItemLevel format contains N9"), N9.ToString().Contains(TEXT("9")));
    TestTrue(TEXT("Legacy ItemLevel 0 hidden"),
        UGridItemTooltipWidget::FormatItemLevel(0).IsEmpty());
    TestTrue(TEXT("Invalid ItemLevel 21 hidden"),
        UGridItemTooltipWidget::FormatItemLevel(21).IsEmpty());

    // The presentation contract is native and allows a missing/incomplete WBP
    // without inventing widgets, assigning fonts/colors or persisting data.
    UGridItemTooltipWidget* BarePresenter = NewObject<UGridItemTooltipWidget>();
    TestNotNull(TEXT("Native presenter can be allocated"), BarePresenter);
    FGridItemTooltipView InvalidView;
    TestFalse(TEXT("Invalid tooltip refuses rendering"),
        BarePresenter->PresentTooltipView(InvalidView));

    FGridWeaponTooltipComparisonFact AP;
    AP.StatId = TEXT("WeaponAP");
    AP.bHasDelta = true;
    AP.Delta = -1.f;
    AP.DeltaState = EGridItemTooltipDeltaState::Positive;
    TestTrue(TEXT("Lower PA retains beneficial semantic status"),
        UGridItemTooltipWeaponLineWidget::VisibleDeltaState(AP) ==
            EGridItemTooltipDeltaState::Positive);
    AP.bHasDelta = false;
    TestTrue(TEXT("Non-numeric dice/type facts do not invent a green/red status"),
        UGridItemTooltipWeaponLineWidget::VisibleDeltaState(AP) ==
            EGridItemTooltipDeltaState::Neutral);

    UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.DA_Weapon_AssassinDagger_01"));
    if (!TestNotNull(TEXT("ARM002 production DataAsset"), Dagger))
        return false;
    FGridItemInstance RealItem;
    RealItem.RuntimeObjectId = FGuid::NewGuid();
    RealItem.ItemDefinitionId = Dagger->ItemDefinitionId;
    RealItem.ItemLevel = 9;
    RealItem.Quantity = 1;
    RealItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    RealItem.OwnerCharacterIndex = 0;
    FGridWeaponTooltipPresentation Profile;
    if (!TestTrue(TEXT("Existing N9 projection survives UI key migration"),
        FGridWeaponTooltipProjection::Build(Dagger, RealItem, Profile)))
        return false;
    TestTrue(TEXT("Localized dice label resolved from ST_UI"),
        !Profile.Facts.IsEmpty() && !Profile.Facts[0].Label.IsEmpty());
    TestTrue(TEXT("Profile family not leaked in untranslated workbook language"),
        Profile.FamilyText.IsEmpty());

    // Reparent and designer row creation deliberately belong to INDUS04.2B.
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
