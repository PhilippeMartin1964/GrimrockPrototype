#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGSkillCatalogAuthoring.h"
#include "RPG/RPGSkillAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP014CanonicalDexterity,
    "Grimrock.TRAP01.4.CanonicalSkillDexterity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP014CanonicalDexterity::RunTest(const FString&)
{
    FRPGCanonicalSkillDefinition Definition;
    if (!TestTrue(TEXT("Editor catalog provides canonical Skill_Traps"),
        FRPGSkillCatalogAuthoring::TryGetCanonicalDefinition(TEXT("Skill_Traps"), Definition)))
    {
        return false;
    }
    TestEqual(TEXT("Traps uses Dexterity in the Editor canonical catalog"),
        Definition.GoverningAttribute, ERPGSkillGoverningAttribute::Dexterity);
    TestFalse(TEXT("Skill_Traps requires prior training"),
        Definition.bAllowUntrainedChecks);

    URPGSkillAsset* Projection = NewObject<URPGSkillAsset>(GetTransientPackage());
    Projection->SkillId = Definition.SkillId;
    Projection->GoverningAttribute = ERPGSkillGoverningAttribute::Intelligence;
    FRPGSkillCatalogAuthoring::ConfigureSkill(*Projection, Definition);
    TestEqual(TEXT("Canonical authoring repairs legacy Intelligence"),
        Projection->GoverningAttribute, ERPGSkillGoverningAttribute::Dexterity);
    TestFalse(TEXT("Canonical re-authoring preserves training requirement"),
        Projection->bAllowUntrainedChecks);
    TestTrue(TEXT("Projection now agrees with canonical identity"),
        FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Projection, Definition));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS