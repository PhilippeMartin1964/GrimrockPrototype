#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace ARMEMENT01111
{
    struct FSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* ClassId;
        const TCHAR* BranchId;
        const TCHAR* RequiredSlot;
    };

    static const FSpec Specs[] = {
        { TEXT("ARM002"), TEXT("Rogue"), TEXT("Assassin"), TEXT("MainHand") },
        { TEXT("ARM003"), TEXT("Rogue"), TEXT("Saboteur"), TEXT("MainHand") },
        { TEXT("ARM013"), TEXT("Rogue"), TEXT("Shadow"), TEXT("OffHand") },
        { TEXT("ARM020"), TEXT("Priest"), TEXT("Protection"), TEXT("MainHand") }
    };

    bool IsApprovedPlacement(const FSpec& Spec, const TArray<TArray<const TCHAR*>>& Rows)
    {
        int32 Matches = 0;
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Row = Rows[Index];
            if (Row.Num() != 15 || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (FCString::Strcmp(Row[0], Spec.ClassId) != 0 ||
                FCString::Strcmp(Row[1], Spec.BranchId) != 0 ||
                FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0)
            {
                continue;
            }

            const bool bFirst = FCString::Strcmp(Row[3], Spec.ArchetypeId) == 0 &&
                FCString::Strcmp(Row[5], Spec.RequiredSlot) == 0 &&
                FCString::Strcmp(Row[4], TEXT("1")) == 0;
            const bool bSecond = FCString::Strcmp(Row[6], Spec.ArchetypeId) == 0 &&
                FCString::Strcmp(Row[8], Spec.RequiredSlot) == 0 &&
                FCString::Strcmp(Row[7], TEXT("1")) == 0;
            Matches += (bFirst || bSecond) ? 1 : 0;
        }
        return Matches == 1;
    }

    FString AsCsvBool(bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01111ApprovedSources,
    "Grimrock.RPG.ARMEMENT01.11.1.ApprovedN1Sources",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01111ApprovedSources::RunTest(const FString&)
{
    FString Csv;
    const FString Path = FPaths::Combine(
        FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
    if (!TestTrue(TEXT("18 approved starter-kit recipes exist"),
        FFileHelper::LoadFileToString(Csv, *Path))) return false;
    FCsvParser Parser(Csv);
    const auto& Rows = Parser.GetRows();
    if (!TestTrue(TEXT("Approved recipe schema is intact"),
        Rows.Num() >= 19 && Rows[0].Num() == 15 &&
        FCString::Strcmp(Rows[0][13], TEXT("ApprovalState")) == 0))
        return false;
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Single installed native ItemSystem configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real weapon catalogue"), Root) ||
        !TestNotNull(TEXT("Numeric class affinities"), Root->WeaponCatalogue.Get()))
        return false;
    const bool bDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();
    TSet<FName> Seen;
    for (const ARMEMENT01111::FSpec& Spec : ARMEMENT01111::Specs)
    {
        const FName ArmId(Spec.ArchetypeId);
        TestFalse(TEXT("No repeated melee archetype"), Seen.Contains(ArmId));
        Seen.Add(ArmId);
        TestTrue(TEXT("Real N1 melee belongs to its approved starter recipe"),
            ARMEMENT01111::IsApprovedPlacement(Spec, Rows));
        const FGridWeaponArchetype* Authoring =
            Root->WeaponCatalogue->Find(ArmId);
        if (TestNotNull(TEXT("Canonical numeric class affinity"), Authoring))
            TestTrue(TEXT("Class explicitly primary or secondary"),
                Authoring->PrimaryClassId == FName(Spec.ClassId) ||
                Authoring->SecondaryClassIds.Contains(FName(Spec.ClassId)));
        FGridResolvedItem N1;
        const FName ItemId(
            ArmId == FName(TEXT("ARM002")) ? TEXT("Weapon_AssassinDagger_01") :
            ArmId == FName(TEXT("ARM003")) ? TEXT("Weapon_Stiletto_01") :
            ArmId == FName(TEXT("ARM013")) ? TEXT("Weapon_MainGauche_01") :
            TEXT("Weapon_Mace_01"));
        if (!TestTrue(TEXT("Approved melee N1 resolves from native ItemId"),
            Fixture.Resolve(ItemId, 1, N1))) continue;
        TestTrue(TEXT("Four approved attacks retain dice and physical hand"),
            N1.bIsArmament && N1.ArchetypeId == ArmId &&
            N1.Weapon.DamageDice.bEnabled &&
            N1.Weapon.CanEquipInHand(
                FCString::Strcmp(Spec.RequiredSlot, TEXT("OffHand")) == 0
                    ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("N1 handed melee attack still authored"),
            N1.AvailableActions.Num() == 1 &&
            N1.Weapon.HasAttackSourceInHand(
                FCString::Strcmp(Spec.RequiredSlot, TEXT("OffHand")) == 0
                    ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand));
    }
    TestEqual(TEXT("Exactly four approved melee identity branches"), Seen.Num(), 4);
    TestEqual(TEXT("Native numeric source never dirtied"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bDirty);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01111Readiness,
    "Grimrock.RPG.ARMEMENT01.11.1.MeleeAssetReadiness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01111Readiness::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Authoritative native ItemSystem available"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Native root"), Root) ||
        !TestNotNull(TEXT("Native visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    TArray<FString> Report = {
        TEXT("ArchetypeId,VisualMapped,IconLoaded,WorldMeshLoaded,ItemCount,ItemId,Valid,MainAttack,OffAttack,Status")
    };
    int32 Ready = 0;
    const struct {const TCHAR* Arm;const TCHAR* Item;const TCHAR* Action;} Items[] =
    {
        {TEXT("ARM002"),TEXT("Weapon_AssassinDagger_01"),TEXT("Attack_AssassinDagger_01")},
        {TEXT("ARM003"),TEXT("Weapon_Stiletto_01"),TEXT("Attack_Stiletto_01")},
        {TEXT("ARM013"),TEXT("Weapon_MainGauche_01"),TEXT("Attack_MainGauche_01")},
        {TEXT("ARM020"),TEXT("Weapon_Mace_01"),TEXT("Attack_Mace_01")}
    };
    for (const auto& Spec : Items)
    {
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("Canonical real N1 melee item"),
            Fixture.Resolve(FName(Spec.Item), 1, N1))) continue;
        const FGridWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(FName(Spec.Arm));
        const bool bMapped = Visual && Visual->VisualId == N1.VisualId;
        const bool bIcon = N1.ItemIcon.LoadSynchronous() != nullptr;
        const bool bMesh = N1.WorldMesh.LoadSynchronous() != nullptr;
        const bool bValid = N1.bIsArmament && N1.ArchetypeId == FName(Spec.Arm) &&
            N1.ItemLevel == 1 && N1.MaxStack == 1;
        const bool bMain = N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand);
        const bool bOff = N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand);
        const bool bGood = bMapped && bIcon && bMesh && bValid &&
            N1.AvailableActions.Num() == 1 &&
            N1.AvailableActions[0].ActionId == FName(Spec.Action) &&
            (bMain || bOff);
        TestTrue(TEXT("Real melee item is complete and can be playtested"), bGood);
        if (bGood) ++Ready;
        Report.Add(FString::Printf(TEXT("%s,%s,%s,%s,1,%s,%s,%s,%s,%s"),
            Spec.Arm,
            *ARMEMENT01111::AsCsvBool(bMapped),
            *ARMEMENT01111::AsCsvBool(bIcon),
            *ARMEMENT01111::AsCsvBool(bMesh),
            Spec.Item,
            *ARMEMENT01111::AsCsvBool(bValid),
            *ARMEMENT01111::AsCsvBool(bMain),
            *ARMEMENT01111::AsCsvBool(bOff),
            bGood ? TEXT("CanonicalReady") : TEXT("BLOCKED")));
    }
    TestEqual(TEXT("All four N1 melee weapons ready"), Ready, 4);
    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Output = FPaths::Combine(Folder, TEXT("ARMEMENT01_11_1_MeleeReadiness.csv"));
    TestTrue(TEXT("Canonical native audit under Saved"),
        FFileHelper::SaveStringArrayToFile(
            Report, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    TestEqual(TEXT("Real visual catalogue unchanged"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
