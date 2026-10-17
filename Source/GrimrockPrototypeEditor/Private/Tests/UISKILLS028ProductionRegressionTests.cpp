#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGSkillAsset.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidget.h"
#include "UI/RPGSkillPresentationAsset.h"

namespace UISKILLS028Tests
{
	constexpr int32 ExpectedProductionSkillCount = 25;

	const TCHAR* SkillPresentationPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGSkillPresentation.DA_RPGSkillPresentation");

	const TCHAR* SkillBlueprintPaths[] =
	{
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillEntry.WBP_RPGSkillEntry"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGSkillDetail.WBP_RPGSkillDetail"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills")
	};

	const FName CanonicalClassIds[] =
	{
		TEXT("Warrior"),
		TEXT("Rogue"),
		TEXT("Ranger"),
		TEXT("Mage"),
		TEXT("Priest"),
		TEXT("Alchemist")
	};

	const TMap<FName, TArray<FName>>& ExpectedDiscouragedMatrix()
	{
		static const TMap<FName, TArray<FName>> Matrix =
		{
			{ TEXT("Skill_Lockpicking"), { TEXT("Priest") } },
			{ TEXT("Skill_Traps"), { TEXT("Warrior"), TEXT("Priest") } },
			{ TEXT("Skill_Alchemy"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
			{ TEXT("Skill_Arcana"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
			{ TEXT("Skill_Runes"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
			{ TEXT("Skill_Religion"), { TEXT("Warrior"), TEXT("Rogue"), TEXT("Alchemist") } }
		};
		return Matrix;
	}

	struct FResolverCacheGuard
	{
		FResolverCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}

		~FResolverCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS028ProductionRegressionTest,
	"Grimrock.UI.Skills02.Production.CatalogIconsAndWiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS028ProductionRegressionTest::RunTest(const FString&)
{
	using namespace UISKILLS028Tests;
	FResolverCacheGuard CacheGuard;

	URPGSkillPresentationAsset* Catalog =
		LoadObject<URPGSkillPresentationAsset>(nullptr, SkillPresentationPath);
	if (!TestNotNull(TEXT("Production Skill presentation catalog loads"), Catalog))
	{
		return false;
	}

	TestTrue(TEXT("Production Skill presentation catalog is structurally valid"),
		Catalog->IsValidDefinition());
	TestEqual(TEXT("Production presentation catalog contains exactly 25 Skills"),
		Catalog->Skills.Num(), ExpectedProductionSkillCount);

	TArray<const URPGSkillAsset*> CanonicalSkills;
	FGridSkillsPageService::ResolveCanonicalSkillDefinitions(CanonicalSkills);
	TestEqual(TEXT("Canonical production Skill catalog contains exactly 25 Skills"),
		CanonicalSkills.Num(), ExpectedProductionSkillCount);

	TSet<FName> CanonicalSkillIds;
	for (const URPGSkillAsset* Skill : CanonicalSkills)
	{
		if (!TestNotNull(TEXT("Canonical Skill definition resolves"), Skill))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s is a valid canonical Skill definition"), *Skill->SkillId.ToString()),
			Skill->IsValidDefinition());
		TestFalse(
			*FString::Printf(TEXT("%s is unique in canonical Skill definitions"), *Skill->SkillId.ToString()),
			CanonicalSkillIds.Contains(Skill->SkillId));
		CanonicalSkillIds.Add(Skill->SkillId);
	}

	int32 DiscouragedSkillCount = 0;
	int32 DiscouragedAssignmentCount = 0;
	TSet<FName> PresentedSkillIds;
	for (const FRPGSkillPresentationDefinition& Presentation : Catalog->Skills)
	{
		const FString SkillLabel = Presentation.SkillId.ToString();

		TestTrue(
			*FString::Printf(TEXT("%s presentation maps to a canonical Skill"), *SkillLabel),
			CanonicalSkillIds.Contains(Presentation.SkillId));
		TestFalse(
			*FString::Printf(TEXT("%s presentation id is unique"), *SkillLabel),
			PresentedSkillIds.Contains(Presentation.SkillId));
		PresentedSkillIds.Add(Presentation.SkillId);

		TestFalse(
			*FString::Printf(TEXT("%s has authored usage guidance"), *SkillLabel),
			Presentation.UsageDescription.IsEmpty());
		TestFalse(
			*FString::Printf(TEXT("%s has authored build advice"), *SkillLabel),
			Presentation.BuildAdvice.IsEmpty());
		TestFalse(
			*FString::Printf(TEXT("%s has a production icon reference"), *SkillLabel),
			Presentation.Icon.IsNull());

		UTexture2D* Icon = Presentation.Icon.LoadSynchronous();
		if (TestNotNull(*FString::Printf(TEXT("%s icon loads"), *SkillLabel), Icon))
		{
			TestTrue(
				*FString::Printf(TEXT("%s icon uses UI texture group"), *SkillLabel),
				Icon->LODGroup == TEXTUREGROUP_UI);
			TestTrue(
				*FString::Printf(TEXT("%s icon uses UserInterface2D compression"), *SkillLabel),
				Icon->CompressionSettings == TC_EditorIcon);
			TestTrue(
				*FString::Printf(TEXT("%s icon has mip generation disabled"), *SkillLabel),
				Icon->MipGenSettings == TMGS_NoMipmaps);
			TestTrue(
				*FString::Printf(TEXT("%s icon keeps sRGB enabled"), *SkillLabel),
				Icon->SRGB != 0);
		}

		for (const FName ClassId : Presentation.RecommendedClassIds)
		{
			TestNotNull(
				*FString::Printf(TEXT("%s recommended class %s resolves"), *SkillLabel, *ClassId.ToString()),
				FRPGAuthoringIdentityResolver::ResolveClassById(ClassId));
		}
		for (const FName ClassId : Presentation.UsefulClassIds)
		{
			TestNotNull(
				*FString::Printf(TEXT("%s useful class %s resolves"), *SkillLabel, *ClassId.ToString()),
				FRPGAuthoringIdentityResolver::ResolveClassById(ClassId));
		}
		for (const FName ClassId : Presentation.DiscouragedClassIds)
		{
			TestNotNull(
				*FString::Printf(TEXT("%s discouraged class %s resolves"), *SkillLabel, *ClassId.ToString()),
				FRPGAuthoringIdentityResolver::ResolveClassById(ClassId));
		}

		const TArray<FName>* ExpectedDiscouraged =
			ExpectedDiscouragedMatrix().Find(Presentation.SkillId);
		const TArray<FName> EmptyDiscouraged;
		const TArray<FName>& ExpectedDiscouragedIds =
			ExpectedDiscouraged ? *ExpectedDiscouraged : EmptyDiscouraged;

		TestTrue(
			*FString::Printf(TEXT("%s has exact production DiscouragedClassIds"), *SkillLabel),
			Presentation.DiscouragedClassIds == ExpectedDiscouragedIds);

		if (!Presentation.DiscouragedClassIds.IsEmpty())
		{
			++DiscouragedSkillCount;
			DiscouragedAssignmentCount += Presentation.DiscouragedClassIds.Num();
		}

		for (const FName ClassId : CanonicalClassIds)
		{
			EGridSkillClassAffinity ExpectedAffinity = EGridSkillClassAffinity::Neutral;
			if (Presentation.RecommendedClassIds.Contains(ClassId))
			{
				ExpectedAffinity = EGridSkillClassAffinity::Recommended;
			}
			else if (Presentation.UsefulClassIds.Contains(ClassId))
			{
				ExpectedAffinity = EGridSkillClassAffinity::Useful;
			}
			else if (Presentation.DiscouragedClassIds.Contains(ClassId))
			{
				ExpectedAffinity = EGridSkillClassAffinity::Discouraged;
			}

			TestEqual(
				*FString::Printf(TEXT("%s / %s production affinity resolves exactly"), *SkillLabel, *ClassId.ToString()),
				Catalog->ResolveClassAffinity(Presentation.SkillId, ClassId),
				ExpectedAffinity);
		}
		for (const FName SynergySkillId : Presentation.SynergySkillIds)
		{
			TestTrue(
				*FString::Printf(TEXT("%s synergy %s is canonical"), *SkillLabel, *SynergySkillId.ToString()),
				CanonicalSkillIds.Contains(SynergySkillId));
		}
	}

	TestEqual(TEXT("Exactly six production Skills carry discouraged affinities"),
		DiscouragedSkillCount, 6);
	TestEqual(TEXT("Production discouraged matrix contains exactly fifteen class assignments"),
		DiscouragedAssignmentCount, 15);

	TestEqual(TEXT("Presentation SkillIds exactly cover canonical SkillIds"),
		PresentedSkillIds.Num(), CanonicalSkillIds.Num());
	for (const FName SkillId : CanonicalSkillIds)
	{
		TestTrue(
			*FString::Printf(TEXT("%s has production presentation metadata"), *SkillId.ToString()),
			PresentedSkillIds.Contains(SkillId));
	}

	for (const TCHAR* BlueprintPath : SkillBlueprintPaths)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s loads"), BlueprintPath), Blueprint))
		{
			continue;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestTrue(
			*FString::Printf(TEXT("%s recompiles against the hardened Skill contracts"), BlueprintPath),
			Blueprint->Status != BS_Error);
	}

	UBlueprint* GridSkillsBlueprint =
		LoadObject<UBlueprint>(nullptr, SkillBlueprintPaths[2]);
	if (GridSkillsBlueprint && GridSkillsBlueprint->GeneratedClass)
	{
		const UGridSkillsWidget* GridSkillsCDO =
			Cast<UGridSkillsWidget>(GridSkillsBlueprint->GeneratedClass->GetDefaultObject());
		if (TestNotNull(TEXT("WBP_GridSkills generated CDO is a UGridSkillsWidget"), GridSkillsCDO))
		{
			TestTrue(TEXT("WBP_GridSkills is wired to the production Skill presentation catalog"),
				GridSkillsCDO->SkillPresentationAsset == Catalog);
			TestNotNull(TEXT("WBP_GridSkills has a concrete Skill row class"),
				GridSkillsCDO->SkillEntryWidgetClass.Get());
		}
	}

	return true;
}

#endif
