#include "UI/SkillAffinityAuditCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGSkillAsset.h"
#include "UI/GridSkillsPageService.h"
#include "UI/RPGSkillPresentationAsset.h"

namespace SkillAffinityAuditCommandlet
{
	constexpr int32 ExpectedProductionSkillCount = 25;

	const TCHAR* SkillPresentationPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGSkillPresentation.DA_RPGSkillPresentation");

	const FName CanonicalClassIds[] =
	{
		TEXT("Warrior"),
		TEXT("Rogue"),
		TEXT("Ranger"),
		TEXT("Mage"),
		TEXT("Priest"),
		TEXT("Alchemist")
	};

	FString CsvEscape(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	FString JoinNames(const TArray<FName>& Values)
	{
		TArray<FString> Strings;
		Strings.Reserve(Values.Num());
		for (const FName Value : Values)
		{
			Strings.Add(Value.ToString());
		}
		return FString::Join(Strings, TEXT(";"));
	}

	FString ResolveDisplayName(
		FName SkillId,
		const TMap<FName, FText>& DisplayNames)
	{
		if (const FText* Found = DisplayNames.Find(SkillId))
		{
			return Found->ToString();
		}
		return SkillId.ToString();
	}
}

USkillAffinityAuditCommandlet::USkillAffinityAuditCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 USkillAffinityAuditCommandlet::Main(const FString& Params)
{
	(void)Params;

	FRPGAuthoringIdentityResolver::ResetRuntimeCache();

	URPGSkillPresentationAsset* Catalog =
		LoadObject<URPGSkillPresentationAsset>(nullptr, SkillAffinityAuditCommandlet::SkillPresentationPath);
	if (!IsValid(Catalog))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-SKILLS-AFFINITY01.3A] Missing production Skill presentation catalog."));
		return 1;
	}

	if (!Catalog->IsValidDefinition())
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-SKILLS-AFFINITY01.3A] Production Skill presentation catalog is structurally invalid."));
		return 1;
	}

	if (Catalog->Skills.Num() != SkillAffinityAuditCommandlet::ExpectedProductionSkillCount)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-SKILLS-AFFINITY01.3A] Expected %d Skill presentations, found %d."),
			SkillAffinityAuditCommandlet::ExpectedProductionSkillCount,
			Catalog->Skills.Num());
		return 1;
	}

	TSet<FName> CanonicalClasses;
	for (const FName ClassId : SkillAffinityAuditCommandlet::CanonicalClassIds)
	{
		if (!FRPGAuthoringIdentityResolver::ResolveClassById(ClassId))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[UI-SKILLS-AFFINITY01.3A] Canonical class %s does not resolve."),
				*ClassId.ToString());
			return 1;
		}
		CanonicalClasses.Add(ClassId);
	}

	TArray<const URPGSkillAsset*> CanonicalSkills;
	FGridSkillsPageService::ResolveCanonicalSkillDefinitions(CanonicalSkills);
	TMap<FName, FText> DisplayNames;
	for (const URPGSkillAsset* Skill : CanonicalSkills)
	{
		if (IsValid(Skill))
		{
			DisplayNames.Add(Skill->SkillId, Skill->DisplayName);
		}
	}

	TArray<const FRPGSkillPresentationDefinition*> Sorted;
	Sorted.Reserve(Catalog->Skills.Num());
	for (const FRPGSkillPresentationDefinition& Presentation : Catalog->Skills)
	{
		Sorted.Add(&Presentation);
	}
	Sorted.Sort([](const FRPGSkillPresentationDefinition& Left, const FRPGSkillPresentationDefinition& Right)
	{
		return Left.SkillId.LexicalLess(Right.SkillId);
	});

	FString Csv = TEXT("SkillId,DisplayName,RecommendedClassIds,UsefulClassIds,DiscouragedClassIds,NeutralClassIds\n");
	int32 RecommendedCount = 0;
	int32 UsefulCount = 0;
	int32 DiscouragedCount = 0;

	for (const FRPGSkillPresentationDefinition* Presentation : Sorted)
	{
		if (!Presentation)
		{
			continue;
		}

		TSet<FName> Assigned;
		auto ValidateAndCollect = [&Assigned, &CanonicalClasses](const TArray<FName>& Values, const TCHAR* Label, FName SkillId) -> bool
		{
			for (const FName ClassId : Values)
			{
				if (!CanonicalClasses.Contains(ClassId))
				{
					UE_LOG(LogTemp, Error,
						TEXT("[UI-SKILLS-AFFINITY01.3A] %s has unknown %s class %s."),
						*SkillId.ToString(), Label, *ClassId.ToString());
					return false;
				}
				if (Assigned.Contains(ClassId))
				{
					UE_LOG(LogTemp, Error,
						TEXT("[UI-SKILLS-AFFINITY01.3A] %s assigns class %s to more than one affinity group."),
						*SkillId.ToString(), *ClassId.ToString());
					return false;
				}
				Assigned.Add(ClassId);
			}
			return true;
		};

		if (!ValidateAndCollect(Presentation->RecommendedClassIds, TEXT("recommended"), Presentation->SkillId) ||
			!ValidateAndCollect(Presentation->UsefulClassIds, TEXT("useful"), Presentation->SkillId) ||
			!ValidateAndCollect(Presentation->DiscouragedClassIds, TEXT("discouraged"), Presentation->SkillId))
		{
			return 1;
		}

		TArray<FName> Neutral;
		for (const FName ClassId : SkillAffinityAuditCommandlet::CanonicalClassIds)
		{
			if (!Assigned.Contains(ClassId))
			{
				Neutral.Add(ClassId);
			}
		}

		RecommendedCount += Presentation->RecommendedClassIds.Num();
		UsefulCount += Presentation->UsefulClassIds.Num();
		DiscouragedCount += Presentation->DiscouragedClassIds.Num();

		const FString Recommended = SkillAffinityAuditCommandlet::JoinNames(Presentation->RecommendedClassIds);
		const FString Useful = SkillAffinityAuditCommandlet::JoinNames(Presentation->UsefulClassIds);
		const FString Discouraged = SkillAffinityAuditCommandlet::JoinNames(Presentation->DiscouragedClassIds);
		const FString NeutralText = SkillAffinityAuditCommandlet::JoinNames(Neutral);

		Csv += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s\n"),
			*SkillAffinityAuditCommandlet::CsvEscape(Presentation->SkillId.ToString()),
			*SkillAffinityAuditCommandlet::CsvEscape(
				SkillAffinityAuditCommandlet::ResolveDisplayName(Presentation->SkillId, DisplayNames)),
			*SkillAffinityAuditCommandlet::CsvEscape(Recommended),
			*SkillAffinityAuditCommandlet::CsvEscape(Useful),
			*SkillAffinityAuditCommandlet::CsvEscape(Discouraged),
			*SkillAffinityAuditCommandlet::CsvEscape(NeutralText));

		UE_LOG(LogTemp, Display,
			TEXT("[UI-SKILLS-AFFINITY01.3A] %s | Recommended=%s | Useful=%s | Discouraged=%s | Neutral=%s"),
			*Presentation->SkillId.ToString(),
			Recommended.IsEmpty() ? TEXT("-") : *Recommended,
			Useful.IsEmpty() ? TEXT("-") : *Useful,
			Discouraged.IsEmpty() ? TEXT("-") : *Discouraged,
			NeutralText.IsEmpty() ? TEXT("-") : *NeutralText);
	}

	const FString OutputDir = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SkillAffinity"),
		TEXT("UI-SKILLS-AFFINITY01.3A"));
	IFileManager::Get().MakeDirectory(*OutputDir, true);
	const FString OutputPath = FPaths::Combine(OutputDir, TEXT("SkillAffinityMatrix.csv"));

	if (!FFileHelper::SaveStringToFile(
		Csv,
		*OutputPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-SKILLS-AFFINITY01.3A] Failed to write %s."),
			*OutputPath);
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-SKILLS-AFFINITY01.3A] Audit complete. Skills=%d Recommended=%d Useful=%d Discouraged=%d"),
		Catalog->Skills.Num(), RecommendedCount, UsefulCount, DiscouragedCount);
	UE_LOG(LogTemp, Display, TEXT("[UI-SKILLS-AFFINITY01.3A] CSV: %s"), *OutputPath);

	FRPGAuthoringIdentityResolver::ResetRuntimeCache();
	return 0;
}
