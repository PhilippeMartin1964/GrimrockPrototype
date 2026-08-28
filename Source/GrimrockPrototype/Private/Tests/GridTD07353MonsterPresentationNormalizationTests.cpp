#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraSystem.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"

namespace GridTD07353Normalization
{
	const TCHAR* RatAssetPath =
		TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant");
	const TCHAR* GoblinAssetPath =
		TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower");

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool HasConfiguredAttackAudio(const UGridMonsterDefinitionAsset& Definition)
	{
		return Definition.Attacks.ContainsByPredicate(
			[](const FGridMonsterAttackDefinition& Attack)
			{
				return Attack.AttackAudio.HasConfiguredSound();
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07353SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_3.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07353SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* AttackStruct = FGridMonsterAttackDefinition::StaticStruct();
	TestNotNull(TEXT("Monster attack struct exists"), AttackStruct);
	if (!AttackStruct)
	{
		return false;
	}

	TestNull(TEXT("AttackSound is physically removed"),
		AttackStruct->FindPropertyByName(TEXT("AttackSound")));
	TestNull(TEXT("ImpactVFX is physically removed"),
		AttackStruct->FindPropertyByName(TEXT("ImpactVFX")));

	TestNotNull(TEXT("AttackAudio remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("AttackAudio")));
	TestNotNull(TEXT("ImpactHitAudio remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("ImpactHitAudio")));
	TestNotNull(TEXT("ImpactMissAudio remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("ImpactMissAudio")));
	TestNotNull(TEXT("AttackVFXDefinition remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("AttackVFXDefinition")));
	TestNotNull(TEXT("ImpactHitVFXDefinition remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("ImpactHitVFXDefinition")));
	TestNotNull(TEXT("ImpactMissVFXDefinition remains authoritative"),
		AttackStruct->FindPropertyByName(TEXT("ImpactMissVFXDefinition")));

	TestNotNull(TEXT("RangeCells intentionally remains until TD07.3.5.4"),
		AttackStruct->FindPropertyByName(TEXT("RangeCells")));
	TestNull(TEXT("MaxRangeCells is not introduced prematurely"),
		AttackStruct->FindPropertyByName(TEXT("MaxRangeCells")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07353RepairedAssetsTest,
	"Grimrock.TechnicalDebt.TD07_3_5_3.Normalization.RepairedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07353RepairedAssetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07353Normalization;

	UGridMonsterDefinitionAsset* Rat =
		LoadObject<UGridMonsterDefinitionAsset>(nullptr, RatAssetPath);
	UGridMonsterDefinitionAsset* Goblin =
		LoadObject<UGridMonsterDefinitionAsset>(nullptr, GoblinAssetPath);

	TestNotNull(TEXT("Repaired RatGiant definition loads"), Rat);
	TestNotNull(TEXT("Repaired GoblinThrower definition loads"), Goblin);
	if (!Rat || !Goblin)
	{
		return false;
	}

	TestEqual(TEXT("RatGiant business id remains stable"), Rat->MonsterId, FName(TEXT("RatGiant")));
	TestEqual(TEXT("GoblinThrower business id remains stable"), Goblin->MonsterId, FName(TEXT("GoblinThrower")));

	TestTrue(TEXT("RatGiant keeps configured AttackAudio"), HasConfiguredAttackAudio(*Rat));
	TestTrue(TEXT("GoblinThrower keeps configured AttackAudio"), HasConfiguredAttackAudio(*Goblin));
	TestTrue(TEXT("RatGiant remains valid"), Rat->IsValidDefinition());
	TestTrue(TEXT("GoblinThrower remains valid"), Goblin->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07353CurrentPresentationDefinitionsTest,
	"Grimrock.TechnicalDebt.TD07_3_5_3.Normalization.CurrentPresentationDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07353CurrentPresentationDefinitionsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("Attack_TD07353_Current");

	USoundWave* AttackSound = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD07353AttackSound"));
	USoundWave* HitSound = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD07353HitSound"));
	USoundWave* MissSound = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD07353MissSound"));
	UNiagaraSystem* AttackSystem = NewObject<UNiagaraSystem>(GetTransientPackage(), TEXT("TD07353AttackVFX"));
	UNiagaraSystem* HitSystem = NewObject<UNiagaraSystem>(GetTransientPackage(), TEXT("TD07353HitVFX"));
	UNiagaraSystem* MissSystem = NewObject<UNiagaraSystem>(GetTransientPackage(), TEXT("TD07353MissVFX"));

	Attack.AttackAudio.Sounds.Add(AttackSound);
	Attack.ImpactHitAudio.Sounds.Add(HitSound);
	Attack.ImpactMissAudio.Sounds.Add(MissSound);
	Attack.AttackVFXDefinition.Systems.Add(AttackSystem);
	Attack.ImpactHitVFXDefinition.Systems.Add(HitSystem);
	Attack.ImpactMissVFXDefinition.Systems.Add(MissSystem);

	TestTrue(TEXT("Current presentation-only attack definition is valid"), Attack.IsValidDefinition());
	TestTrue(TEXT("AttackAudio is configured"), Attack.AttackAudio.HasConfiguredSound());
	TestTrue(TEXT("ImpactHitAudio is configured"), Attack.ImpactHitAudio.HasConfiguredSound());
	TestTrue(TEXT("ImpactMissAudio is configured"), Attack.ImpactMissAudio.HasConfiguredSound());
	TestTrue(TEXT("AttackVFXDefinition is configured"), Attack.AttackVFXDefinition.HasConfiguredSystem());
	TestTrue(TEXT("ImpactHitVFXDefinition is configured"), Attack.ImpactHitVFXDefinition.HasConfiguredSystem());
	TestTrue(TEXT("ImpactMissVFXDefinition is configured"), Attack.ImpactMissVFXDefinition.HasConfiguredSystem());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07353RuntimeFallbackRemovalTest,
	"Grimrock.TechnicalDebt.TD07_3_5_3.Normalization.RuntimeFallbackRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07353RuntimeFallbackRemovalTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07353Normalization;

	FString TypesSource;
	FString AudioSource;
	FString VFXSource;
	TestTrue(TEXT("Monster types source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterTypes.h"), TypesSource));
	TestTrue(TEXT("Monster audio source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterAudioComponent.cpp"), AudioSource));
	TestTrue(TEXT("Monster VFX source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterVFXComponent.cpp"), VFXSource));

	TestFalse(TEXT("AttackSound declaration is gone"),
		TypesSource.Contains(TEXT("TSoftObjectPtr<USoundBase> AttackSound")));
	TestFalse(TEXT("ImpactVFX declaration is gone"),
		TypesSource.Contains(TEXT("TSoftObjectPtr<UNiagaraSystem> ImpactVFX")));
	TestFalse(TEXT("Audio runtime no longer reads AttackSound"),
		AudioSource.Contains(TEXT("Attack.AttackSound")));
	TestFalse(TEXT("Audio runtime contains no legacy adapter definition"),
		AudioSource.Contains(TEXT("LegacyDefinition")));
	TestFalse(TEXT("VFX runtime no longer reads ImpactVFX"),
		VFXSource.Contains(TEXT("Attack.ImpactVFX")));
	TestFalse(TEXT("VFX runtime contains no legacy adapter definition"),
		VFXSource.Contains(TEXT("LegacyDefinition")));

	TestTrue(TEXT("RangeCells compatibility debt remains isolated for TD07.3.5.4"),
		TypesSource.Contains(TEXT("Kept as RangeCells for serialized asset compatibility")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
