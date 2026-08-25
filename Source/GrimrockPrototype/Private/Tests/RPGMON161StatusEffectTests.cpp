#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "UObject/UnrealType.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON161MakeDefinition(UObject* Outer, FName EffectId,
		EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds, int32 DefaultDuration = 3,
		EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DefaultDuration;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		return Definition;
	}

	struct FGridMON161TestWorld
	{
		UWorld* World = nullptr;

		FGridMON161TestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MON161TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON161TestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161DefinitionValidationTest, "Grimrock.RPG.MON16.1.DefinitionValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161DefinitionValidationTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* ValidDefinition =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::AddStacks, 4);

	FString Error;
	TestTrue(TEXT("A coherent definition is accepted"), ValidDefinition->ValidateDefinition(Error));
	TestTrue(TEXT("A coherent definition reports no error"), Error.IsEmpty());

	UGridStatusEffectDefinitionAsset* MissingId = MON161MakeDefinition(GetTransientPackage(), NAME_None);
	TestFalse(TEXT("EffectId None is rejected"), MissingId->ValidateDefinition(Error));
	TestTrue(TEXT("Invalid EffectId reports its cause"), Error.Contains(TEXT("EffectId")));

	UGridStatusEffectDefinitionAsset* InvalidTimedDuration =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Bleeding"), EGridStatusEffectDurationUnit::Turns, 0);
	TestFalse(TEXT("A timed effect with zero duration is rejected"), InvalidTimedDuration->ValidateDefinition(Error));

	UGridStatusEffectDefinitionAsset* InvalidPermanentDuration =
		MON161MakeDefinition(GetTransientPackage(), TEXT("PermanentBlessing"), EGridStatusEffectDurationUnit::Permanent, 2);
	TestFalse(TEXT("A permanent effect with a non-zero duration is rejected"), InvalidPermanentDuration->ValidateDefinition(Error));

	UGridStatusEffectDefinitionAsset* InvalidStacking =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Burning"), EGridStatusEffectDurationUnit::Rounds, 2, EGridStatusEffectStackPolicy::AddStacks, 1);
	TestFalse(TEXT("AddStacks with MaxStacks one is rejected"), InvalidStacking->ValidateDefinition(Error));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161StableIdentityTest, "Grimrock.RPG.MON16.1.StableIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161StableIdentityTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* First = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage(), TEXT("MON161_FirstAssetName"));
	First->EffectId = TEXT("Haste");
	First->DisplayName = FText::FromString(TEXT("Haste"));

	UGridStatusEffectDefinitionAsset* Second = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage(), TEXT("MON161_SecondAssetName"));
	Second->EffectId = TEXT("Haste");
	Second->DisplayName = FText::FromString(TEXT("Haste"));

	const FPrimaryAssetId FirstId = First->GetPrimaryAssetId();
	const FPrimaryAssetId SecondId = Second->GetPrimaryAssetId();

	TestTrue(TEXT("Primary identity is independent from the UObject name"), FirstId == SecondId);
	TestTrue(TEXT("Primary asset type is GridStatusEffect"), FirstId.PrimaryAssetType == FPrimaryAssetType(TEXT("GridStatusEffect")));
	TestTrue(TEXT("Primary asset name is the stable EffectId"), FirstId.PrimaryAssetName == FName(TEXT("Haste")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161RuntimeStateCreationTest, "Grimrock.RPG.MON16.1.RuntimeStateCreation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161RuntimeStateCreationTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Definition =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 5, EGridStatusEffectStackPolicy::AddStacks, 4);

	const FGuid SourceId = FGuid::NewGuid();
	FGridStatusEffectRuntimeState State;
	FString Error;
	TestTrue(TEXT("A valid runtime state can be created"), Definition->BuildRuntimeState(SourceId, 2, 3, State, Error));
	TestTrue(TEXT("Created runtime state validates"), State.IsValid());
	TestTrue(TEXT("Runtime EffectId comes from the definition"), State.EffectId == FName(TEXT("Poison")));
	TestTrue(TEXT("Runtime source keeps the stable source identity"), State.SourceId == SourceId);
	TestEqual(TEXT("Runtime stack count is preserved"), State.StackCount, 2);
	TestEqual(TEXT("Runtime duration override is preserved"), State.RemainingDuration, 3);
	TestTrue(TEXT("Runtime duration unit comes from the definition"), State.DurationUnit == EGridStatusEffectDurationUnit::Rounds);

	FGridStatusEffectRuntimeState Sentinel;
	Sentinel.EffectId = TEXT("Sentinel");
	Sentinel.StackCount = 1;
	Sentinel.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	Sentinel.RemainingDuration = 9;

	TestFalse(TEXT("An invalid stack count is rejected"), Definition->BuildRuntimeState(SourceId, 5, 3, Sentinel, Error));
	TestTrue(TEXT("Failed state creation is atomic"), Sentinel.EffectId == FName(TEXT("Sentinel")) && Sentinel.RemainingDuration == 9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161TargetIsolationTest, "Grimrock.RPG.MON16.1.TargetIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161TargetIsolationTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::AddStacks, 4);
	UGridStatusEffectDefinitionAsset* Burning = MON161MakeDefinition(GetTransientPackage(), TEXT("Burning"), EGridStatusEffectDurationUnit::Turns, 2);

	FGridCharacterInventoryState Elias;
	Elias.CharacterId = FGuid::NewGuid();
	FGridCharacterInventoryState SecondCharacter;
	SecondCharacter.CharacterId = FGuid::NewGuid();

	FGridMON161TestWorld TestWorld;
	TestNotNull(TEXT("Transient test world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
	TestNotNull(TEXT("Monster target is created"), Monster);
	if (!Monster)
	{
		return false;
	}

	FString Error;
	TestTrue(TEXT("Effect can be added to a character target"), Elias.StatusEffects.TryAdd(*Poison, FGuid::NewGuid(), 2, 3, Error));
	TestTrue(TEXT("A separate effect can be added to a monster target"), Monster->StatusEffects.TryAdd(*Burning, FGuid::NewGuid(), Error));

	TestEqual(TEXT("First character owns one effect"), Elias.StatusEffects.Num(), 1);
	TestTrue(TEXT("Second character is not contaminated"), SecondCharacter.StatusEffects.IsEmpty());
	TestEqual(TEXT("Monster owns one independent effect"), Monster->StatusEffects.Num(), 1);
	TestTrue(TEXT("Character state is not contaminated by monster effect"),
		Elias.StatusEffects.Contains(TEXT("Poison")) && !Elias.StatusEffects.Contains(TEXT("Burning")));
	TestTrue(TEXT("Monster state is not contaminated by character effect"),
		Monster->StatusEffects.Contains(TEXT("Burning")) && !Monster->StatusEffects.Contains(TEXT("Poison")));

	const FStructProperty* CharacterProperty =
		FindFProperty<FStructProperty>(FGridCharacterInventoryState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridCharacterInventoryState, StatusEffects));
	const FStructProperty* MonsterProperty =
		FindFProperty<FStructProperty>(AGridMonsterActor::StaticClass(), GET_MEMBER_NAME_CHECKED(AGridMonsterActor, StatusEffects));

	TestTrue(TEXT("Character and monster expose the same collection structure"),
		CharacterProperty && MonsterProperty && CharacterProperty->Struct == FGridStatusEffectCollection::StaticStruct() &&
			MonsterProperty->Struct == FGridStatusEffectCollection::StaticStruct());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON161DeterministicCollectionTest, "Grimrock.RPG.MON16.1.DeterministicCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161DeterministicCollectionTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison = MON161MakeDefinition(GetTransientPackage(), TEXT("Poison"));
	UGridStatusEffectDefinitionAsset* Haste = MON161MakeDefinition(GetTransientPackage(), TEXT("Haste"));
	UGridStatusEffectDefinitionAsset* Burning = MON161MakeDefinition(GetTransientPackage(), TEXT("Burning"));

	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Poison is added"), Collection.TryAdd(*Poison, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Haste is added"), Collection.TryAdd(*Haste, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Burning is added"), Collection.TryAdd(*Burning, FGuid::NewGuid(), Error));

	TestEqual(TEXT("Three effects are active"), Collection.Num(), 3);
	TestTrue(TEXT("Read order starts with Burning"), Collection.ActiveEffects[0].EffectId == FName(TEXT("Burning")));
	TestTrue(TEXT("Read order continues with Haste"), Collection.ActiveEffects[1].EffectId == FName(TEXT("Haste")));
	TestTrue(TEXT("Read order ends with Poison"), Collection.ActiveEffects[2].EffectId == FName(TEXT("Poison")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161AtomicInvalidAddTest, "Grimrock.RPG.MON16.1.AtomicInvalidAdd", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161AtomicInvalidAddTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison =
		MON161MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::AddStacks, 3);

	FGridStatusEffectCollection Collection;
	FString Error;
	const FGuid OriginalSource = FGuid::NewGuid();
	TestTrue(TEXT("Initial valid effect is added"), Collection.TryAdd(*Poison, OriginalSource, 1, 3, Error));

	UGridStatusEffectDefinitionAsset* InvalidDefinition = MON161MakeDefinition(GetTransientPackage(), NAME_None);
	TestFalse(TEXT("Invalid definition is rejected atomically"), Collection.TryAdd(*InvalidDefinition, FGuid::NewGuid(), Error));
	TestEqual(TEXT("Invalid definition does not change collection size"), Collection.Num(), 1);

	TestFalse(TEXT("Invalid initial stack count is rejected atomically"), Collection.TryAdd(*Poison, FGuid::NewGuid(), 4, 3, Error));
	TestEqual(TEXT("Invalid stack count does not change collection size"), Collection.Num(), 1);

	TestFalse(TEXT("Duplicate EffectId is rejected until MON16.2 stacking"), Collection.TryAdd(*Poison, FGuid::NewGuid(), 1, 3, Error));
	TestEqual(TEXT("Duplicate add does not change collection size"), Collection.Num(), 1);

	const FGridStatusEffectRuntimeState* Remaining = Collection.FindByEffectId(TEXT("Poison"));
	TestTrue(TEXT("Original state remains unchanged after failed additions"),
		Remaining && Remaining->SourceId == OriginalSource && Remaining->StackCount == 1 && Remaining->RemainingDuration == 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON161NoUIDependencyTest, "Grimrock.RPG.MON16.1.NoUIDependency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON161NoUIDependencyTest::RunTest(const FString& Parameters)
{
	const TArray<FString> RelativePaths = { TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h"),
		TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectTypes.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp") };

	for (const FString& RelativePath : RelativePaths)
	{
		FString SourceText;
		const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		const bool bLoaded = FFileHelper::LoadFileToString(SourceText, *FullPath);
		TestTrue(*FString::Printf(TEXT("Source file is readable: %s"), *RelativePath), bLoaded);
		if (!bLoaded)
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("No UI include in %s"), *RelativePath), SourceText.Contains(TEXT("#include \"UI/")));
		TestFalse(*FString::Printf(TEXT("No UMG dependency in %s"), *RelativePath), SourceText.Contains(TEXT("UMG")));
		TestFalse(*FString::Printf(TEXT("No widget dependency in %s"), *RelativePath), SourceText.Contains(TEXT("UUserWidget")));
	}

	return true;
}

#endif
