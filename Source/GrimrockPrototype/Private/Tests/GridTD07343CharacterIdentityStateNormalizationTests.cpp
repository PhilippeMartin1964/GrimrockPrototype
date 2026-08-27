#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterIdentityPersistence.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07343Normalization
{
	struct FRuntimeCacheGuard
	{
		FRuntimeCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}

		~FRuntimeCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}
	};

	URPGClassAsset* MakeClass(const TCHAR* ClassId, const TCHAR* DisplayName)
	{
		URPGClassAsset* Definition = NewObject<URPGClassAsset>(GetTransientPackage());
		Definition->ClassId = FName(ClassId);
		Definition->DisplayName = FText::FromString(DisplayName);
		Definition->BaseAttributes = FRPGAttributes{ 12, 11, 13, 10, 9, 8 };
		Definition->HealthAtLevelOne = 20;
		Definition->HealthPerLevel = 5;
		Definition->ManaAtLevelOne = 5;
		Definition->ManaPerLevel = 2;
		return Definition;
	}

	URPGRaceAsset* MakeRace(const TCHAR* RaceId, const TCHAR* DisplayName)
	{
		URPGRaceAsset* Definition = NewObject<URPGRaceAsset>(GetTransientPackage());
		Definition->RaceId = FName(RaceId);
		Definition->DisplayName = FText::FromString(DisplayName);
		return Definition;
	}

	FGridCharacterInventoryState MakeCharacter(
		URPGClassAsset* ClassDefinition, URPGRaceAsset* RaceDefinition, const TCHAR* DisplayName, int32 Level = 2)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(DisplayName);
		Character.ClassId = ClassDefinition->ClassId;
		Character.ClassDisplayName = FText::FromString(TEXT("CORRUPTED CLASS LABEL"));
		Character.ClassDefinition.Reset();
		Character.RaceId = RaceDefinition->RaceId;
		Character.RaceDisplayName = FText::FromString(TEXT("CORRUPTED RACE LABEL"));
		Character.Level = Level;
		Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Level);
		Character.LastAcknowledgedLevel = Level;
		Character.Attributes = ClassDefinition->BaseAttributes;
		Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Level);
		Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, ClassDefinition);
		return Character;
	}

	void Remember(URPGClassAsset* ClassDefinition, URPGRaceAsset* RaceDefinition)
	{
		FRPGAuthoringIdentityResolver::RememberClassDefinition(ClassDefinition);
		FRPGAuthoringIdentityResolver::RememberRaceDefinition(RaceDefinition);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07343SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_4_3.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07343SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	auto IsTransient = [CharacterStruct](const TCHAR* PropertyName)
	{
		const FProperty* Property = FindFProperty<FProperty>(CharacterStruct, PropertyName);
		return Property && Property->HasAnyPropertyFlags(CPF_Transient);
	};
	auto IsDurable = [CharacterStruct](const TCHAR* PropertyName)
	{
		const FProperty* Property = FindFProperty<FProperty>(CharacterStruct, PropertyName);
		return Property && !Property->HasAnyPropertyFlags(CPF_Transient);
	};

	TestTrue(TEXT("ClassId remains durable"), IsDurable(TEXT("ClassId")));
	TestTrue(TEXT("RaceId remains durable"), IsDurable(TEXT("RaceId")));
	TestTrue(TEXT("ClassDefinition becomes transient"), IsTransient(TEXT("ClassDefinition")));
	TestTrue(TEXT("ClassDisplayName becomes transient"), IsTransient(TEXT("ClassDisplayName")));
	TestTrue(TEXT("RaceDisplayName becomes transient"), IsTransient(TEXT("RaceDisplayName")));
	TestTrue(TEXT("TD07.3.4.3 established SaveGame v21 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 21);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07343ActivePoolRehydrationTest,
	"Grimrock.TechnicalDebt.TD07_3_4_3.Normalization.ActivePoolRehydration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07343ActivePoolRehydrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07343Normalization;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = MakeClass(TEXT("TD07343_Fighter"), TEXT("Fighter Canonical"));
	URPGRaceAsset* RaceDefinition = MakeRace(TEXT("TD07343_Human"), TEXT("Human Canonical"));
	Remember(ClassDefinition, RaceDefinition);

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeCharacter(ClassDefinition, RaceDefinition, TEXT("Active")));
	Party.CharacterPool.Add(MakeCharacter(ClassDefinition, RaceDefinition, TEXT("Reserve"), 3));

	FString Error;
	TestTrue(TEXT("Active + Pool identity caches rehydrate atomically"), FRPGCharacterIdentityPersistence::RehydratePartyIdentity(Party, Error));
	TestTrue(TEXT("Rehydration reports no error"), Error.IsEmpty());

	for (const FGridCharacterInventoryState* Character : { &Party.ActiveCharacters[0], &Party.CharacterPool[0] })
	{
		TestTrue(TEXT("ClassDefinition cache is canonical"),
			Character->ClassDefinition.Get() == ClassDefinition);
		TestEqual(TEXT("ClassDisplayName cache is canonical"), Character->ClassDisplayName.ToString(), FString(TEXT("Fighter Canonical")));
		TestEqual(TEXT("RaceDisplayName cache is canonical"), Character->RaceDisplayName.ToString(), FString(TEXT("Human Canonical")));
	}

	TestTrue(TEXT("Rehydrated party identity validates"), FRPGCharacterIdentityPersistence::ValidateRuntimePartyIdentity(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07343SaveRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_4_3.Normalization.SaveRoundTripRehydratesCaches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07343SaveRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07343Normalization;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = MakeClass(TEXT("TD07343_Mage"), TEXT("Mage Canonical"));
	URPGRaceAsset* RaceDefinition = MakeRace(TEXT("TD07343_Elf"), TEXT("Elf Canonical"));
	Remember(ClassDefinition, RaceDefinition);

	FGridCharacterInventoryState Character = MakeCharacter(ClassDefinition, RaceDefinition, TEXT("Aelwen"), 3);
	Character.ClassDefinition = ClassDefinition;
	Character.ClassDisplayName = FText::FromString(TEXT("STALE CLASS LABEL"));
	Character.RaceDisplayName = FText::FromString(TEXT("STALE RACE LABEL"));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState.ActiveCharacters.Add(Character);
	Save->PartyInventoryState.ActiveEquipment.SetNum(1);

	TArray<uint8> Bytes;
	TestTrue(TEXT("v21 serializes durable identity while transient caches are stale"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("v21 loads from memory"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	const FGridCharacterInventoryState& Restored = Loaded->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("ClassId survives"), Restored.ClassId, FName(TEXT("TD07343_Mage")));
	TestEqual(TEXT("RaceId survives"), Restored.RaceId, FName(TEXT("TD07343_Elf")));
	TestTrue(TEXT("ClassDefinition cache is rebuilt from ClassId"), Restored.ClassDefinition.Get() == ClassDefinition);
	TestEqual(TEXT("Class label is rebuilt from authoring source"), Restored.ClassDisplayName.ToString(), FString(TEXT("Mage Canonical")));
	TestEqual(TEXT("Race label is rebuilt from authoring source"), Restored.RaceDisplayName.ToString(), FString(TEXT("Elf Canonical")));
	TestEqual(TEXT("Transient Level is still rebuilt"), Restored.Level, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07343SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_4_3.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07343SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 20;
	FText Error;
	TestFalse(TEXT("Previous v20 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v20 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never rewrites v20"), Previous->SaveVersion, 20);
	TestTrue(TEXT("v20 rejection reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
