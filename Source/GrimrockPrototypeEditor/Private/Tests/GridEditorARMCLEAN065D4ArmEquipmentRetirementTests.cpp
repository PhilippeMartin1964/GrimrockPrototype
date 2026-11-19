#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065D4ArmExtraStats,
    "Grimrock.RPG.ARM.CLEAN06_5D4.ArmExtraContributionsRetirementEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065D4ArmExtraStats::RunTest(const FString&)
{
    // Read-only retirement gate: the approved schema must not lose an authored
    // bonus or a resistance. Never erase a non-zero field to make this pass.
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Read exact production manifest"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
    { AddError(Error); return false; }
    if (!TestEqual(TEXT("22 ARM sources"), Rows.Num(), 22)) return false;
    const FGridEquipmentStatBonus NoExtra;
    const FGridDamageResistanceSet NoResistance;
    TSet<FName> Ids;
    for (const auto& Row : Rows)
    {
        const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
        if (!TestNotNull(TEXT("Exact real ARM source"), Old)) return false;
        const FName Id(*Row.ItemDefinitionId);
        if (!TestTrue(TEXT("Source identity and path exact"), Old->ItemDefinitionId == Id &&
            Old->GetPathName() == Row.ItemPath && !Ids.Contains(Id))) return false;
        Ids.Add(Id);
        const bool bWasDirty = Old->GetOutermost()->IsDirty();
        FGridEquipmentStatBonus Extras = Old->EquipmentStatBonus;
        // N1 copies of these two fields already have authoritative growth in
        // the native weapon catalogue. All other fields must be exactly empty.
        Extras.ArmorBonus = 0;
        Extras.MagicalArmorBonus = 0;
        TestTrue(FString(TEXT("ARM historical extra bonuses empty: ")) + Row.ArchetypeId,
            FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&Extras, &NoExtra, 0));
        TestTrue(FString(TEXT("ARM historical resistances empty: ")) + Row.ArchetypeId,
            FGridDamageResistanceSet::StaticStruct()->CompareScriptStruct(
                &Old->EquipmentResistanceBonus, &NoResistance, 0));
        TestTrue(TEXT("No historical package mutation"), bWasDirty == Old->GetOutermost()->IsDirty());
    }
    TestEqual(TEXT("All distinct ARM identities inspected"), Ids.Num(), 22);
    return !HasAnyErrors();
}
#endif
