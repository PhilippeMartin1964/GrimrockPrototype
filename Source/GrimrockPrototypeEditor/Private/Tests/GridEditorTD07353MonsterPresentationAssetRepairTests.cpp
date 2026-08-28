#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/PackageName.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridTD07353MonsterPresentationRepair
{
	struct FTarget
	{
		const TCHAR* Label;
		const TCHAR* AssetPath;
	};

	const FTarget Targets[] = {
		{
			TEXT("RatGiant"),
			TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant")
		},
		{
			TEXT("GoblinThrower"),
			TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower")
		}
	};

	bool SaveAssetPackage(UGridMonsterDefinitionAsset* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}

	bool ContainsSound(const FGridMonsterAudioEventDefinition& Definition, const TSoftObjectPtr<USoundBase>& Sound)
	{
		const FSoftObjectPath Wanted = Sound.ToSoftObjectPath();
		return Definition.Sounds.ContainsByPredicate(
			[&Wanted](const TSoftObjectPtr<USoundBase>& Candidate)
			{
				return Candidate.ToSoftObjectPath() == Wanted;
			});
	}

	bool ContainsSystem(const FGridMonsterVFXEventDefinition& Definition, const TSoftObjectPtr<UNiagaraSystem>& System)
	{
		const FSoftObjectPath Wanted = System.ToSoftObjectPath();
		return Definition.Systems.ContainsByPredicate(
			[&Wanted](const TSoftObjectPtr<UNiagaraSystem>& Candidate)
			{
				return Candidate.ToSoftObjectPath() == Wanted;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07353MonsterPresentationAssetRepairTest,
	"Grimrock.TechnicalDebt.TD07_3_5_3.AssetRepair.MonsterPresentationAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07353MonsterPresentationAssetRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07353MonsterPresentationRepair;

	int32 LoadedCount = 0;
	int32 ChangedAssetCount = 0;
	for (const FTarget& Target : Targets)
	{
		UGridMonsterDefinitionAsset* Definition =
			LoadObject<UGridMonsterDefinitionAsset>(nullptr, Target.AssetPath);
		TestNotNull(*FString::Printf(TEXT("%s definition loads"), Target.Label), Definition);
		if (!Definition)
		{
			continue;
		}
		++LoadedCount;

		bool bChanged = false;
		int32 ConfiguredAttackAudioCount = 0;
		for (FGridMonsterAttackDefinition& Attack : Definition->Attacks)
		{
			if (!Attack.AttackSound.IsNull())
			{
				if (Attack.AttackAudio.HasConfiguredSound() &&
					!ContainsSound(Attack.AttackAudio, Attack.AttackSound))
				{
					AddError(FString::Printf(
						TEXT("%s attack %s has ambiguous AttackSound + AttackAudio authoring."),
						Target.Label, *Attack.AttackId.ToString()));
					continue;
				}

				Definition->Modify();
				if (!Attack.AttackAudio.HasConfiguredSound())
				{
					Attack.AttackAudio.Sounds.Add(Attack.AttackSound);
				}
				Attack.AttackSound.Reset();
				bChanged = true;
			}

			if (!Attack.ImpactVFX.IsNull())
			{
				if (Attack.ImpactHitVFXDefinition.HasConfiguredSystem() &&
					!ContainsSystem(Attack.ImpactHitVFXDefinition, Attack.ImpactVFX))
				{
					AddError(FString::Printf(
						TEXT("%s attack %s has ambiguous ImpactVFX + ImpactHitVFXDefinition authoring."),
						Target.Label, *Attack.AttackId.ToString()));
					continue;
				}

				Definition->Modify();
				if (!Attack.ImpactHitVFXDefinition.HasConfiguredSystem())
				{
					Attack.ImpactHitVFXDefinition.Systems.Add(Attack.ImpactVFX);
				}
				Attack.ImpactVFX.Reset();
				bChanged = true;
			}

			if (Attack.AttackAudio.HasConfiguredSound())
			{
				++ConfiguredAttackAudioCount;
			}
		}

		TestTrue(*FString::Printf(TEXT("%s keeps at least one configured AttackAudio"), Target.Label),
			ConfiguredAttackAudioCount > 0);
		TestTrue(*FString::Printf(TEXT("%s remains a valid monster definition"), Target.Label),
			Definition->IsValidDefinition());

		for (const FGridMonsterAttackDefinition& Attack : Definition->Attacks)
		{
			TestTrue(*FString::Printf(TEXT("%s attack %s clears legacy AttackSound"),
				Target.Label, *Attack.AttackId.ToString()), Attack.AttackSound.IsNull());
			TestTrue(*FString::Printf(TEXT("%s attack %s clears legacy ImpactVFX"),
				Target.Label, *Attack.AttackId.ToString()), Attack.ImpactVFX.IsNull());
		}

		if (bChanged)
		{
			Definition->MarkPackageDirty();
			TestTrue(*FString::Printf(TEXT("%s repaired package saves"), Target.Label),
				SaveAssetPackage(Definition));
			++ChangedAssetCount;
		}
	}

	TestEqual(TEXT("Both monster DataAssets load"), LoadedCount, 2);
	AddInfo(ChangedAssetCount > 0
		? FString::Printf(TEXT("TD07.3.5.3 repaired and saved %d monster presentation asset(s)."), ChangedAssetCount)
		: TEXT("TD07.3.5.3 monster presentation assets were already current; no package mutation was required."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
