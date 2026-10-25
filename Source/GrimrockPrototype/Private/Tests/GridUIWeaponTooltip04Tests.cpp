#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridInventorySlotWidget.h"

namespace WeaponTooltipTests
{
    const TCHAR* CataloguePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

    FGridItemInstance MakeItem(const UGridItemDefinitionAsset* Def, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Def->ItemDefinitionId;
        Item.DisplayName = Def->DisplayName;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerCharacterIndex = 0;
        return Item;
    }

    const FGridWeaponTooltipFact* Find(const FGridWeaponTooltipPresentation& View, FName Id)
    {
        return View.Facts.FindByPredicate([Id](const FGridWeaponTooltipFact& Fact)
        {
            return Fact.StatId == Id;
        });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipFixedLevelsTest,
    "Grimrock.UI.WeaponTooltip04.FixedLevelsAndDice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipFixedLevelsTest::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponTooltipTests::CataloguePath);
    UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.DA_Weapon_AssassinDagger_01"));
    if (!TestNotNull(TEXT("Canonical catalogue"), Catalogue) ||
        !TestNotNull(TEXT("ARM002 authored item"), Dagger))
        return false;
    const bool bCatDirty = Catalogue->GetOutermost()->IsDirty();
    const bool bItemDirty = Dagger->GetOutermost()->IsDirty();

    FGridWeaponTooltipPresentation N1;
    FGridWeaponTooltipPresentation N9;
    const bool bN1 = FGridWeaponTooltipProjection::Build(
        Dagger, WeaponTooltipTests::MakeItem(Dagger, 1), N1);
    const bool bN9 = FGridWeaponTooltipProjection::Build(
        Dagger, WeaponTooltipTests::MakeItem(Dagger, 9), N9);
    if (!TestTrue(TEXT("N1 profile resolved"), bN1) ||
        !TestTrue(TEXT("N9 profile resolved"), bN9))
        return false;
    TestEqual(TEXT("N1 real level"), N1.ItemLevel, 1);
    TestEqual(TEXT("N9 real level"), N9.ItemLevel, 9);
    TestFalse(TEXT("ARM002 is an attacking weapon"), N1.bPassive);
    const FGridWeaponLevelProfile* P1 = Catalogue->FindProfile(TEXT("ARM002"), 1);
    const FGridWeaponLevelProfile* P9 = Catalogue->FindProfile(TEXT("ARM002"), 9);
    if (!TestNotNull(TEXT("Actual N1"), P1) ||
        !TestNotNull(TEXT("Actual N9"), P9))
        return false;
    const FGridWeaponTooltipFact* Dice1 = WeaponTooltipTests::Find(N1, TEXT("WeaponDice"));
    const FGridWeaponTooltipFact* Dice9 = WeaponTooltipTests::Find(N9, TEXT("WeaponDice"));
    const FGridWeaponTooltipFact* Minimum9 = WeaponTooltipTests::Find(N9, TEXT("WeaponMinDamage"));
    const FGridWeaponTooltipFact* Maximum9 = WeaponTooltipTests::Find(N9, TEXT("WeaponMaxDamage"));
    if (!TestNotNull(TEXT("N1 dice fact"), Dice1) ||
        !TestNotNull(TEXT("N9 dice fact"), Dice9) ||
        !TestNotNull(TEXT("N9 minimum"), Minimum9) ||
        !TestNotNull(TEXT("N9 maximum"), Maximum9))
        return false;
    const FString N1Suffix = P1->DamageDice.FlatBonus == 0
        ? FString() : FString::Printf(TEXT(" + %d"), P1->DamageDice.FlatBonus);
    TestEqual(TEXT("N1 formula exactly"), Dice1->ValueText.ToString(),
        FString::Printf(TEXT("%dd%d%s"), P1->DamageDice.DiceCount,
            P1->DamageDice.DiceSides, *N1Suffix));
    TestEqual(TEXT("N9 minimum from actual dice"), Minimum9->ValueText.ToString(),
        FString::FromInt(P9->DamageDice.GetMinimumDamage()));
    TestEqual(TEXT("N9 maximum from actual dice"), Maximum9->ValueText.ToString(),
        FString::FromInt(P9->DamageDice.GetMaximumDamage()));
    const FGridWeaponTooltipFact* ActionPoints = WeaponTooltipTests::Find(N9, TEXT("WeaponAP"));
    if (TestNotNull(TEXT("N9 PA fact"), ActionPoints))
    {
        TestEqual(TEXT("N9 PA consistent with workbook"), ActionPoints->ValueText.ToString(),
            FString::FromInt(P9->ActionPointCost));
    }
    TestTrue(TEXT("N1 and N9 formulas differ when catalogue changes"),
        P1->DamageDice.FlatBonus == P9->DamageDice.FlatBonus ||
            !Dice1->ValueText.EqualTo(Dice9->ValueText));
    TestEqual(TEXT("Catalogue package untouched"), Catalogue->GetOutermost()->IsDirty(), bCatDirty);
    TestEqual(TEXT("Item package untouched"), Dagger->GetOutermost()->IsDirty(), bItemDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipPassiveLegacyTest,
    "Grimrock.UI.WeaponTooltip04.PassiveShieldAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipPassiveLegacyTest::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponTooltipTests::CataloguePath);
    UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
    if (!TestNotNull(TEXT("Catalogue"), Catalogue) ||
        !TestNotNull(TEXT("ARM075 shield"), Shield))
        return false;

    FGridWeaponTooltipPresentation Passive;
    if (!TestTrue(TEXT("Passive level 12 resolved"),
        FGridWeaponTooltipProjection::Build(
            Shield, WeaponTooltipTests::MakeItem(Shield, 12), Passive)))
        return false;
    TestTrue(TEXT("Passive shield flagged as passive"), Passive.bPassive);
    TestTrue(TEXT("No fictional attack dice"), Passive.DiceText.IsEmpty());
    TestTrue(TEXT("No fictional weapon PA"), WeaponTooltipTests::Find(Passive, TEXT("WeaponAP")) == nullptr);
    TestTrue(TEXT("No fictional damage line"), WeaponTooltipTests::Find(Passive, TEXT("WeaponDice")) == nullptr);
    const FGridWeaponLevelProfile* P = Catalogue->FindProfile(TEXT("ARM075"), 12);
    const FGridWeaponTooltipFact* Armor =
        WeaponTooltipTests::Find(Passive, TEXT("WeaponPhysicalArmor"));
    if (!TestNotNull(TEXT("Actual passive profile"), P) ||
        !TestNotNull(TEXT("Shield physical armor shown"), Armor))
        return false;
    TestEqual(TEXT("Physical armor uses actual level 12"), Armor->ValueText.ToString(),
        FString::FromInt(P->PhysicalArmor));

    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>();
    Legacy->ItemDefinitionId = TEXT("Legacy_ItemWithoutCatalogue");
    Legacy->DisplayName = FText::FromString(TEXT("Legacy item"));
    FGridWeaponTooltipPresentation LegacyView;
    TestFalse(TEXT("Legacy is not retroactively migrated"),
        FGridWeaponTooltipProjection::Build(
            Legacy, WeaponTooltipTests::MakeItem(Legacy, 0), LegacyView));
    TestFalse(TEXT("Legacy produces no new weapon profile"), LegacyView.bHasProfile);

    FGridWeaponTooltipPresentation InvalidLevel;
    TestFalse(TEXT("No guessing a level outside N1-N20"),
        FGridWeaponTooltipProjection::Build(
            Shield, WeaponTooltipTests::MakeItem(Shield, 21), InvalidLevel));
    TestFalse(TEXT("Invalid level remains empty"), InvalidLevel.bHasProfile);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipSlotIntegrationTest,
    "Grimrock.UI.WeaponTooltip04.InventorySlotProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipSlotIntegrationTest::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.DA_Weapon_AssassinDagger_01"));
    if (!TestNotNull(TEXT("Canonical ARM002"), Dagger))
        return false;
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    FGridCharacterInventoryState Hero;
    Hero.CharacterId = FGuid::NewGuid();
    Hero.DisplayName = FText::FromString(TEXT("Weapon profile preview"));
    Hero.InventorySlots.SetNum(2);
    Inventory->PartyInventoryState.ActiveCharacters.Add(Hero);
    Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
    Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
    Inventory->PartyInventoryState.MaxActiveCharacters = 6;
    if (!TestTrue(TEXT("Production definition registered to test inventory"),
        Inventory->RegisterItemDefinition(Dagger)))
        return false;

    UGridInventoryWidget* Parent = NewObject<UGridInventoryWidget>();
    Parent->InventoryComponent = Inventory;
    UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
    Slot->SetOwnerInventoryWidget(Parent);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    FGridItemInstance Item = WeaponTooltipTests::MakeItem(Dagger, 9);
    Slot->SetItem(Item);

    // Attach a genuinely equipped N1 to exercise the existing inventory comparison route.
    FGridItemInstance Equipped = WeaponTooltipTests::MakeItem(Dagger, 1);
    Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
    Equipped.EquipmentSlot = EGridEquipmentSlot::MainHand;
    Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Equipped;

    const FGridItemTooltipView View = Slot->GetTooltipView();
    TestTrue(TEXT("Existing tooltip remains valid"), View.bValid);
    TestTrue(TEXT("Actual N9 reaches WBP-facing read model"), View.WeaponProfile.bHasProfile);
    TestEqual(TEXT("Real level reaches tooltip"), View.WeaponProfile.ItemLevel, 9);
    const FGridWeaponTooltipFact* Dice = WeaponTooltipTests::Find(
        View.WeaponProfile, TEXT("WeaponDice"));
    if (!TestNotNull(TEXT("Dice forwarded to slot view"), Dice))
        return false;
    TestTrue(TEXT("Fallback tooltip contains real dice"),
        Slot->GetTooltipText().ToString().Contains(Dice->ValueText.ToString()));
    const FGridItemTooltipEquipmentComparison* WeaponComp =
        View.EquipmentComparisons.FindByPredicate(
            [](const FGridItemTooltipEquipmentComparison& Comparison)
            {
                return Comparison.EquipmentSlot == EGridEquipmentSlot::MainHand;
            });
    if (TestNotNull(TEXT("Equipment comparison exists for MainHand"), WeaponComp))
    {
        const FGridWeaponTooltipComparisonFact* WeaponDice =
            WeaponComp->WeaponFacts.FindByPredicate(
                [](const FGridWeaponTooltipComparisonFact& Fact)
                {
                    return Fact.StatId == FName(TEXT("WeaponDice"));
                });
        if (TestNotNull(TEXT("New weapon comparison reaches WBP read model"), WeaponDice))
        {
            TestFalse(TEXT("Dice remain purely textual"), WeaponDice->bHasDelta);
            TestTrue(TEXT("Fallback comparison shows equipped dice"),
                Slot->GetTooltipText().ToString().Contains(
                    WeaponDice->EquippedText.ToString()));
        }
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipRealComparisonTest,
    "Grimrock.UI.WeaponTooltip04.RealWeaponComparison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipRealComparisonTest::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponTooltipTests::CataloguePath);
    UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.DA_Weapon_AssassinDagger_01"));
    UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
    if (!TestNotNull(TEXT("Production catalogue"), Catalogue) ||
        !TestNotNull(TEXT("ARM002"), Dagger) ||
        !TestNotNull(TEXT("ARM075"), Shield))
        return false;
    const bool bBeforeCatalogue = Catalogue->GetOutermost()->IsDirty();
    TArray<FGridWeaponTooltipComparisonFact> Facts;
    const FGridItemInstance N9 = WeaponTooltipTests::MakeItem(Dagger, 9);
    const FGridItemInstance N1 = WeaponTooltipTests::MakeItem(Dagger, 1);
    if (!TestTrue(TEXT("Two real weapon levels comparable"),
        FGridWeaponTooltipProjection::BuildComparison(Dagger, N9, Dagger, N1, Facts)))
        return false;
    const FGridWeaponLevelProfile* P9 = Catalogue->FindProfile(TEXT("ARM002"), 9);
    const FGridWeaponLevelProfile* P1 = Catalogue->FindProfile(TEXT("ARM002"), 1);
    if (!TestNotNull(TEXT("Source profile N9"), P9) ||
        !TestNotNull(TEXT("Source profile N1"), P1))
        return false;
    const auto* Min = Facts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& F)
    {
        return F.StatId == TEXT("WeaponMinDamage");
    });
    const auto* Dice = Facts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& F)
    {
        return F.StatId == TEXT("WeaponDice");
    });
    if (!TestNotNull(TEXT("Numeric min damage comparison"), Min) ||
        !TestNotNull(TEXT("Text-only NdM+K comparison"), Dice))
        return false;
    TestTrue(TEXT("Same damage semantics permit numeric delta"), Min->bHasDelta);
    TestEqual(TEXT("Minimum damage from the exact two levels"), Min->Delta,
        static_cast<float>(P9->DamageDice.GetMinimumDamage() -
            P1->DamageDice.GetMinimumDamage()));
    TestFalse(TEXT("Dice formulas are not arithmetic deltas"), Dice->bHasDelta);

    TArray<FGridWeaponTooltipComparisonFact> Incompatible;
    TestFalse(TEXT("Cannot claim weapon and shield have comparable attacks"),
        FGridWeaponTooltipProjection::BuildComparison(
            Dagger, N9, Shield, WeaponTooltipTests::MakeItem(Shield, 12), Incompatible));
    TestEqual(TEXT("Incompatible roles leave no partial comparison"), Incompatible.Num(), 0);

    TArray<FGridWeaponTooltipComparisonFact> ShieldFacts;
    if (!TestTrue(TEXT("Two passive shield levels comparable"),
        FGridWeaponTooltipProjection::BuildComparison(
            Shield, WeaponTooltipTests::MakeItem(Shield, 12),
            Shield, WeaponTooltipTests::MakeItem(Shield, 1), ShieldFacts)))
        return false;
    TestTrue(TEXT("No fictitious shield dice"), ShieldFacts.FindByPredicate(
        [](const FGridWeaponTooltipComparisonFact& F) { return F.StatId == TEXT("WeaponDice"); }) == nullptr);
    const auto* Armor = ShieldFacts.FindByPredicate(
        [](const FGridWeaponTooltipComparisonFact& F)
        { return F.StatId == TEXT("WeaponPhysicalArmor"); });
    const FGridWeaponLevelProfile* S12 = Catalogue->FindProfile(TEXT("ARM075"), 12);
    const FGridWeaponLevelProfile* S1 = Catalogue->FindProfile(TEXT("ARM075"), 1);
    if (!TestNotNull(TEXT("Shield N12"), S12) ||
        !TestNotNull(TEXT("Shield N1"), S1) ||
        !TestNotNull(TEXT("Shield armor comparison"), Armor))
        return false;
    TestEqual(TEXT("Shield delta from canonical N12 and N1"), Armor->Delta,
        static_cast<float>(S12->PhysicalArmor - S1->PhysicalArmor));
    TestEqual(TEXT("Catalogue not mutated"), Catalogue->GetOutermost()->IsDirty(), bBeforeCatalogue);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipPAValenceTest,
    "Grimrock.UI.WeaponTooltip04.ActionPointDeltaSemantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipPAValenceTest::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Production =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponTooltipTests::CataloguePath);
    if (!TestNotNull(TEXT("Production catalogue"), Production))
        return false;
    const FGridWeaponLevelProfile* Real1 = Production->FindProfile(TEXT("ARM002"), 1);
    const FGridWeaponLevelProfile* Real9 = Production->FindProfile(TEXT("ARM002"), 9);
    if (!TestNotNull(TEXT("Real N1 profile"), Real1) ||
        !TestNotNull(TEXT("Real N9 profile"), Real9))
        return false;

    // Synthetic fixture on transient objects only; no production .uasset mutation.
    UGridWeaponCatalogueAsset* Sim = NewObject<UGridWeaponCatalogueAsset>();
    FGridWeaponLevelProfile EquipProfile = *Real1;
    FGridWeaponLevelProfile CandidateProfile = *Real9;
    EquipProfile.ActionPointCost = 3;
    CandidateProfile.ActionPointCost = 2;
    Sim->Profiles.Add(EquipProfile);
    Sim->Profiles.Add(CandidateProfile);

    UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
    Item->ItemDefinitionId = TEXT("Weapon_TooltipPAFixture");
    Item->bUseWeaponCatalogue = true;
    Item->WeaponArchetypeId = TEXT("ARM002");
    Item->WeaponCatalogue = Sim;
    TArray<FGridWeaponTooltipComparisonFact> Facts;
    if (!TestTrue(TEXT("PA fixture resolves both levels"),
        FGridWeaponTooltipProjection::BuildComparison(
            Item, WeaponTooltipTests::MakeItem(Item, 9),
            Item, WeaponTooltipTests::MakeItem(Item, 1), Facts)))
        return false;
    const auto* AP = Facts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& F)
    {
        return F.StatId == TEXT("WeaponAP");
    });
    if (!TestNotNull(TEXT("PA comparison fact"), AP))
        return false;
    TestEqual(TEXT("Candidate 2 PA minus equipped 3 PA"), AP->Delta, -1.f);
    TestTrue(TEXT("Lower AP is a green beneficial delta"),
        AP->DeltaState == EGridItemTooltipDeltaState::Positive);
    TestEqual(TEXT("Display signed PA delta"), AP->DeltaText.ToString(), FString(TEXT("-1")));

    // Raw damage dice are comparable despite different attribute scaling.
    // Pic de guerre (Force) vs poignard d'assassin (Dexterite):
    // their 1d4+3 versus 1d4 base rolls should generate two green +3s.
    Sim->Profiles[1].DamageDice = Real1->DamageDice;
    Sim->Profiles[1].DamageDice.FlatBonus += 3;
    Sim->Profiles[1].DamageType = Real1->DamageType;
    Sim->Profiles[1].PhysicalSubtype = Real1->PhysicalSubtype;
    Sim->Profiles[1].ScalingAttribute =
        Real1->ScalingAttribute == EGridAttackScalingAttribute::Strength
            ? EGridAttackScalingAttribute::Dexterity
            : EGridAttackScalingAttribute::Strength;
    Facts.Reset();
    if (TestTrue(TEXT("Different scaling attributes preserve comparable raw dice"),
        FGridWeaponTooltipProjection::BuildComparison(
            Item, WeaponTooltipTests::MakeItem(Item, 9),
            Item, WeaponTooltipTests::MakeItem(Item, 1), Facts)))
    {
        for (const FName StatId : { FName(TEXT("WeaponMinDamage")), FName(TEXT("WeaponMaxDamage")) })
        {
            const auto* RawFact = Facts.FindByPredicate(
                [StatId](const FGridWeaponTooltipComparisonFact& F)
                { return F.StatId == StatId; });
            if (TestNotNull(TEXT("Raw damage fact is projected"), RawFact))
            {
                TestTrue(TEXT("Different scaling permits raw dice delta"), RawFact->bHasDelta);
                TestEqual(TEXT("Raw damage bonus is +3"), RawFact->Delta, 3.f);
                TestTrue(TEXT("Raw damage improvement is positive (green)"),
                    RawFact->DeltaState == EGridItemTooltipDeltaState::Positive);
                TestEqual(TEXT("Signed positive display value"),
                    RawFact->DeltaText.ToString(), FString(TEXT("+3")));
            }
        }
    }

    // A different damage element is NOT an automatic green numeric damage gain.
    Sim->Profiles[1].DamageType = EGridDamageType::Fire;
    Sim->Profiles[1].PhysicalSubtype = EGridPhysicalDamageSubtype::None;
    Facts.Reset();
    if (TestTrue(TEXT("Different damage types still show both sets of facts"),
        FGridWeaponTooltipProjection::BuildComparison(
            Item, WeaponTooltipTests::MakeItem(Item, 9),
            Item, WeaponTooltipTests::MakeItem(Item, 1), Facts)))
    {
        const auto* Minimum = Facts.FindByPredicate(
            [](const FGridWeaponTooltipComparisonFact& F)
            { return F.StatId == TEXT("WeaponMinDamage"); });
        if (TestNotNull(TEXT("Damage bounds remain readable"), Minimum))
        {
            TestFalse(TEXT("Different damage semantics suppress misleading delta"),
                Minimum->bHasDelta);
        }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
