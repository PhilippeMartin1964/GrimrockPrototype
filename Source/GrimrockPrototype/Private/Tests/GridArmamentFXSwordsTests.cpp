#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "NiagaraSystem.h"
#include "Runtime/Combat/GridAttackPresentationReadModel.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Sound/SoundBase.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridArmamentFXSwordsTest, "Grimrock.RPG.ARMEMENT.FX01.SwordPilot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridArmamentFXSwordsTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Production registry loaded"), Fixture.bReady))
	{
		return false;
	}

	const FName Items[] = { TEXT("Weapon_StraightSword_01"), TEXT("Weapon_LongSword_01") };
	const FName Actions[] = { TEXT("Attack_StraightSword_01"), TEXT("Attack_LongSword_01") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Items); ++Index)
	{
		FGridPlayerAttackRequest Request;
		Request.OffensiveItemDefinitionId = Items[Index];
		Request.OffensiveEquipmentSlot = EGridEquipmentSlot::MainHand;
		Request.PresentationSource.RuntimeObjectId = FGuid::NewGuid();
		Request.PresentationSource.ItemLevel = Index == 0 ? 1 : 20;
		Request.PresentationSource.ActionId = Actions[Index];
		FGridPlayerAttackPresentationProfile Presentation;
		if (!TestTrue(TEXT("The physical sword resolves its canonical presentation"),
				FGridAttackPresentationReadModel::TryResolve(Fixture.Registry.Get(), Request, Presentation)))
		{
			continue;
		}
		TestEqual(TEXT("Four standard swings remain available"), Presentation.AttackAudio.Sounds.Num(), 4);
		TestEqual(TEXT("Four distinct follow-through cues for a miss"), Presentation.ImpactMissAudio.Sounds.Num(), 4);
		TestEqual(TEXT("Four flesh impacts are projected from the catalogue"), Presentation.ImpactFleshAudio.Sounds.Num(), 4);
		for (const FGridPlayerAttackAudioDefinition* Audio : { &Presentation.AttackAudio, &Presentation.ImpactMissAudio, &Presentation.ImpactFleshAudio })
		{
			for (const TSoftObjectPtr<USoundBase>& Sound : Audio->Sounds)
			{
				TestNotNull(TEXT("Authored sword audio resolves"), Sound.LoadSynchronous());
			}
		}
		TestEqual(TEXT("One contact burst on a successful hit"), Presentation.ImpactHitVFX.Systems.Num(), 1);
		for (const TSoftObjectPtr<UNiagaraSystem>& System : Presentation.ImpactHitVFX.Systems)
		{
			TestNotNull(TEXT("Authored Niagara contact resolves"), System.LoadSynchronous());
		}
		TestTrue(TEXT("Miss does not emit a contact effect"), Presentation.ImpactMissVFX.Systems.IsEmpty());
	}
	const UGridMonsterDefinitionAsset* Rat =
		LoadObject<UGridMonsterDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
	if (TestNotNull(TEXT("Production rat definition"), Rat))
	{
		TestEqual(TEXT("Exposed rat body has an authored material"), Rat->BodyImpactSurface, EGridAttackImpactSurface::Flesh);
	}
	return true;
}

#endif
