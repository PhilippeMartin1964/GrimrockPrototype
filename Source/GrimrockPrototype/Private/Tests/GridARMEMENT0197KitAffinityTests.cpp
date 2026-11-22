#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace ARMEMENT0197
{
    const FGridCanonicalWeaponArchetype* Find(FName Id)
    {
        UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
        return Root && Root->WeaponCatalogue ? Root->WeaponCatalogue->Find(Id) : nullptr;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197QualifiedPriest,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.QualifiedPriestAffinities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0197QualifiedPriest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real canonical registry"), Fixture.bReady)) return false;
    const TCHAR* ReviewArms[] = {TEXT("ARM027"),TEXT("ARM031"),TEXT("ARM034"),TEXT("ARM037")};
    for (const TCHAR* Id : ReviewArms)
    {
        const FGridCanonicalWeaponArchetype* Arm = ARMEMENT0197::Find(FName(Id));
        if (!TestNotNull(TEXT("Qualified priest ARM retained"), Arm)) continue;
        TestTrue(TEXT("Priest still secondary-qualified, not unconditionally approved"),
            Arm->SecondaryClassIds.Contains(TEXT("Priest")) &&
            Arm->SecondarySourceLabel.Contains(TEXT("Prêtre")));
    }
    const FGridCanonicalWeaponArchetype* GreatAxe = ARMEMENT0197::Find(TEXT("ARM031"));
    if (TestNotNull(TEXT("Great axe affinity"), GreatAxe))
        TestEqual(TEXT("Warrior remains primary"), GreatAxe->PrimaryClassId, FName(TEXT("Warrior")));
    const FGridCanonicalWeaponArchetype* Shield = ARMEMENT0197::Find(TEXT("ARM075"));
    if (TestNotNull(TEXT("Round shield affinity"), Shield))
        TestTrue(TEXT("Priest retains secondary shield affinity"),
            Shield->SecondaryClassIds.Contains(TEXT("Priest")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197IndependentFragments,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.IndependentSourceQualifiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0197IndependentFragments::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical ItemSystem"), Fixture.bReady)) return false;
    const FGridCanonicalWeaponArchetype* Arm = ARMEMENT0197::Find(TEXT("ARM031"));
    if (!TestNotNull(TEXT("Real qualified weapon"), Arm)) return false;
    TestTrue(TEXT("Qualified Priest coexists with different class source"),
        Arm->SecondaryClassIds.Contains(TEXT("Priest")) &&
        Arm->PrimaryClassId != FName(TEXT("Priest")) &&
        !Arm->SecondarySourceLabel.IsEmpty());
    const FGridCanonicalWeaponArchetype* Bow = ARMEMENT0197::Find(TEXT("ARM045"));
    if (TestNotNull(TEXT("Real Ranger source"), Bow))
        TestEqual(TEXT("Bow primary stays Ranger"), Bow->PrimaryClassId, FName(TEXT("Ranger")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0197NoGameplayMutation,
    "Grimrock.RPG.ARMEMENT01.9.KitApproval.NoGameplayOrCatalogueMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0197NoGameplayMutation::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Single authoritative ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production root"), Root)) return false;
    const bool bDirty = Root->GetOutermost()->IsDirty();
    FGridResolvedItem Before, After;
    if (!TestTrue(TEXT("Canonical GreatAxe resolves"),
        Fixture.Resolve(TEXT("Weapon_GreatAxe_01"), 1, Before))) return false;
    const FGridCanonicalWeaponArchetype* Affinity = ARMEMENT0197::Find(TEXT("ARM031"));
    TestNotNull(TEXT("Class advice lookup is independent"), Affinity);
    TestTrue(TEXT("Same weapon numbers after affinity lookup"),
        Fixture.Resolve(Before.ItemId, 1, After) &&
        After.Weapon.ActionPointCost == Before.Weapon.ActionPointCost &&
        After.Weapon.DamageDice.DiceCount == Before.Weapon.DamageDice.DiceCount);
    TestEqual(TEXT("No canonical asset mutated during class advice read"),
        Root->GetOutermost()->IsDirty(), bDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
