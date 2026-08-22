#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridMonsterMON178Death
{
UGridMonsterDefinitionAsset* MakeValidDefinition ()
{
    UGridMonsterDefinitionAsset* Definition =
        NewObject<UGridMonsterDefinitionAsset> (GetTransientPackage ());
    if (!Definition)
    {
        return nullptr;
    }

    Definition->MonsterId = TEXT ("MON178_DeathContract");
    Definition->DisplayName = FText::FromString (TEXT ("MON17.8 Death Contract"));
    Definition->CategoryId = TEXT ("Test");
    Definition->MaxHealth = 1;
    Definition->ActionPointsPerTurn = 1;
    return Definition;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON178DeathDefinitionContractTest,
    "Grimrock.Monsters.MON17.8.DeathDefinitionContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathDefinitionContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON178Death::MakeValidDefinition ();
    TestNotNull (
        TEXT ("Transient monster definition can be created"),
        Definition);
    if (!Definition)
    {
        return false;
    }

    TestTrue (
        TEXT ("Death montage remains optional by default"),
        Definition->DeathMontage.IsNull ());
    TestEqual (
        TEXT ("Death expected duration keeps the generic one-second default"),
        Definition->DeathExpectedDuration,
        1.0f);

    FString ValidationError;
    TestTrue (
        TEXT ("A monster definition remains valid without a DeathMontage"),
        Definition->ValidateDefinition (ValidationError));
    TestTrue (
        TEXT ("An optional missing DeathMontage produces no validation error"),
        ValidationError.IsEmpty ());

    Definition->DeathExpectedDuration = 0.0f;
    ValidationError.Reset ();
    TestFalse (
        TEXT ("A zero death presentation duration is rejected"),
        Definition->ValidateDefinition (ValidationError));
    TestTrue (
        TEXT ("Invalid death duration reports the DeathExpectedDuration field"),
        ValidationError.Contains (TEXT ("DeathExpectedDuration")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON178DeathPresentationApiContractTest,
    "Grimrock.Monsters.MON17.8.DeathPresentationApiContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathPresentationApiContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UClass* DefinitionClass = UGridMonsterDefinitionAsset::StaticClass ();
    UClass* DeathComponentClass = UGridMonsterDeathComponent::StaticClass ();

    TestNotNull (
        TEXT ("Generic monster definition class exists"),
        DefinitionClass);
    TestNotNull (
        TEXT ("Generic monster death component class exists"),
        DeathComponentClass);
    if (!DefinitionClass || !DeathComponentClass)
    {
        return false;
    }

    TestNotNull (
        TEXT ("Monster definition exposes DeathMontage"),
        FindFProperty<FProperty> (
            DefinitionClass,
            GET_MEMBER_NAME_CHECKED (
                UGridMonsterDefinitionAsset,
                DeathMontage)));
    TestNotNull (
        TEXT ("Monster definition exposes DeathExpectedDuration"),
        FindFProperty<FProperty> (
            DefinitionClass,
            GET_MEMBER_NAME_CHECKED (
                UGridMonsterDefinitionAsset,
                DeathExpectedDuration)));
    TestNotNull (
        TEXT ("Death component exposes bDeathPresentationActive"),
        FindFProperty<FProperty> (
            DeathComponentClass,
            GET_MEMBER_NAME_CHECKED (
                UGridMonsterDeathComponent,
                bDeathPresentationActive)));

    const FName RequiredFunctions[] =
    {
        TEXT ("CommitDeath"),
        TEXT ("StartDeathPresentation"),
        TEXT ("NotifyDeathPresentationComplete"),
        TEXT ("RestoreCommittedDeathState")
    };

    for (const FName FunctionName : RequiredFunctions)
    {
        TestNotNull (
            FString::Printf (
                TEXT ("Death component exposes %s"),
                *FunctionName.ToString ()),
            DeathComponentClass->FindFunctionByName (FunctionName));
    }

    return true;
}

#endif
