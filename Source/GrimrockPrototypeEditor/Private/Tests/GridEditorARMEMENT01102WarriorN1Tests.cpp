#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "ARMEMENT/GridWarriorN1ItemAuthoringSpecs.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace ARMEMENT01102Tests
{
    bool ReadWarriorRecipes(TMap<FString, TPair<FName, FName>>& Out)
    {
        Out.Reset();
        FString Text;
        const FString Path = FPaths::Combine(
            FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        if (!FFileHelper::LoadFileToString(Text, *Path))
        {
            return false;
        }
        FCsvParser Parser(Text);
        const auto& Rows = Parser.GetRows();
        if (Rows.Num() < 2 || Rows[0].Num() != 15 ||
            FCString::Strcmp(Rows[0][3], TEXT("Weapon1ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][6], TEXT("Weapon2ArchetypeId")) != 0)
        {
            return false;
        }

        for (int32 I = 1; I < Rows.Num(); ++I)
        {
            const TArray<const TCHAR*>& Row = Rows[I];
            if (Row.Num() != 15 || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (FCString::Strcmp(Row[0], TEXT("Warrior")) == 0)
            {
                if (FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0 ||
                    Out.Contains(FString(Row[1])))
                {
                    return false;
                }
                Out.Add(FString(Row[1]), TPair<FName, FName>(FName(Row[3]), FName(Row[6])));
            }
        }
        return Out.Num() == 3;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01102WarriorSource,
    "Grimrock.RPG.ARMEMENT01.10.Warrior.ApprovedN1Source",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01102WarriorSource::RunTest(const FString&)
{
    TMap<FString, TPair<FName, FName>> Recipes;
    if (!TestTrue(TEXT("Three approved Warrior branches"),
        ARMEMENT01102Tests::ReadWarriorRecipes(Recipes))) return false;
    const auto* Guardian = Recipes.Find(TEXT("Guardian"));
    const auto* Breaker = Recipes.Find(TEXT("Breaker"));
    const auto* Master = Recipes.Find(TEXT("WeaponMaster"));
    if (!TestNotNull(TEXT("Guardian recipe"), Guardian) ||
        !TestNotNull(TEXT("Breaker recipe"), Breaker) ||
        !TestNotNull(TEXT("WeaponMaster recipe"), Master)) return false;
    TestEqual(TEXT("Guardian primary N1"), Guardian->Key, FName(TEXT("ARM014")));
    TestEqual(TEXT("Guardian off-hand shield"), Guardian->Value, FName(TEXT("ARM075")));
    TestEqual(TEXT("Breaker primary GreatAxe"), Breaker->Key, FName(TEXT("ARM031")));
    TestEqual(TEXT("Breaker reserves no second weapon"), Breaker->Value, FName(TEXT("None")));
    TestEqual(TEXT("WeaponMaster LongSword"), Master->Key, FName(TEXT("ARM015")));
    TestEqual(TEXT("WeaponMaster WarPick"), Master->Value, FName(TEXT("ARM025")));

    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Single installed canonical ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Root"), Root) ||
        !TestNotNull(TEXT("Unique numeric weapon catalogue"), Root->WeaponCatalogue.Get()))
        return false;
    TSet<FName> UniqueItems, UniqueArchetypes;
    for (const ARMEMENT01102::FWarriorItemSpec& Spec : ARMEMENT01102::WarriorItems)
    {
        const FName ItemId(Spec.ItemDefinitionId), ArmId(Spec.ArchetypeId);
        TestFalse(TEXT("No duplicate Warrior ItemId"), UniqueItems.Contains(ItemId));
        TestFalse(TEXT("No duplicate Warrior archetype"), UniqueArchetypes.Contains(ArmId));
        UniqueItems.Add(ItemId);
        UniqueArchetypes.Add(ArmId);
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("Real Warrior N1 profile"),
            Fixture.Resolve(ItemId, 1, N1))) continue;
        TestTrue(TEXT("Canonical source has proper archetype, dice and physical hand"),
            N1.bIsArmament && N1.ArchetypeId == ArmId &&
            N1.Weapon.DamageDice.bEnabled &&
            N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
        const FGridCanonicalWeaponArchetype* Numeric = Root->WeaponCatalogue->Find(ArmId);
        if (TestNotNull(TEXT("Single authored Warrior class affinity"), Numeric))
            TestTrue(TEXT("Warrior is explicitly primary/secondary and never inferred"),
                Numeric->PrimaryClassId == FName(TEXT("Warrior")) ||
                Numeric->SecondaryClassIds.Contains(TEXT("Warrior")));
        FGridResolvedBoundAction Bound;
        if (TestTrue(TEXT("One explicit canonical attack for real Warrior weapon"),
            Fixture.Registry->ResolveBoundAttackForHand(
                ItemId, 1, FName(Spec.AttackId), EGridEquipmentSlot::MainHand, Bound)))
            TestTrue(TEXT("Authoritative action is valid and not empty"),
                Bound.IsValid() && Bound.ActionId == FName(Spec.AttackId));
        TestEqual(TEXT("Only GreatAxe occupies both hands"),
            N1.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded,
            ArmId == FName(TEXT("ARM031")));
    }
    TestEqual(TEXT("Exactly four Warrior N1 identities"), UniqueItems.Num(), 4);
    TestEqual(TEXT("Exactly four Warrior archetypes"), UniqueArchetypes.Num(), 4);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01102WarriorGuard,
    "Grimrock.RPG.ARMEMENT01.10.Warrior.ExistingAssetsGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01102WarriorGuard::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production canonical ItemSystem configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real root"), Root) ||
        !TestNotNull(TEXT("Real numeric catalogue"), Root->WeaponCatalogue.Get()) ||
        !TestNotNull(TEXT("Real visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bNumbersDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();
    const bool bVisualsDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    int32 Present = 0;
    for (const ARMEMENT01102::FWarriorItemSpec& Spec : ARMEMENT01102::WarriorItems)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical Warrior item is materialized"),
            Fixture.Resolve(FName(Spec.ItemDefinitionId), 1, Item))) continue;
        ++Present;
        TestTrue(TEXT("Unique registered armament identity and visuals"),
            Item.ArchetypeId == FName(Spec.ArchetypeId) && Item.MaxStack == 1 &&
            !Item.ItemIcon.IsNull() && !Item.WorldMesh.IsNull() && !Item.EquippedMesh.IsNull());
        const FGridCanonicalWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(Item.ArchetypeId);
        if (TestNotNull(TEXT("Exactly one unique visual binding"), Visual))
            TestEqual(TEXT("Visual join cannot point at a different weapon"),
                Item.VisualId, Visual->VisualId);
        FGridResolvedBoundAction Attack;
        if (TestTrue(TEXT("One real MainHand attack"),
            Fixture.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, FName(Spec.AttackId), EGridEquipmentSlot::MainHand, Attack)))
        {
            TestEqual(TEXT("Action cost comes from canonical numeric source"),
                Attack.Action.ActionPointCost, Item.Weapon.ActionPointCost);
            TestEqual(TEXT("Action range comes from canonical numeric source"),
                Attack.Action.RangeCells, Item.Weapon.RangeCells);
        }
    }
    TestEqual(TEXT("All four Warrior items are really materialized"), Present, 4);
    TestEqual(TEXT("No ItemSystem root mutation"), Root->GetOutermost()->IsDirty(), bRootDirty);
    TestEqual(TEXT("No numeric catalogue mutation"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumbersDirty);
    TestEqual(TEXT("No visual catalogue mutation"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualsDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
