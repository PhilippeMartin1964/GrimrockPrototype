#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridMonsterMON178Death
{
	UGridMonsterDefinitionAsset* MakeValidDefinition()
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(GetTransientPackage());
		if (!Definition)
		{
			return nullptr;
		}

		Definition->MonsterId = TEXT("MON178_DeathContract");
		Definition->DisplayName = FText::FromString(TEXT("MON17.8 Death Contract"));
		Definition->CategoryId = TEXT("Test");
		Definition->MaxHealth = 1;
		Definition->ActionPointsPerTurn = 1;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeathDefinitionContractTest, "Grimrock.Monsters.MON17.8.DeathDefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathDefinitionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMonsterDefinitionAsset* Definition = GridMonsterMON178Death::MakeValidDefinition();
	TestNotNull(TEXT("Transient monster definition can be created"), Definition);
	if (!Definition)
	{
		return false;
	}

	TestTrue(TEXT("Death montage remains optional by default"), Definition->DeathMontage.IsNull());
	TestEqual(TEXT("Death expected duration keeps the generic one-second default"), Definition->DeathExpectedDuration, 1.0f);

	FString ValidationError;
	TestTrue(TEXT("A monster definition remains valid without a DeathMontage"), Definition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("An optional missing DeathMontage produces no validation error"), ValidationError.IsEmpty());

	Definition->DeathExpectedDuration = 0.0f;
	ValidationError.Reset();
	TestFalse(TEXT("A zero death presentation duration is rejected"), Definition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("Invalid death duration reports the DeathExpectedDuration field"), ValidationError.Contains(TEXT("DeathExpectedDuration")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeathPresentationApiContractTest, "Grimrock.Monsters.MON17.8.DeathPresentationApiContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathPresentationApiContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridMonsterDefinitionAsset::StaticClass();
	UClass* DeathComponentClass = UGridMonsterDeathComponent::StaticClass();

	TestNotNull(TEXT("Generic monster definition class exists"), DefinitionClass);
	TestNotNull(TEXT("Generic monster death component class exists"), DeathComponentClass);
	if (!DefinitionClass || !DeathComponentClass)
	{
		return false;
	}

	TestNotNull(TEXT("Monster definition exposes DeathMontage"),
		FindFProperty<FProperty>(DefinitionClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathMontage)));
	TestNotNull(TEXT("Monster definition exposes DeathExpectedDuration"),
		FindFProperty<FProperty>(DefinitionClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathExpectedDuration)));
	TestNotNull(TEXT("Death component exposes bDeathPresentationActive"),
		FindFProperty<FProperty>(DeathComponentClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, bDeathPresentationActive)));

	const FName RequiredFunctions[] = { TEXT("CommitDeath"), TEXT("StartDeathPresentation"), TEXT("NotifyDeathPresentationComplete"),
		TEXT("RestoreCommittedDeathState") };

	for (const FName FunctionName : RequiredFunctions)
	{
		TestNotNull(FString::Printf(TEXT("Death component exposes %s"), *FunctionName.ToString()), DeathComponentClass->FindFunctionByName(FunctionName));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeathDissolveDefinitionContractTest, "Grimrock.Monsters.MON17.8.DeathDissolveDefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathDissolveDefinitionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridMonsterDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Monster definition class exists"), DefinitionClass);
	if (!DefinitionClass)
	{
		return false;
	}

	const FName ForbiddenPerMonsterProperties[] = {
		TEXT("bEnableDeathDissolve"),
		TEXT("DeathDissolveDelay"),
		TEXT("DeathDissolveDuration"),
		TEXT("DeathDissolveParameterName")
	};

	for (const FName PropertyName : ForbiddenPerMonsterProperties)
	{
		TestTrue(FString::Printf(TEXT("Monster definition does not expose corpse cleanup setting %s"), *PropertyName.ToString()),
			FindFProperty<FProperty>(DefinitionClass, PropertyName) == nullptr);
	}

	UGridMonsterDefinitionAsset* Definition = GridMonsterMON178Death::MakeValidDefinition();
	TestNotNull(TEXT("A normal monster definition remains valid without corpse cleanup authoring knobs"), Definition);
	if (!Definition)
	{
		return false;
	}

	FString ValidationError;
	TestTrue(TEXT("Mandatory corpse cleanup requires no per-monster configuration"), Definition->ValidateDefinition(ValidationError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeathDissolveApiContractTest, "Grimrock.Monsters.MON17.8.DeathDissolveApiContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathDissolveApiContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DeathComponentClass = UGridMonsterDeathComponent::StaticClass();
	TestNotNull(TEXT("Death component class exists"), DeathComponentClass);
	if (!DeathComponentClass)
	{
		return false;
	}

	TestNotNull(TEXT("Death component exposes bDeathDissolveActive"),
		FindFProperty<FProperty>(DeathComponentClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, bDeathDissolveActive)));
	TestNotNull(TEXT("Death component exposes DeathDissolveAlpha"),
		FindFProperty<FProperty>(DeathComponentClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, DeathDissolveAlpha)));
	TestNotNull(TEXT("Death component exposes dissolve reset API"), DeathComponentClass->FindFunctionByName(TEXT("ResetDeathDissolvePresentation")));
	TestNotNull(TEXT("Death component exposes committed-death restore API"), DeathComponentClass->FindFunctionByName(TEXT("RestoreCommittedDeathState")));

	const UGridMonsterDeathComponent* ComponentCDO = GetDefault<UGridMonsterDeathComponent>();
	TestNotNull(TEXT("Death component CDO exists"), ComponentCDO);
	if (ComponentCDO)
	{
		TestFalse(TEXT("Mandatory corpse dissolve does not require a permanent component tick"), ComponentCDO->PrimaryComponentTick.bCanEverTick);
	}

	return true;
}

#endif
