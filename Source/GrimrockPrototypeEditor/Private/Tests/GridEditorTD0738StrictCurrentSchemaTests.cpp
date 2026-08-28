#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/DataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD0738StrictSchema
{
	struct FCandidate
	{
		FString Code;
		FString AssetPath;
		FString Context;
	};

	void AddCandidate(TArray<FCandidate>& Candidates, const TCHAR* Code, const FString& AssetPath, const FString& Context)
	{
		FCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Code = Code;
		Candidate.AssetPath = AssetPath;
		Candidate.Context = Context;
	}

	void AuditPair(const FString& AssetPath, const FString& Context, const UObject* DefinitionObject, FName CanonicalId, FName StoredId,
		TArray<FCandidate>& Candidates)
	{
		if (DefinitionObject && CanonicalId.IsNone())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.DEFINITION_WITHOUT_ID"), AssetPath, Context);
		}

		if (StoredId.IsNone())
		{
			return;
		}

		if (!DefinitionObject)
		{
			AddCandidate(Candidates, TEXT("AUTHORING.ID_ONLY"), AssetPath, Context);
			return;
		}

		AddCandidate(
			Candidates,
			CanonicalId == StoredId ? TEXT("AUTHORING.ASSET_ID_DUPLICATE") : TEXT("AUTHORING.ASSET_ID_CONFLICT"),
			AssetPath,
			Context);
	}

	void AuditBehavior(const FString& AssetPath, const FString& Context, const FGridObjectBehaviorParams& Behavior, TArray<FCandidate>& Candidates)
	{
		const UGridItemDefinitionAsset* Item = Behavior.Item.ItemDefinitionAsset.Get();
		AuditPair(AssetPath, Context + TEXT(".Item"), Item, Item ? Item->ItemDefinitionId : NAME_None, Behavior.Item.ItemDefinitionId, Candidates);

		const UGridReadableContentAsset* Readable = Behavior.Item.DefaultReadableContentAsset.Get();
		AuditPair(
			AssetPath,
			Context + TEXT(".DefaultReadableContent"),
			Readable,
			Readable ? Readable->ReadableContentId : NAME_None,
			Behavior.Item.DefaultReadableContentId,
			Candidates);

		if (!Behavior.Lock.AcceptedKeyIds.IsEmpty())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.LOCK_KEY_IDS"), AssetPath, Context + TEXT(".Lock"));
		}
	}

	void AuditDataAsset(const UDataAsset& DataAsset, TArray<FCandidate>& Candidates)
	{
		const FString AssetPath = DataAsset.GetPathName();

		if (const UGridLevelAsset* Level = Cast<UGridLevelAsset>(&DataAsset))
		{
			for (int32 Index = 0; Index < Level->Objects.Num(); ++Index)
			{
				const FGridLevelObjectData& Object = Level->Objects[Index];
				const FString Context = FString::Printf(TEXT("Objects[%d]"), Index);

				const UGridItemDefinitionAsset* Item = Object.ItemDefinitionAsset.Get();
				AuditPair(AssetPath, Context + TEXT(".ItemDefinition"), Item, Item ? Item->ItemDefinitionId : NAME_None, Object.ItemDefinitionId, Candidates);

				const UGridReadableContentAsset* Readable = Object.ReadableContentAsset.Get();
				AuditPair(
					AssetPath,
					Context + TEXT(".ReadableContent"),
					Readable,
					Readable ? Readable->ReadableContentId : NAME_None,
					Object.ReadableContentId,
					Candidates);

				const UGridMonsterDefinitionAsset* Monster = Object.MonsterDefinitionAsset.Get();
				AuditPair(
					AssetPath,
					Context + TEXT(".MonsterDefinition"),
					Monster,
					Monster ? Monster->MonsterId : NAME_None,
					Object.MonsterDefinitionId,
					Candidates);

				AuditBehavior(AssetPath, Context + TEXT(".Behavior"), Object.Behavior, Candidates);
			}
			return;
		}

		if (const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(&DataAsset))
		{
			AuditBehavior(AssetPath, TEXT("DefaultBehavior"), Archetype->DefaultBehavior, Candidates);
			return;
		}

		if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(&DataAsset))
		{
			if (!Item->HasValidCombatActions())
			{
				AddCandidate(Candidates, TEXT("ITEM.INVALID_COMBAT_ACTIONS"), AssetPath, TEXT("CombatActions"));
			}
			return;
		}

		if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			for (int32 LootIndex = 0; LootIndex < Monster->LootTable.Num(); ++LootIndex)
			{
				const FGridMonsterLootEntry& Loot = Monster->LootTable[LootIndex];
				const UGridItemDefinitionAsset* Item = Loot.ItemDefinitionAsset.Get();
				AuditPair(
					AssetPath,
					FString::Printf(TEXT("LootTable[%d]"), LootIndex),
					Item,
					Item ? Item->ItemDefinitionId : NAME_None,
					Loot.ItemDefinitionId,
					Candidates);
			}
		}
	}

	bool IsTransientProperty(const UStruct* Struct, const TCHAR* Name)
	{
		const FProperty* Property = FindFProperty<FProperty>(Struct, Name);
		return Property && Property->HasAnyPropertyFlags(CPF_Transient);
	}

	bool IsDurableProperty(const UStruct* Struct, const TCHAR* Name)
	{
		const FProperty* Property = FindFProperty<FProperty>(Struct, Name);
		return Property && !Property->HasAnyPropertyFlags(CPF_Transient);
	}

	bool IsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East ||
			Facing == EGridEdge::South || Facing == EGridEdge::West;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0738SaveExactMatchTest,
	"Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.SaveExactMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0738SaveExactMatchTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Current prototype save generation is at or beyond TD07.3 v22"),
		UGrimrockPartySaveGame::CurrentSaveVersion >= 22);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("Fresh SaveGame uses CurrentSaveVersion"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Fresh current-schema SaveGame is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	FText PreviousError;
	TestFalse(TEXT("Previous prototype schema is rejected"), Previous->ValidateCurrentState(PreviousError));
	TestEqual(TEXT("Previous version is never rewritten"), Previous->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion - 1);

	UGrimrockPartySaveGame* Future = NewObject<UGrimrockPartySaveGame>();
	Future->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion + 1;
	FText FutureError;
	TestFalse(TEXT("Future schema is rejected"), Future->ValidateCurrentState(FutureError));
	TestEqual(TEXT("Future version is never rewritten"), Future->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion + 1);

	FString SaveSource;
	const FString SaveSourcePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/Save/GrimrockPartySaveGame.cpp"));
	TestTrue(TEXT("Save source loads"), FFileHelper::LoadFileToString(SaveSource, *SaveSourcePath));
	TestTrue(TEXT("Save validation remains exact-match"), SaveSource.Contains(TEXT("SaveVersion != CurrentSaveVersion")));
	TestFalse(TEXT("Save source contains no historical migration helper"), SaveSource.Contains(TEXT("MigrateSave")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0738CharacterAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.CharacterAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0738CharacterAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0738StrictSchema;

	const UScriptStruct* Character = FGridCharacterInventoryState::StaticStruct();
	TestNotNull(TEXT("Character state struct exists"), Character);
	if (!Character)
	{
		return false;
	}

	for (const TCHAR* Name : {
			 TEXT("ClassId"), TEXT("RaceId"), TEXT("Experience"), TEXT("LastAcknowledgedLevel"),
			 TEXT("SelectedClassProgressionChoiceIds"), TEXT("Attributes"), TEXT("Resources"),
			 TEXT("SkillRanks"), TEXT("KnownSpellIds"), TEXT("StatusEffects"),
			 TEXT("PortraitGender"), TEXT("PortraitVariantId") })
	{
		TestTrue(*FString::Printf(TEXT("%s remains durable authority"), Name), IsDurableProperty(Character, Name));
	}

	for (const TCHAR* Name : {
			 TEXT("ClassDisplayName"), TEXT("ClassDefinition"), TEXT("RaceDisplayName"),
			 TEXT("Level"), TEXT("DerivedStats"), TEXT("Portrait"), TEXT("ClassIcon") })
	{
		TestTrue(*FString::Printf(TEXT("%s remains transient projection"), Name), IsTransientProperty(Character, Name));
	}

	TestNull(TEXT("Legacy CharacterSkillStates mirror is absent"), Character->FindPropertyByName(TEXT("CharacterSkillStates")));
	TestNull(TEXT("Legacy CharacterSpellbookStates mirror is absent"), Character->FindPropertyByName(TEXT("CharacterSpellbookStates")));
	TestNull(TEXT("Legacy ClassProgressionStates mirror is absent"), Character->FindPropertyByName(TEXT("ClassProgressionStates")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0738LegacySymbolsAbsentTest,
	"Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.LegacySymbolsAbsent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0738LegacySymbolsAbsentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	TestNull(TEXT("bPlaceOnEdge is absent"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceOnEdge")));
	TestNull(TEXT("bPlaceAtCellCenter is absent"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceAtCellCenter")));

	UClass* ItemClass = UGridItemDefinitionAsset::StaticClass();
	TestNull(TEXT("Legacy item bProvidesAttack is absent"), ItemClass->FindPropertyByName(TEXT("bProvidesAttack")));
	TestNull(TEXT("Legacy item-level OffensiveProfile is absent"), ItemClass->FindPropertyByName(TEXT("OffensiveProfile")));

	TestNull(TEXT("Deprecated combat phase query is absent"),
		UGridTurnManagerComponent::StaticClass()->FindFunctionByName(TEXT("HasCharacterCommittedAttackThisPhase")));

	UClass* PawnClass = AGrimrockPartyPawn::StaticClass();
	TestNull(TEXT("Legacy keyboard Use flag is absent"), PawnClass->FindPropertyByName(TEXT("bEnableLegacyKeyboardUseAction")));
	TestNull(TEXT("Legacy keyboard UseAction is absent"), PawnClass->FindPropertyByName(TEXT("UseAction")));

	const UEnum* RebuildEnum = StaticEnum<EGridRuntimeRebuildMode>();
	TestNotNull(TEXT("Rebuild enum exists"), RebuildEnum);
	if (RebuildEnum)
	{
		TestTrue(TEXT("ObjectsOnly is absent"), RebuildEnum->GetValueByNameString(TEXT("ObjectsOnly")) == INDEX_NONE);
	}

	UScriptStruct* MonsterAttack = FGridMonsterAttackDefinition::StaticStruct();
	TestNotNull(TEXT("Monster attack struct exists"), MonsterAttack);
	if (MonsterAttack)
	{
		TestNull(TEXT("AttackSound is absent"), MonsterAttack->FindPropertyByName(TEXT("AttackSound")));
		TestNull(TEXT("ImpactVFX is absent"), MonsterAttack->FindPropertyByName(TEXT("ImpactVFX")));
		TestNull(TEXT("Legacy RangeCells is absent"), MonsterAttack->FindPropertyByName(TEXT("RangeCells")));
		TestNotNull(TEXT("MinRangeCells is present"), MonsterAttack->FindPropertyByName(TEXT("MinRangeCells")));
		TestNotNull(TEXT("MaxRangeCells is present"), MonsterAttack->FindPropertyByName(TEXT("MaxRangeCells")));
	}

	TestFalse(TEXT("TD07.3.6 one-shot script is absent"),
		FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/RepairTD0736MonsterSpawnFacing.ps1"))));
	TestFalse(TEXT("TD07.3.7 one-shot script is absent"),
		FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/RepairTD0737CurrentAssets.ps1"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0738CurrentAssetsStrictTest,
	"Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.CurrentAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0738CurrentAssetsStrictTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0738StrictSchema;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	int32 LoadedAssets = 0;
	TArray<FCandidate> Candidates;
	for (const FAssetData& Entry : Assets)
	{
		UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
		TestNotNull(*FString::Printf(TEXT("%s loads"), *Entry.PackageName.ToString()), DataAsset);
		if (!DataAsset)
		{
			continue;
		}

		++LoadedAssets;
		AuditDataAsset(*DataAsset, Candidates);
	}

	for (const FCandidate& Candidate : Candidates)
	{
		AddError(FString::Printf(TEXT("%s | %s | %s"), *Candidate.Code, *Candidate.AssetPath, *Candidate.Context));
	}

	TestEqual(TEXT("Every discovered DataAsset loads"), LoadedAssets, Assets.Num());
	TestEqual(TEXT("Strict current-schema authoring has zero repair candidates"), Candidates.Num(), 0);
	AddInfo(FString::Printf(TEXT("TD07.3.8 strict schema scanned %d DataAssets."), LoadedAssets));
	return Candidates.IsEmpty() && LoadedAssets == Assets.Num();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0738MonsterAssetsStrictTest,
	"Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.MonsterAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0738MonsterAssetsStrictTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0738StrictSchema;

	UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
	UGridMonsterDefinitionAsset* Goblin = LoadObject<UGridMonsterDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"));

	if (!TestNotNull(TEXT("Production GridLevel loads"), Level) ||
		!TestNotNull(TEXT("RatGiant definition loads"), Rat) ||
		!TestNotNull(TEXT("GoblinThrower definition loads"), Goblin))
	{
		return false;
	}

	TestTrue(TEXT("RatGiant definition is valid"), Rat->IsValidDefinition());
	TestTrue(TEXT("GoblinThrower definition is valid"), Goblin->IsValidDefinition());

	int32 MonsterSpawnCount = 0;
	for (const FGridLevelObjectData& Object : Level->Objects)
	{
		if (Object.Type != EGridLevelObjectType::MonsterSpawn)
		{
			continue;
		}

		++MonsterSpawnCount;
		TestNotNull(*FString::Printf(TEXT("MonsterSpawn %s has definition asset"), *Object.ObjectId.ToString()), Object.MonsterDefinitionAsset.Get());
		TestTrue(*FString::Printf(TEXT("MonsterSpawn %s has no mirrored definition id"), *Object.ObjectId.ToString()), Object.MonsterDefinitionId.IsNone());
		TestTrue(*FString::Printf(TEXT("MonsterSpawn %s has cardinal facing"), *Object.ObjectId.ToString()), IsCardinalFacing(Object.InitialFacing));
	}
	TestTrue(TEXT("Production level covers at least one MonsterSpawn"), MonsterSpawnCount > 0);

	for (const UGridMonsterDefinitionAsset* Monster : { Rat, Goblin })
	{
		for (const FGridMonsterAttackDefinition& Attack : Monster->Attacks)
		{
			TestTrue(*FString::Printf(TEXT("%s/%s MinRangeCells > 0"), *Monster->MonsterId.ToString(), *Attack.AttackId.ToString()), Attack.MinRangeCells > 0);
			TestTrue(*FString::Printf(TEXT("%s/%s MaxRangeCells >= MinRangeCells"), *Monster->MonsterId.ToString(), *Attack.AttackId.ToString()),
				Attack.MaxRangeCells >= Attack.MinRangeCells);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
