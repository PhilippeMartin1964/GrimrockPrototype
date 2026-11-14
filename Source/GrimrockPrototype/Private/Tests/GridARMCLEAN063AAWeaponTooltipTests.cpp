#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AAWeaponTooltip,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWeaponTooltipProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AAWeaponTooltip::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register real item catalogues"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }
    // Deliberately omit the former per-weapon catalogue: canonical tooltip
    // projection must work even when old DA_Weapon N1-N20 profiles are absent.
    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    const FName Ids[] = { TEXT("Weapon_StraightSword_01"),
        TEXT("Weapon_ThrowingKnife_01"), TEXT("Weapon_RoundShield_01"),
        TEXT("Weapon_HolyAegis_01") };
    const int32 Levels[] = { 1, 20 };
    for (const FName Id : Ids)
    {
        Old->ItemDefinitionId = Id;
        for (const int32 Level : Levels)
        {
            FGridResolvedItem Canonical;
            if (!TestTrue(TEXT("Production ARM N1/N20 exists"),
                Registry->ResolveItem(Id, Level, Canonical))) return false;
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            FGridWeaponTooltipPresentation Tooltip;
            if (!TestTrue(TEXT("Canonical tooltip must not depend on old catalogue"),
                FGridWeaponTooltipProjection::Build(Old.Get(), Item, Tooltip, Registry.Get())))
                return false;
            TestTrue(TEXT("Tooltip reports a profile"), Tooltip.bHasProfile);
            TestEqual(TEXT("Tooltip fixed instance level"), Tooltip.ItemLevel, Level);
            TestEqual(TEXT("Tooltip canonical ARM archetype"), Tooltip.ArchetypeId, Canonical.ArchetypeId);
            TestEqual(TEXT("Tooltip passive-vs-attack role"),
                Tooltip.bPassive, !Canonical.Weapon.DamageDice.bEnabled);
            if (!Canonical.Weapon.DamageDice.bEnabled)
            {
                const FGridWeaponTooltipFact* Armor = Tooltip.Facts.FindByPredicate(
                    [](const FGridWeaponTooltipFact& Fact)
                    { return Fact.StatId == FName(TEXT("WeaponPhysicalArmor")); });
                if (Canonical.Weapon.PhysicalArmor > 0)
                {
                    TestNotNull(TEXT("Passive shield nominal armor shown"), Armor);
                    if (Armor)
                    {
                        TestEqual(TEXT("Nominal physical armor from ItemSystem"),
                            Armor->ValueText.ToString(), FString::FromInt(Canonical.Weapon.PhysicalArmor));
                    }
                }
            }
            else
            {
                const FGridWeaponTooltipFact* Min = Tooltip.Facts.FindByPredicate(
                    [](const FGridWeaponTooltipFact& Fact)
                    { return Fact.StatId == FName(TEXT("WeaponMinDamage")); });
                TestNotNull(TEXT("Attack raw minimum shown"), Min);
                if (Min)
                {
                    TestEqual(TEXT("Attack minimum comes from canonical dice"),
                        Min->ValueText.ToString(),
                        FString::FromInt(Canonical.Weapon.DamageDice.GetMinimumDamage()));
                }
            }

            FGridItemInstance Equipped = Item;
            Equipped.RuntimeObjectId = FGuid::NewGuid();
            Equipped.ItemLevel = Level == 1 ? 20 : 1;
            TArray<FGridWeaponTooltipComparisonFact> Comparisons;
            TestTrue(TEXT("Compare levels without legacy data"),
                FGridWeaponTooltipProjection::BuildComparison(
                    Old.Get(), Item, Old.Get(), Equipped, Comparisons, Registry.Get()));
            TestTrue(TEXT("Comparable roles yield facts"), !Comparisons.IsEmpty());

            Item.ItemLevel = 0;
            FGridWeaponTooltipPresentation Invalid;
            TestFalse(TEXT("Registered ARM N0 refuses old N1 tooltip"),
                FGridWeaponTooltipProjection::Build(Old.Get(), Item, Invalid, Registry.Get()));
            TestFalse(TEXT("Registered ARM N0 has no profile"), Invalid.bHasProfile);
        }
    }
    Old->ItemDefinitionId = TEXT("UnknownToolTip_063AA");
    FGridItemInstance Unknown;
    Unknown.RuntimeObjectId = FGuid::NewGuid();
    Unknown.ItemDefinitionId = Old->ItemDefinitionId;
    Unknown.ItemLevel = 1;
    Unknown.Quantity = 1;
    FGridWeaponTooltipPresentation Unregistered;
    TestFalse(TEXT("Unknown ID retains legacy path, cannot invent profile"),
        FGridWeaponTooltipProjection::Build(Old.Get(), Unknown, Unregistered, Registry.Get()));
    return true;
}

#endif
