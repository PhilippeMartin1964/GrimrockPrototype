#if WITH_DEV_AUTOMATION_TESTS

#include "GridCanonicalWeightTestWorld.h"
#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridAttackPresentationReadModel.h"
#include "Runtime/GridThrownItemActor.h"
#include "Sound/SoundBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridArmamentProjectilePresentationTest, "Grimrock.RPG.ARMEMENT.FX01.ProjectilePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridArmamentProjectilePresentationTest::RunTest(const FString&)
{
	FGridCanonicalWeightTestWorld Fixture({});
	if (!TestNotNull(TEXT("Canonical registry"), Fixture.Scope.Registry))
	{
		return false;
	}
	for (const bool bBow : { true, false })
	{
		AGridThrownItemActor* Projectile = Fixture.World->SpawnActor<AGridThrownItemActor>();
		if (!TestNotNull(TEXT("Existing physical projectile actor"), Projectile))
		{
			return false;
		}
		FGridItemInstance Source;
		Source.RuntimeObjectId = FGuid::NewGuid();
		Source.OwnerGuid = FGuid::NewGuid();
		Source.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Source.OwnerCharacterIndex = 0;
		Source.EquipmentSlot = EGridEquipmentSlot::MainHand;
		Source.ItemDefinitionId = bBow ? TEXT("Weapon_RecurveBow_01") : TEXT("Weapon_ThrowingKnife_01");
		Source.ItemLevel = bBow ? 11 : 8;
		Projectile->ThrownItemInstance = Source;
		if (bBow)
		{
			Projectile->ThrownItemInstance.ItemDefinitionId = TEXT("Item_Arrows_01");
			Projectile->ThrownItemInstance.RuntimeObjectId = FGuid::NewGuid();
			Projectile->ThrownItemInstance.ItemLevel = 0;
		}
		const FGridItemInstance UnitBefore = Projectile->ThrownItemInstance;

		FGridPlayerAttackRequest Request;
		Request.OffensiveItemDefinitionId = Source.ItemDefinitionId;
		Request.OffensiveEquipmentSlot = Source.EquipmentSlot;
		Request.AttackId = bBow ? TEXT("Attack_RecurveBow_01") : TEXT("Attack_ThrowingKnife_01");
		Request.PresentationSource = FGridAttackPresentationReadModel::CaptureFromInstance(
			Fixture.Scope.Registry, Source, Source.OwnerGuid, 0, Source.EquipmentSlot, Request.AttackId, false);
		Projectile->StartCombatFlightPresentation(Request, *Fixture.Scope.Registry, false, true);
		TestFalse(TEXT("A rejected or uncommitted request has no FX"), Projectile->AttackSourceGuid.IsValid());
		Request.PreparedThrownItemActor = Projectile;
		Projectile->StartCombatFlightPresentation(Request, *Fixture.Scope.Registry, false, true);
		TestEqual(TEXT("Source weapon GUID survives the transfer"), Projectile->AttackSourceGuid, Source.RuntimeObjectId);
		TestEqual(TEXT("Source weapon level remains independent of ammo"), Projectile->AttackSourceItemLevel, Source.ItemLevel);
		TestTrue(TEXT("Release, collision and stop have authored media"),
			!Projectile->ProjectileCues.ReleaseVFX.Systems.IsEmpty() && !Projectile->ProjectileCues.CollisionVFX.Systems.IsEmpty() &&
				!Projectile->ProjectileCues.StopVFX.Systems.IsEmpty());
		TestTrue(TEXT("Source hand is the real hand of launch"), Projectile->AttackSourceHand == EGridEquipmentSlot::MainHand);
		// A second delegate delivery cannot replace a launched unit's immutable source.
		Request.PresentationSource.RuntimeObjectId = FGuid::NewGuid();
		Projectile->StartCombatFlightPresentation(Request, *Fixture.Scope.Registry, false, true);
		TestEqual(TEXT("Duplicate release cannot change the recorded source"), Projectile->AttackSourceGuid, Source.RuntimeObjectId);

		if (bBow)
		{
			FHitResult Contact;
			Contact.bBlockingHit = true;
			Contact.ImpactNormal = FVector::UpVector;
			Projectile->HandleProjectileStopped(Contact);
			TestTrue(TEXT("Physics stop with a blocking hit presents collision"), Projectile->bPresentedFlightCollision);
		}
		else
		{
			Projectile->StopCombatFlightPresentation(FVector::ZeroVector);
			Projectile->PresentFlightCollision(FVector::ZeroVector, FVector::UpVector);
			TestFalse(TEXT("A projectile which has already stopped cannot fake a new contact"), Projectile->bPresentedFlightCollision);
		}
		TestTrue(TEXT("Stop marks the lifecycle as ended"), Projectile->bPresentedFlightStop);
		TestFalse(TEXT("Stop clears the attached flight presentation"), Projectile->FlightEffect.IsValid());
		TestEqual(TEXT("Media preserve the projectile GUID"), Projectile->ThrownItemInstance.RuntimeObjectId, UnitBefore.RuntimeObjectId);
		TestEqual(TEXT("Media preserve the projectile level"), Projectile->ThrownItemInstance.ItemLevel, UnitBefore.ItemLevel);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridFireWandImpactPresentationTest, "Grimrock.RPG.ARMEMENT.FX01.FireWandImpact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridFireWandImpactPresentationTest::RunTest(const FString&)
{
	FGridCanonicalWeightTestWorld Fixture({});
	FGridPlayerAttackRequest Request;
	Request.OffensiveItemDefinitionId = TEXT("Weapon_FireWand_01");
	Request.OffensiveEquipmentSlot = EGridEquipmentSlot::MainHand;
	Request.PresentationSource.RuntimeObjectId = FGuid::NewGuid();
	Request.PresentationSource.ItemLevel = 3;
	Request.PresentationSource.ActionId = TEXT("Attack_FireWand_01");
	FGridPlayerAttackPresentationProfile Profile;
	if (!TestTrue(TEXT("Production fire wand resolves a presentation"), FGridAttackPresentationReadModel::TryResolve(Fixture.Scope.Registry, Request, Profile)))
	{
		return false;
	}
	TestTrue(TEXT("Catalyst has distinct release, hit and miss VFX"),
		!Profile.AttackVFX.Systems.IsEmpty() && !Profile.ImpactHitVFX.Systems.IsEmpty() && !Profile.ImpactMissVFX.Systems.IsEmpty() &&
			Profile.ImpactHitVFX.Systems != Profile.ImpactMissVFX.Systems);
	TestTrue(TEXT("No hit sound on a magic miss"), Profile.ImpactMissAudio.Sounds.IsEmpty());
	if (TestEqual(TEXT("Successful fire contact has one authored transient"), Profile.ImpactHitAudio.Sounds.Num(), 1))
	{
		TestEqual(
			TEXT("Native fire impact SoundWave"), GetNameSafe(Profile.ImpactHitAudio.Sounds[0].LoadSynchronous()), FString(TEXT("SFX_Focus_Fire_Hit_01")));
	}
	return true;
}

#endif
