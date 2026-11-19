#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorArmPaletteDetachPlan.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARM065FPalette,
    "Grimrock.RPG.ARM.CLEAN06_5F.NativeIconPaletteDetachPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARM065FPalette::RunTest(const FString&)
{
    GridEditorArmPaletteDetachPlan::FPlan Plan;
    FString Error;
    if (!TestTrue(TEXT("Real WBP, palette parity and graph/tree are valid"),
        GridEditorArmPaletteDetachPlan::Build(Plan, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Exact twelve icon categories"), Plan.IconCount, 12);
    TestEqual(TEXT("CDO agrees with on-disk dependency"), Plan.bAssigned, Plan.bDiskReference);
    TestFalse(TEXT("Read-only plan does not dirty WBP"), Plan.Blueprint->GetOutermost()->IsDirty());
    TestFalse(TEXT("Designer and editable-property manifest is nonempty"), Plan.Manifest.IsEmpty());
    return !HasAnyErrors();
}
#endif
