#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridInventorySlotWidget.h"

namespace WeaponTooltipTests
{
    FGridItemInstance MakeItem(const UGridItemDefinitionAsset* Def, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Def->ItemDefinitionId;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerCharacterIndex = 0;
        return Item;
    }

    FGridItemInstance MakeCanonicalItem(FName ItemId, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = ItemId;
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
    FGridCanonicalRegistryFixture Fixture;
    const FName DaggerId(TEXT("Weapon_AssassinDagger_01"));
    FGridResolvedItem P1, P9;
    if (!TestTrue(TEXT("Canonical production ARM002 N1 resolves"),
        Fixture.Resolve(DaggerId, 1, P1)) ||
        !TestTrue(TEXT("Canonical production ARM002 N9 resolves"),
            Fixture.Resolve(DaggerId, 9, P9)) ||
        !TestTrue(TEXT("Both levels are offensive armament"),
            P1.bIsArmament && P9.bIsArmament &&
            P1.Weapon.DamageDice.bEnabled && P9.Weapon.DamageDice.bEnabled))
        return false;
    UGridItemSystemAsset* Root = Fixture.Registry->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production ItemSystem root"), Root)) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();

    FGridWeaponTooltipPresentation N1;
    FGridWeaponTooltipPresentation N9;
    const bool bN1 = FGridWeaponTooltipProjection::Build(
        nullptr, WeaponTooltipTests::MakeCanonicalItem(DaggerId, 1),
        N1, Fixture.Registry.Get());
    const bool bN9 = FGridWeaponTooltipProjection::Build(
        nullptr, WeaponTooltipTests::MakeCanonicalItem(DaggerId, 9),
        N9, Fixture.Registry.Get());
    if (!TestTrue(TEXT("N1 profile resolved"), bN1) ||
        !TestTrue(TEXT("N9 profile resolved"), bN9))
        return false;
    TestEqual(TEXT("N1 real level"), N1.ItemLevel, 1);
    TestEqual(TEXT("N9 real level"), N9.ItemLevel, 9);
    TestFalse(TEXT("ARM002 is an attacking weapon"), N1.bPassive);
    const FGridWeaponTooltipFact* Dice1 = WeaponTooltipTests::Find(N1, TEXT("WeaponDice"));
    const FGridWeaponTooltipFact* Dice9 = WeaponTooltipTests::Find(N9, TEXT("WeaponDice"));
    const FGridWeaponTooltipFact* Minimum9 = WeaponTooltipTests::Find(N9, TEXT("WeaponMinDamage"));
    const FGridWeaponTooltipFact* Maximum9 = WeaponTooltipTests::Find(N9, TEXT("WeaponMaxDamage"));
    if (!TestNotNull(TEXT("N1 dice fact"), Dice1) ||
        !TestNotNull(TEXT("N9 dice fact"), Dice9) ||
        !TestNotNull(TEXT("N9 minimum"), Minimum9) ||
        !TestNotNull(TEXT("N9 maximum"), Maximum9))
        return false;
    const FString N1Suffix = P1.Weapon.DamageDice.FlatBonus == 0
        ? FString() : FString::Printf(TEXT(" + %d"), P1.Weapon.DamageDice.FlatBonus);
    TestEqual(TEXT("N1 formula exactly"), Dice1->ValueText.ToString(),
        FString::Printf(TEXT("%dd%d%s"), P1.Weapon.DamageDice.DiceCount,
            P1.Weapon.DamageDice.DiceSides, *N1Suffix));
    TestEqual(TEXT("N9 minimum from actual dice"), Minimum9->ValueText.ToString(),
        FString::FromInt(P9.Weapon.DamageDice.GetMinimumDamage()));
    TestEqual(TEXT("N9 maximum from actual dice"), Maximum9->ValueText.ToString(),
        FString::FromInt(P9.Weapon.DamageDice.GetMaximumDamage()));
    const FGridWeaponTooltipFact* ActionPoints = WeaponTooltipTests::Find(N9, TEXT("WeaponAP"));
    if (TestNotNull(TEXT("N9 PA fact"), ActionPoints))
    {
        TestEqual(TEXT("N9 PA consistent with canonical catalogue"),
            ActionPoints->ValueText.ToString(),
            FString::FromInt(P9.Weapon.ActionPointCost));
    }
    TestTrue(TEXT("N1 and N9 formulas differ when catalogue changes"),
        P1.Weapon.DamageDice.FlatBonus == P9.Weapon.DamageDice.FlatBonus ||
            !Dice1->ValueText.EqualTo(Dice9->ValueText));
    TestEqual(TEXT("Production ItemSystem root untouched"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipPassiveLegacyTest,
    "Grimrock.UI.WeaponTooltip04.PassiveShieldAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipPassiveLegacyTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    const FName ShieldId(TEXT("Weapon_RoundShield_01"));
    FGridResolvedItem Shield;
    if (!TestTrue(TEXT("Canonical ARM075 N12 resolves"),
            Fixture.Resolve(ShieldId, 12, Shield)) ||
        !TestTrue(TEXT("Shield is a passive ARM with physical armor"),
            Shield.bIsArmament && Shield.Weapon.PhysicalArmor > 0 &&
            !Shield.Weapon.DamageDice.bEnabled))
        return false;

    FGridWeaponTooltipPresentation Passive;
    if (!TestTrue(TEXT("Passive level 12 resolved"),
        FGridWeaponTooltipProjection::Build(nullptr,
            WeaponTooltipTests::MakeCanonicalItem(ShieldId, 12),
            Passive, Fixture.Registry.Get())))
        return false;
    TestTrue(TEXT("Passive shield flagged as passive"), Passive.bPassive);
    TestTrue(TEXT("No fictional attack dice"), Passive.DiceText.IsEmpty());
    TestTrue(TEXT("No fictional weapon PA"),
        WeaponTooltipTests::Find(Passive, TEXT("WeaponAP")) == nullptr);
    TestTrue(TEXT("No fictional damage line"),
        WeaponTooltipTests::Find(Passive, TEXT("WeaponDice")) == nullptr);
    const FGridWeaponTooltipFact* Armor =
        WeaponTooltipTests::Find(Passive, TEXT("WeaponPhysicalArmor"));
    if (!TestNotNull(TEXT("Shield physical armor shown"), Armor))
        return false;
    TestEqual(TEXT("Physical armor uses actual level 12"),
        Armor->ValueText.ToString(),
        FString::FromInt(Shield.Weapon.PhysicalArmor));

    // A deliberately unregistered legacy object is a negative characterization
    // fixture only. It is not an authorized production data source.
    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>();
    Legacy->ItemDefinitionId = TEXT("Legacy_ItemWithoutCatalogue");
    Legacy->DisplayName = FText::FromString(TEXT("Legacy item"));
    FGridWeaponTooltipPresentation LegacyView;
    TestFalse(TEXT("Legacy is not retroactively migrated"),
        FGridWeaponTooltipProjection::Build(
            Legacy, WeaponTooltipTests::MakeItem(Legacy, 0), LegacyView));
    TestFalse(TEXT("Legacy produces no new weapon profile"),
        LegacyView.bHasProfile);

    FGridWeaponTooltipPresentation InvalidLevel;
    TestFalse(TEXT("No guessing a level outside N1-N20"),
        FGridWeaponTooltipProjection::Build(nullptr,
            WeaponTooltipTests::MakeCanonicalItem(ShieldId, 21),
            InvalidLevel, Fixture.Registry.Get()));
    TestFalse(TEXT("Invalid level remains empty"),
        InvalidLevel.bHasProfile);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipSlotIntegrationTest,
    "Grimrock.UI.WeaponTooltip04.InventorySlotProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipSlotIntegrationTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    const UGridItemSubsystem* Registry = Fixture.Scope.Registry;
    const FName DaggerId(TEXT("Weapon_AssassinDagger_01"));
    FGridResolvedItem Dagger;
    if (!TestNotNull(TEXT("World-bound ItemSystem registry"), Registry) ||
        !TestTrue(TEXT("Canonical ARM002 N9 resolves"),
            Registry && Registry->ResolveItem(DaggerId, 9, Dagger)) ||
        !TestTrue(TEXT("N9 dagger has a real weapon profile"),
            Dagger.bIsArmament && Dagger.Weapon.DamageDice.bEnabled))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("World-bound canonical inventory"), Inventory) ||
        !TestNull(TEXT("No old ARM002 definition registered"),
            Inventory->FindItemDefinition(DaggerId)))
        return false;
    FGridCharacterInventoryState Hero;
    Hero.CharacterId = FGuid::NewGuid();
    Hero.DisplayName = FText::FromString(TEXT("Weapon profile preview"));
    Hero.InventorySlots.SetNum(2);
    Inventory->PartyInventoryState.ActiveCharacters.Add(Hero);
    Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
    Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
    Inventory->PartyInventoryState.MaxActiveCharacters = 6;
    // The transient parent widget must discover ItemSystem through the
    // inventory component's GameInstance rather than an old definition map.

    UGridInventoryWidget* Parent = NewObject<UGridInventoryWidget>();
    Parent->InventoryComponent = Inventory;
    UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
    Slot->SetOwnerInventoryWidget(Parent);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    FGridItemInstance Item = WeaponTooltipTests::MakeCanonicalItem(DaggerId, 9);
    Slot->SetItem(Item);

    // Attach a genuinely equipped N1 to exercise the existing inventory comparison route.
    FGridItemInstance Equipped = WeaponTooltipTests::MakeCanonicalItem(DaggerId, 1);
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
    FGridCanonicalRegistryFixture Fixture;
    const FName DaggerId(TEXT("Weapon_AssassinDagger_01"));
    const FName ShieldId(TEXT("Weapon_RoundShield_01"));
    FGridResolvedItem P9, P1, S12, S1;
    if (!TestTrue(TEXT("Canonical ARM002 N9 resolves"), Fixture.Resolve(DaggerId, 9, P9)) ||
        !TestTrue(TEXT("Canonical ARM002 N1 resolves"), Fixture.Resolve(DaggerId, 1, P1)) ||
        !TestTrue(TEXT("Canonical ARM075 N12 resolves"), Fixture.Resolve(ShieldId, 12, S12)) ||
        !TestTrue(TEXT("Canonical ARM075 N1 resolves"), Fixture.Resolve(ShieldId, 1, S1)))
        return false;
    UGridItemSystemAsset* Root = Fixture.Registry->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Read-only production ItemSystem"), Root)) return false;
    const bool bBeforeRoot = Root->GetOutermost()->IsDirty();

    TArray<FGridWeaponTooltipComparisonFact> Facts;
    const FGridItemInstance N9 = WeaponTooltipTests::MakeCanonicalItem(DaggerId, 9);
    const FGridItemInstance N1 = WeaponTooltipTests::MakeCanonicalItem(DaggerId, 1);
    if (!TestTrue(TEXT("Two canonical ARM002 weapon levels comparable"),
        FGridWeaponTooltipProjection::BuildComparison(
            nullptr, N9, nullptr, N1, Facts, Fixture.Registry.Get())))
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
    TestEqual(TEXT("Minimum damage from exact canonical levels"), Min->Delta,
        static_cast<float>(P9.Weapon.DamageDice.GetMinimumDamage() -
            P1.Weapon.DamageDice.GetMinimumDamage()));
    TestFalse(TEXT("Dice formulas are not arithmetic deltas"), Dice->bHasDelta);

    TArray<FGridWeaponTooltipComparisonFact> Incompatible;
    TestFalse(TEXT("Weapon and shield have incomparable attack roles"),
        FGridWeaponTooltipProjection::BuildComparison(
            nullptr, N9, nullptr,
            WeaponTooltipTests::MakeCanonicalItem(ShieldId, 12),
            Incompatible, Fixture.Registry.Get()));
    TestEqual(TEXT("Incompatible roles leave no partial comparison"),
        Incompatible.Num(), 0);

    TArray<FGridWeaponTooltipComparisonFact> ShieldFacts;
    if (!TestTrue(TEXT("Two canonical passive shield levels comparable"),
        FGridWeaponTooltipProjection::BuildComparison(
            nullptr, WeaponTooltipTests::MakeCanonicalItem(ShieldId, 12),
            nullptr, WeaponTooltipTests::MakeCanonicalItem(ShieldId, 1),
            ShieldFacts, Fixture.Registry.Get())))
        return false;
    TestTrue(TEXT("No fictitious shield dice"),
        ShieldFacts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& F)
        {
            return F.StatId == TEXT("WeaponDice");
        }) == nullptr);
    const auto* Armor = ShieldFacts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& F)
    {
        return F.StatId == TEXT("WeaponPhysicalArmor");
    });
    if (!TestNotNull(TEXT("Canonical shield armor comparison"), Armor))
        return false;
    TestEqual(TEXT("Shield delta from canonical N12 and N1"), Armor->Delta,
        static_cast<float>(S12.Weapon.PhysicalArmor - S1.Weapon.PhysicalArmor));
    TestEqual(TEXT("ItemSystem root not mutated"),
        Root->GetOutermost()->IsDirty(), bBeforeRoot);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipPAValenceTest,
    "Grimrock.UI.WeaponTooltip04.ActionPointDeltaSemantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipPAValenceTest::RunTest(const FString&)
{
    // Verify tooltip comparison semantics with actual production N1 items.
    // No source DA_WeaponCatalogue, no transient legacy production authority,
    // and no mutation of an installed ItemSystem asset.
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real canonical tooltip registry"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production ItemSystem"), Root))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();

    TArray<FName> Ids;
    TArray<FGridResolvedItem> Physical;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition)) continue;
        FGridResolvedItem Profile;
        if (Fixture.Resolve(Definition->ItemId, 1, Profile) &&
            Profile.bIsArmament && Profile.Weapon.DamageDice.bEnabled)
        {
            Ids.Add(Definition->ItemId);
            Physical.Add(MoveTemp(Profile));
        }
    }
    TestTrue(TEXT("Production registry contains offensive N1 ARM"), Ids.Num() > 1);
    if (Ids.Num() < 2) return false;

    FName CheaperId = NAME_None, CostlierId = NAME_None;
    FName StrongerId = NAME_None, WeakerId = NAME_None;
    FName OtherDamageId = NAME_None, OtherBaseId = NAME_None;
    int32 ExpectedMinGain = 0, ExpectedMaxGain = 0, ExpectedApDelta = 0;
    for (int32 A = 0; A < Physical.Num(); ++A)
    {
        for (int32 B = 0; B < Physical.Num(); ++B)
        {
            if (A == B) continue;
            const FGridCanonicalResolvedWeapon& Candidate = Physical[A].Weapon;
            const FGridCanonicalResolvedWeapon& Equipped = Physical[B].Weapon;
            if (CheaperId.IsNone() && Candidate.ActionPointCost > 0 &&
                Candidate.ActionPointCost < Equipped.ActionPointCost)
            {
                CheaperId = Ids[A];
                CostlierId = Ids[B];
                ExpectedApDelta = Candidate.ActionPointCost - Equipped.ActionPointCost;
            }
            if (StrongerId.IsNone() &&
                Candidate.DamageType == EGridDamageType::Physical &&
                Equipped.DamageType == EGridDamageType::Physical &&
                Candidate.ScalingAttribute != Equipped.ScalingAttribute &&
                Candidate.DamageDice.GetMinimumDamage() > Equipped.DamageDice.GetMinimumDamage() &&
                Candidate.DamageDice.GetMaximumDamage() > Equipped.DamageDice.GetMaximumDamage())
            {
                StrongerId = Ids[A];
                WeakerId = Ids[B];
                ExpectedMinGain = Candidate.DamageDice.GetMinimumDamage() -
                    Equipped.DamageDice.GetMinimumDamage();
                ExpectedMaxGain = Candidate.DamageDice.GetMaximumDamage() -
                    Equipped.DamageDice.GetMaximumDamage();
            }
            if (OtherDamageId.IsNone() && Candidate.DamageType != Equipped.DamageType)
            {
                OtherDamageId = Ids[A];
                OtherBaseId = Ids[B];
            }
        }
    }
    if (!TestTrue(TEXT("Real production weapons demonstrate cheaper action points"),
        !CheaperId.IsNone()) ||
        !TestTrue(TEXT("Real physical N1 weapons differ in scaling and raw damage"),
            !StrongerId.IsNone()) ||
        !TestTrue(TEXT("Real offensive weapons include distinct primary damage types"),
            !OtherDamageId.IsNone()))
        return false;

    auto Compare = [&Fixture](FName CandidateId, FName EquippedId,
        TArray<FGridWeaponTooltipComparisonFact>& Out)
    {
        return FGridWeaponTooltipProjection::BuildComparison(
            nullptr, WeaponTooltipTests::MakeCanonicalItem(CandidateId, 1),
            nullptr, WeaponTooltipTests::MakeCanonicalItem(EquippedId, 1),
            Out, Fixture.Registry.Get());
    };
    TArray<FGridWeaponTooltipComparisonFact> Facts;
    if (!TestTrue(TEXT("Two real ARM compare their lower PA expense"),
        Compare(CheaperId, CostlierId, Facts)))
        return false;
    const auto* Ap = Facts.FindByPredicate([](const FGridWeaponTooltipComparisonFact& Fact)
    {
        return Fact.StatId == TEXT("WeaponAP");
    });
    if (!TestNotNull(TEXT("Canonical PA comparison fact"), Ap))
        return false;
    TestTrue(TEXT("A lower PA cost is a meaningful nonzero delta"), Ap->bHasDelta);
    TestEqual(TEXT("Signed native PA difference"), Ap->Delta,
        static_cast<float>(ExpectedApDelta));
    TestTrue(TEXT("Lower PA is green/beneficial"),
        Ap->DeltaState == EGridItemTooltipDeltaState::Positive);
    TestEqual(TEXT("Signed lower-PA label"), Ap->DeltaText.ToString(),
        FString::Printf(TEXT("%d"), ExpectedApDelta));

    Facts.Reset();
    if (!TestTrue(TEXT("Different scaling attributes compare physical raw damage"),
        Compare(StrongerId, WeakerId, Facts)))
        return false;
    for (const auto& Expected : {
        TPair<FName, int32>(TEXT("WeaponMinDamage"), ExpectedMinGain),
        TPair<FName, int32>(TEXT("WeaponMaxDamage"), ExpectedMaxGain) })
    {
        const auto* Raw = Facts.FindByPredicate(
            [&Expected](const FGridWeaponTooltipComparisonFact& Fact)
            { return Fact.StatId == Expected.Key; });
        if (!TestNotNull(TEXT("Canonical raw-damage comparison fact"), Raw))
            continue;
        TestTrue(TEXT("Different scaling still permits comparable physical raw points"),
            Raw->bHasDelta);
        TestEqual(TEXT("Signed raw damage delta"), Raw->Delta,
            static_cast<float>(Expected.Value));
        TestEqual(TEXT("Positive raw damage delta label"),
            Raw->DeltaText.ToString(), FString::Printf(TEXT("+%d"), Expected.Value));
        TestTrue(TEXT("Higher raw damage is green/beneficial"),
            Raw->DeltaState == EGridItemTooltipDeltaState::Positive);
    }

    Facts.Reset();
    if (!TestTrue(TEXT("Different elemental damage types still render facts"),
        Compare(OtherDamageId, OtherBaseId, Facts)))
        return false;
    const auto* Min = Facts.FindByPredicate(
        [](const FGridWeaponTooltipComparisonFact& Fact)
        { return Fact.StatId == TEXT("WeaponMinDamage"); });
    if (TestNotNull(TEXT("Different-type raw damage row is shown"), Min))
        TestFalse(TEXT("Different damage types suppress misleading numeric delta"),
            Min->bHasDelta);

    TestEqual(TEXT("Production ItemSystem was never changed"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTooltipPhysicalSubtypeRawDamageTest,
    "Grimrock.UI.WeaponTooltip04.PhysicalSubtypeRawDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponTooltipPhysicalSubtypeRawDamageTest::RunTest(const FString&)
{
    // Real ARM025 N1 (piercing) versus equipped ARM001 N1 (slashing),
    // projected exclusively from the single production ItemSystem.
    FGridCanonicalRegistryFixture Fixture;
    const FName PickId(TEXT("Weapon_WarPick_01"));
    const FName DaggerId(TEXT("Weapon_Dagger_01"));
    FGridResolvedItem Pick, Dagger;
    if (!TestTrue(TEXT("Canonical ARM025 War Pick N1 resolves"),
            Fixture.Resolve(PickId, 1, Pick)) ||
        !TestTrue(TEXT("Canonical ARM001 Dagger N1 resolves"),
            Fixture.Resolve(DaggerId, 1, Dagger)) ||
        !TestTrue(TEXT("Both are real ARM weapons"),
            Pick.bIsArmament && Dagger.bIsArmament))
        return false;
    UGridItemSystemAsset* Root = Fixture.Registry->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Read-only production ItemSystem"), Root))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();

    TestTrue(TEXT("Both damage types are physical"),
        Pick.Weapon.DamageType == EGridDamageType::Physical &&
        Dagger.Weapon.DamageType == EGridDamageType::Physical);
    TestTrue(TEXT("Piercing versus slashing are different physical subtypes"),
        Pick.Weapon.PhysicalSubtype == EGridPhysicalDamageSubtype::Piercing &&
        Dagger.Weapon.PhysicalSubtype == EGridPhysicalDamageSubtype::Slashing);

    TArray<FGridWeaponTooltipComparisonFact> Facts;
    if (!TestTrue(TEXT("Canonical War Pick / Dagger comparison built"),
        FGridWeaponTooltipProjection::BuildComparison(
            nullptr, WeaponTooltipTests::MakeCanonicalItem(PickId, 1),
            nullptr, WeaponTooltipTests::MakeCanonicalItem(DaggerId, 1),
            Facts, Fixture.Registry.Get())))
        return false;

    for (const FName StatId : { FName(TEXT("WeaponMinDamage")), FName(TEXT("WeaponMaxDamage")) })
    {
        const auto* Damage = Facts.FindByPredicate(
            [StatId](const FGridWeaponTooltipComparisonFact& Fact)
            { return Fact.StatId == StatId; });
        if (!TestNotNull(TEXT("Canonical physical raw damage row exists"), Damage))
            return false;
        TestTrue(TEXT("Different physical subtypes allow raw numeric delta"), Damage->bHasDelta);
        TestEqual(TEXT("War Pick gains three raw damage points"), Damage->Delta, 3.f);
        TestEqual(TEXT("Delta is signed +3"), Damage->DeltaText.ToString(), FString(TEXT("+3")));
        TestTrue(TEXT("Raw damage increase is beneficial"),
            Damage->DeltaState == EGridItemTooltipDeltaState::Positive);
    }

    const auto* DamageType = Facts.FindByPredicate(
        [](const FGridWeaponTooltipComparisonFact& Fact)
        { return Fact.StatId == TEXT("WeaponDamageType"); });
    const auto* Dice = Facts.FindByPredicate(
        [](const FGridWeaponTooltipComparisonFact& Fact)
        { return Fact.StatId == TEXT("WeaponDice"); });
    if (!TestNotNull(TEXT("Both physical subtypes remain visible"), DamageType) ||
        !TestNotNull(TEXT("Dice remain visible"), Dice))
        return false;
    TestFalse(TEXT("Damage type has no arithmetic delta"), DamageType->bHasDelta);
    TestFalse(TEXT("Dice formula has no arithmetic delta"), Dice->bHasDelta);
    TestFalse(TEXT("Different physical subtype labels remain distinct"),
        DamageType->CandidateText.EqualTo(DamageType->EquippedText));
    TestEqual(TEXT("Canonical ItemSystem remains clean"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
