#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06PresentationProjection.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace ArmClean062G
{
	bool EqualAudio(const FGridPlayerAttackAudioDefinition& A,
		const FGridCanonicalAudioVariants& B)
	{
		if (A.VolumeMultiplier != B.VolumeMultiplier || A.PitchMin != B.PitchMin ||
			A.PitchMax != B.PitchMax || A.Sounds.Num() != B.Sounds.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Sounds.Num(); ++Index)
		{
			if (A.Sounds[Index].ToSoftObjectPath() != B.Sounds[Index].ToSoftObjectPath())
			{
				return false;
			}
		}
		return true;
	}

	bool EqualVFX(const FGridPlayerAttackVFXDefinition& A,
		const FGridCanonicalVFXVariants& B)
	{
		if (A.bAttachToHeldItem != B.bAttachToHeldItem || A.SocketName != B.SocketName ||
			A.LocationOffset != B.LocationOffset || A.RotationOffset != B.RotationOffset ||
			A.Scale != B.Scale || A.Systems.Num() != B.Systems.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Systems.Num(); ++Index)
		{
			if (A.Systems[Index].ToSoftObjectPath() != B.Systems[Index].ToSoftObjectPath())
			{
				return false;
			}
		}
		return true;
	}

	bool EqualPresentation(const FGridPlayerAttackPresentationProfile& A,
		const FGridCanonicalActionPresentation& B)
	{
		return A.MotionStyle == B.MotionStyle &&
			A.bAnimateHeldItem == B.bAnimateHeldItem &&
			A.MotionDurationSeconds == B.MotionDurationSeconds &&
			A.ImpactDelaySeconds == B.ImpactDelaySeconds &&
			A.RecoveryDelaySeconds == B.RecoveryDelaySeconds &&
			A.PeakLocationOffset == B.PeakLocationOffset &&
			A.PeakRotationOffset == B.PeakRotationOffset &&
			A.FeedbackDurationSeconds == B.FeedbackDurationSeconds &&
			EqualAudio(A.AttackAudio, B.AttackAudio) &&
			EqualAudio(A.ImpactHitAudio, B.ImpactHitAudio) &&
			EqualAudio(A.ImpactMissAudio, B.ImpactMissAudio) &&
			EqualVFX(A.AttackVFX, B.AttackVFX) &&
			EqualVFX(A.ImpactHitVFX, B.ImpactHitVFX) &&
			EqualVFX(A.ImpactMissVFX, B.ImpactMissVFX);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062GProductionActionsPresentationsTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionPresentationParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062GProductionActionsPresentationsTest::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("Manifest validated using production reader"),
		FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("Exactly 22 canonical production ARM items"), Rows.Num(), 22))
	{
		return false;
	}
	int32 Active = 0;
	int32 Passive = 0;
	int32 ActionCount = 0;
	int32 OffensiveCount = 0;
	int32 SourcesWithPresentations = 0;
	int32 SourcesWithoutPresentations = 0;
	int32 MissingPresentationKeys = 0;
	bool bAllEqual = true;
	TSet<FName> ArmIds;
	for (const FGridWeaponPipelineRow& Row : Rows)
	{
		if (Row.ProductionState != TEXT("Active") ||
			ArmIds.Contains(FName(*Row.ArchetypeId)))
		{
			AddError(TEXT("Duplicate or inactive row in the 22-production manifest"));
			return false;
		}
		ArmIds.Add(FName(*Row.ArchetypeId));
		++Active;
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!IsValid(Item) || !Item->bUseWeaponCatalogue ||
			Item->WeaponArchetypeId != FName(*Row.ArchetypeId) ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
			!Item->HasValidCombatActions())
		{
			AddError(TEXT("Active ARM item failed identity/action validity"));
			return false;
		}
		const bool bWasDirty = Item->GetOutermost()->IsDirty();
		if (Row.IsPassive()) { ++Passive; }
		TSet<FName> SourceActionIds;
		bool bManifestAttackFound = false;
		for (const FGridCombatActionDefinition& Action : Item->CombatActions)
		{
			++ActionCount;
			if (!Action.IsValid() ||
				Action.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
				Action.ActionId.IsNone() || SourceActionIds.Contains(Action.ActionId))
			{
				AddError(TEXT("Production ARM has invalid or duplicate local combat action"));
				return false;
			}
			SourceActionIds.Add(Action.ActionId);
			if (Action.PresentationProfileId.IsNone()) { ++MissingPresentationKeys; }
			if (Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack)
			{
				++OffensiveCount;
				if (Row.IsPassive())
				{
					AddError(TEXT("Passive ARM must not gain an authored equipment attack"));
					return false;
				}
				bManifestAttackFound |= (Action.ActionId == FName(*Row.AttackId));
			}
		}
		if (!Row.IsPassive() && !bManifestAttackFound)
		{
			AddError(TEXT("Authored primary attack does not match the production manifest"));
			return false;
		}
		if (Item->bProvidesAttackPresentation)
		{
			++SourcesWithPresentations;
			FGridCanonicalActionPresentation Projected;
			// Ephemeral test identity only, not an authored PresentationId.
			const FName TestId(*FString::Printf(TEXT("TestProjection_%s"), *Row.ArchetypeId));
			if (!GridArmClean06PresentationProjection::TryProject(
					TestId, Item->PlayerAttackPresentationProfile, Projected) ||
				!ArmClean062G::EqualPresentation(
					Item->PlayerAttackPresentationProfile, Projected) ||
				!Item->HasValidPlayerAttackPresentation())
			{
				AddError(TEXT("A real production ARM presentation cannot project without loss"));
				bAllEqual = false;
			}
		}
		else
		{
			++SourcesWithoutPresentations;
		}
		if (Item->GetOutermost()->IsDirty() != bWasDirty)
		{
			AddError(TEXT("Read-only production projection unexpectedly dirtied an asset"));
			return false;
		}
	}
	AddInfo(FString::Printf(TEXT(
		"ARM-CLEAN06.2G production-only: %d items, %d passives, %d actions, "
		"%d offensive, %d real presentations, %d absent presentations, "
		"%d unset legacy PresentationProfileId. All IDs remain private to the source." ),
		Active, Passive, ActionCount, OffensiveCount,
		SourcesWithPresentations, SourcesWithoutPresentations, MissingPresentationKeys));
	TestEqual(TEXT("22 active ARM rows"), Active, 22);
	TestEqual(TEXT("3 passive ARM rows"), Passive, 3);
	TestTrue(TEXT("At least one actual ARM presentation tested"), SourcesWithPresentations > 0);
	return bAllEqual && Active == 22 && Passive == 3 &&
		SourcesWithPresentations > 0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062GProjectionSafetyTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.PresentationProjectionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062GProjectionSafetyTest::RunTest(const FString&)
{
	FGridPlayerAttackPresentationProfile Source;
	FGridCanonicalActionPresentation Out;
	TestFalse(TEXT("No synthetic presentation identity"),
		GridArmClean06PresentationProjection::TryProject(NAME_None, Source, Out));
	TestTrue(TEXT("Default source may project under explicit transient test ID"),
		GridArmClean06PresentationProjection::TryProject(TEXT("TransientOnly"), Source, Out));
	TestEqual(TEXT("Explicit ID survives"), Out.PresentationId, FName(TEXT("TransientOnly")));
	Source.AttackAudio.PitchMin = 1.5f;
	Source.AttackAudio.PitchMax = 0.7f;
	TestFalse(TEXT("Malformed source never silently converted"),
		GridArmClean06PresentationProjection::TryProject(TEXT("TransientOnly"), Source, Out));
	TestTrue(TEXT("Rejected output carries no stale presentation identity"),
		Out.PresentationId.IsNone());
	return true;
}
#endif
