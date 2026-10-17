#include "UI/SkillAffinityProductionAuthoring.h"

#include "UI/RPGSkillPresentationAsset.h"

namespace SkillAffinityProductionAuthoring
{
	struct FImmutablePresentationSnapshot
	{
		FName SkillId = NAME_None;
		TSoftObjectPtr<UTexture2D> Icon;
		FText UsageDescription;
		FText BuildAdvice;
		TArray<FName> RecommendedClassIds;
		TArray<FName> UsefulClassIds;
		TArray<FName> SynergySkillIds;

		explicit FImmutablePresentationSnapshot(const FRPGSkillPresentationDefinition& Source)
			: SkillId(Source.SkillId)
			, Icon(Source.Icon)
			, UsageDescription(Source.UsageDescription)
			, BuildAdvice(Source.BuildAdvice)
			, RecommendedClassIds(Source.RecommendedClassIds)
			, UsefulClassIds(Source.UsefulClassIds)
			, SynergySkillIds(Source.SynergySkillIds)
		{
		}

		bool Matches(const FRPGSkillPresentationDefinition& Candidate) const
		{
			return Candidate.SkillId == SkillId
				&& Candidate.Icon == Icon
				&& Candidate.UsageDescription.EqualTo(UsageDescription)
				&& Candidate.BuildAdvice.EqualTo(BuildAdvice)
				&& Candidate.RecommendedClassIds == RecommendedClassIds
				&& Candidate.UsefulClassIds == UsefulClassIds
				&& Candidate.SynergySkillIds == SynergySkillIds;
		}
	};

	const TMap<FName, TArray<FName>>& CanonicalDiscouragedMatrix()
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
}

void FSkillAffinityProductionAuthoring::GetCanonicalDiscouragedClassIds(
	FName SkillId,
	TArray<FName>& OutClassIds)
{
	OutClassIds.Reset();
	if (const TArray<FName>* Found = SkillAffinityProductionAuthoring::CanonicalDiscouragedMatrix().Find(SkillId))
	{
		OutClassIds = *Found;
	}
}

bool FSkillAffinityProductionAuthoring::ApplyCanonicalDiscouragedAffinities(
	URPGSkillPresentationAsset& Catalog,
	int32& OutChangedSkillCount,
	FString& OutError)
{
	OutChangedSkillCount = 0;
	OutError.Reset();

	if (!Catalog.IsValidDefinition())
	{
		OutError = TEXT("Production Skill presentation catalog is structurally invalid before authoring.");
		return false;
	}
	if (Catalog.Skills.Num() != ExpectedProductionSkillCount)
	{
		OutError = FString::Printf(
			TEXT("Expected %d production Skill presentations, found %d."),
			ExpectedProductionSkillCount,
			Catalog.Skills.Num());
		return false;
	}

	TMap<FName, SkillAffinityProductionAuthoring::FImmutablePresentationSnapshot> Snapshots;
	Snapshots.Reserve(Catalog.Skills.Num());
	TSet<FName> SkillIds;
	for (const FRPGSkillPresentationDefinition& Presentation : Catalog.Skills)
	{
		SkillIds.Add(Presentation.SkillId);
		Snapshots.Emplace(Presentation.SkillId, SkillAffinityProductionAuthoring::FImmutablePresentationSnapshot(Presentation));
	}

	for (const TPair<FName, TArray<FName>>& Pair : SkillAffinityProductionAuthoring::CanonicalDiscouragedMatrix())
	{
		if (!SkillIds.Contains(Pair.Key))
		{
			OutError = FString::Printf(TEXT("Canonical discouraged matrix references missing Skill %s."), *Pair.Key.ToString());
			return false;
		}
	}

	for (FRPGSkillPresentationDefinition& Presentation : Catalog.Skills)
	{
		TArray<FName> Expected;
		GetCanonicalDiscouragedClassIds(Presentation.SkillId, Expected);
		if (Presentation.DiscouragedClassIds != Expected)
		{
			Presentation.DiscouragedClassIds = MoveTemp(Expected);
			++OutChangedSkillCount;
		}
	}

	if (!Catalog.IsValidDefinition())
	{
		OutError = TEXT("Canonical discouraged matrix conflicts with existing Recommended/Useful production metadata.");
		return false;
	}

	for (const FRPGSkillPresentationDefinition& Presentation : Catalog.Skills)
	{
		const SkillAffinityProductionAuthoring::FImmutablePresentationSnapshot* Snapshot = Snapshots.Find(Presentation.SkillId);
		if (!Snapshot || !Snapshot->Matches(Presentation))
		{
			OutError = FString::Printf(
				TEXT("Authoring changed a protected presentation field for %s."),
				*Presentation.SkillId.ToString());
			return false;
		}
	}

	return HasCanonicalDiscouragedAffinities(Catalog, OutError);
}

bool FSkillAffinityProductionAuthoring::HasCanonicalDiscouragedAffinities(
	const URPGSkillPresentationAsset& Catalog,
	FString& OutError)
{
	OutError.Reset();
	if (Catalog.Skills.Num() != ExpectedProductionSkillCount)
	{
		OutError = FString::Printf(
			TEXT("Expected %d production Skill presentations, found %d."),
			ExpectedProductionSkillCount,
			Catalog.Skills.Num());
		return false;
	}

	for (const FRPGSkillPresentationDefinition& Presentation : Catalog.Skills)
	{
		TArray<FName> Expected;
		GetCanonicalDiscouragedClassIds(Presentation.SkillId, Expected);
		if (Presentation.DiscouragedClassIds != Expected)
		{
			OutError = FString::Printf(
				TEXT("%s has non-canonical DiscouragedClassIds."),
				*Presentation.SkillId.ToString());
			return false;
		}
	}

	return true;
}
