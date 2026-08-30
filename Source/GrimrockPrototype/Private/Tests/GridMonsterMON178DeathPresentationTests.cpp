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

	UGridMonsterDefinitionAsset* Definition = GridMonsterMON178Death::MakeValidDefinition();
	TestNotNull(TEXT("Transient dissolve definition can be created"), Definition);
	if (!Definition)
	{
		return false;
	}

	TestTrue(TEXT("Death dissolve is enabled by default for every monster"), Definition->bEnableDeathDissolve);
	TestEqual(TEXT("Default corpse hold before dissolve is two seconds"), Definition->DeathDissolveDelay, 2.0f);
	TestEqual(TEXT("Default dissolve duration is one and a half seconds"), Definition->DeathDissolveDuration, 1.5f);
	TestEqual(TEXT("Default dissolve parameter is DissolveAmount"), Definition->DeathDissolveParameterName, FName(TEXT("DissolveAmount")));

	FString ValidationError;
	TestTrue(TEXT("Default mandatory dissolve settings are valid"), Definition->ValidateDefinition(ValidationError));

	// The former serialized flag is ignored by the runtime and retained only
	// for backward asset compatibility. A valid material dissolve contract is
	// required regardless of that legacy value.
	Definition->bEnableDeathDissolve = false;
	Definition->DeathDissolveParameterName = NAME_None;
	ValidationError.Reset();
	TestFalse(TEXT("Mandatory dissolve requires a material parameter name even if a legacy asset stored false"), Definition->ValidateDefinition(ValidationError));
	TestTrue(TEXT("Missing parameter reports DeathDissolveParameterName"), ValidationError.Contains(TEXT("DeathDissolveParameterName")));

	Definition->DeathDissolveParameterName = TEXT("DissolveAmount");
	ValidationError.Reset();
	TestTrue(TEXT("Legacy false cannot invalidate an otherwise valid mandatory dissolve contract"), Definition->ValidateDefinition(ValidationError));

	Definition->DeathDissolveParameterName = TEXT("DissolveAmount");
	Definition->DeathDissolveDelay = -0.1f;
	ValidationError.Reset();
	TestFalse(TEXT("Negative corpse hold is rejected"), Definition->ValidateDefinition(ValidationError));

	Definition->DeathDissolveDelay = 0.0f;
	Definition->DeathDissolveDuration = 0.0f;
	ValidationError.Reset();
	TestFalse(TEXT("Zero dissolve duration is rejected"), Definition->ValidateDefinition(ValidationError));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON178DeathDissolveApiContractTest, "Grimrock.Monsters.MON17.8.DeathDissolveApiContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON178DeathDissolveApiContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DefinitionClass = UGridMonsterDefinitionAsset::StaticClass();
	UClass* DeathComponentClass = UGridMonsterDeathComponent::StaticClass();
	TestNotNull(TEXT("Definition class exists"), DefinitionClass);
	TestNotNull(TEXT("Death component class exists"), DeathComponentClass);
	if (!DefinitionClass || !DeathComponentClass)
	{
		return false;
	}

	const FName DefinitionProperties[] = { GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, bEnableDeathDissolve),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathDissolveDelay), GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathDissolveDuration),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathDissolveParameterName) };

	for (const FName PropertyName : DefinitionProperties)
	{
		TestNotNull(FString::Printf(TEXT("Definition exposes %s"), *PropertyName.ToString()), FindFProperty<FProperty>(DefinitionClass, PropertyName));
	}

	TestNotNull(TEXT("Death component exposes bDeathDissolveActive"),
		FindFProperty<FProperty>(DeathComponentClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, bDeathDissolveActive)));
	TestNotNull(TEXT("Death component exposes DeathDissolveAlpha"),
		FindFProperty<FProperty>(DeathComponentClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, DeathDissolveAlpha)));
	TestNotNull(TEXT("Death component exposes dissolve reset API"), DeathComponentClass->FindFunctionByName(TEXT("ResetDeathDissolvePresentation")));

	const UGridMonsterDeathComponent* ComponentCDO = GetDefault<UGridMonsterDeathComponent>();
	TestNotNull(TEXT("Death component CDO exists"), ComponentCDO);
	if (ComponentCDO)
	{
		TestFalse(TEXT("Death dissolve does not require a permanent component tick"), ComponentCDO->PrimaryComponentTick.bCanEverTick);
	}

	return true;
}

#endif
