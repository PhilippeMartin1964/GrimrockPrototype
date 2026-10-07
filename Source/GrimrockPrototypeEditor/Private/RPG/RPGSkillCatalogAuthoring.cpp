#include "RPG/RPGSkillCatalogAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGSkillAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGSkillCatalogAuthoringPrivate
{
	constexpr int32 CanonicalSkillCount = 25;
	constexpr int32 CanonicalMaxRank = 5;
	const TCHAR* ProductionFolderPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills");

	void AddDefinition(
		TArray<FRPGCanonicalSkillDefinition>& OutDefinitions,
		const TCHAR* SkillId,
		const TCHAR* DisplayName,
		ERPGSkillGoverningAttribute GoverningAttribute,
		bool bAllowUntrainedChecks)
	{
		FRPGCanonicalSkillDefinition Definition;
		Definition.SkillId = FName(SkillId);
		Definition.DisplayName = FText::FromString(DisplayName);
		Definition.GoverningAttribute = GoverningAttribute;
		Definition.bAllowUntrainedChecks = bAllowUntrainedChecks;
		OutDefinitions.Add(MoveTemp(Definition));
	}

	FString MakeAssetName(FName SkillId)
	{
		return FString::Printf(TEXT("DA_%s"), *SkillId.ToString());
	}

	URPGSkillAsset* FindOrCreateSkillAsset(const FRPGCanonicalSkillDefinition& Definition, FString& OutError)
	{
		const FString AssetName = MakeAssetName(Definition.SkillId);
		const FString PackageName = FString::Printf(
			TEXT("%s/%s"),
			RPGSkillCatalogAuthoringPrivate::ProductionFolderPath,
			*AssetName);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

		if (URPGSkillAsset* Existing = LoadObject<URPGSkillAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Impossible de créer le package '%s'."), *PackageName);
			return nullptr;
		}

		URPGSkillAsset* Created = NewObject<URPGSkillAsset>(
			Package,
			FName(*AssetName),
			RF_Public | RF_Standalone);
		if (!Created)
		{
			OutError = FString::Printf(TEXT("Impossible de créer l'asset '%s'."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}

	bool SaveSkillAsset(URPGSkillAsset& SkillAsset, FString& OutError)
	{
		UPackage* Package = SkillAsset.GetOutermost();
		if (!Package)
		{
			OutError = FString::Printf(TEXT("La compétence '%s' ne possède aucun package."), *GetNameSafe(&SkillAsset));
			return false;
		}

		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, &SkillAsset, *Filename, SaveArgs))
		{
			OutError = FString::Printf(TEXT("Impossible de sauvegarder '%s'."), *Filename);
			return false;
		}
		return true;
	}
}

const TCHAR* FRPGSkillCatalogAuthoring::ProductionFolder()
{
	return RPGSkillCatalogAuthoringPrivate::ProductionFolderPath;
}

void FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(TArray<FRPGCanonicalSkillDefinition>& OutDefinitions)
{
	using namespace RPGSkillCatalogAuthoringPrivate;

	OutDefinitions.Reset(CanonicalSkillCount);
	AddDefinition(OutDefinitions, TEXT("Skill_HeavyWeapons"), TEXT("Armes lourdes"), ERPGSkillGoverningAttribute::Strength, true);
	AddDefinition(OutDefinitions, TEXT("Skill_LightWeapons"), TEXT("Armes légères"), ERPGSkillGoverningAttribute::Dexterity, true);
	AddDefinition(OutDefinitions, TEXT("Skill_RangedWeapons"), TEXT("Armes à distance"), ERPGSkillGoverningAttribute::Dexterity, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Throwing"), TEXT("Lancer"), ERPGSkillGoverningAttribute::Dexterity, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Shield"), TEXT("Bouclier"), ERPGSkillGoverningAttribute::Constitution, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Armor"), TEXT("Armures"), ERPGSkillGoverningAttribute::Constitution, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Athletics"), TEXT("Athlétisme"), ERPGSkillGoverningAttribute::Strength, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Acrobatics"), TEXT("Acrobatie"), ERPGSkillGoverningAttribute::Dexterity, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Perception"), TEXT("Perception"), ERPGSkillGoverningAttribute::Wisdom, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Survival"), TEXT("Survie"), ERPGSkillGoverningAttribute::Wisdom, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Stealth"), TEXT("Discrétion"), ERPGSkillGoverningAttribute::Dexterity, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Lockpicking"), TEXT("Crochetage"), ERPGSkillGoverningAttribute::Dexterity, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Traps"), TEXT("Pièges / désamorçage"), ERPGSkillGoverningAttribute::Intelligence, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Mechanics"), TEXT("Mécanique"), ERPGSkillGoverningAttribute::Intelligence, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Crafting"), TEXT("Artisanat"), ERPGSkillGoverningAttribute::Intelligence, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Alchemy"), TEXT("Alchimie"), ERPGSkillGoverningAttribute::Intelligence, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Arcana"), TEXT("Arcane"), ERPGSkillGoverningAttribute::Intelligence, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Runes"), TEXT("Runes"), ERPGSkillGoverningAttribute::Intelligence, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Religion"), TEXT("Religion"), ERPGSkillGoverningAttribute::Wisdom, false);
	AddDefinition(OutDefinitions, TEXT("Skill_Nature"), TEXT("Nature"), ERPGSkillGoverningAttribute::Wisdom, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Medicine"), TEXT("Médecine"), ERPGSkillGoverningAttribute::Wisdom, true);
	AddDefinition(OutDefinitions, TEXT("Skill_History"), TEXT("Histoire"), ERPGSkillGoverningAttribute::Intelligence, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Persuasion"), TEXT("Persuasion"), ERPGSkillGoverningAttribute::Charisma, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Intimidation"), TEXT("Intimidation"), ERPGSkillGoverningAttribute::Charisma, true);
	AddDefinition(OutDefinitions, TEXT("Skill_Deception"), TEXT("Tromperie"), ERPGSkillGoverningAttribute::Charisma, true);
}

bool FRPGSkillCatalogAuthoring::TryGetCanonicalDefinition(FName SkillId, FRPGCanonicalSkillDefinition& OutDefinition)
{
	OutDefinition = FRPGCanonicalSkillDefinition();
	if (SkillId.IsNone())
	{
		return false;
	}

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	GetCanonicalDefinitions(Definitions);
	const FRPGCanonicalSkillDefinition* Found = Definitions.FindByPredicate(
		[SkillId](const FRPGCanonicalSkillDefinition& Definition)
		{
			return Definition.SkillId == SkillId;
		});
	if (!Found)
	{
		return false;
	}

	OutDefinition = *Found;
	return true;
}

bool FRPGSkillCatalogAuthoring::ValidateCanonicalCatalog(FString& OutError)
{
	OutError.Reset();

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	GetCanonicalDefinitions(Definitions);
	if (Definitions.Num() != RPGSkillCatalogAuthoringPrivate::CanonicalSkillCount)
	{
		OutError = FString::Printf(
			TEXT("Le catalogue doit contenir exactement %d compétences, mais en contient %d."),
			RPGSkillCatalogAuthoringPrivate::CanonicalSkillCount,
			Definitions.Num());
		return false;
	}

	TSet<FName> SeenSkillIds;
	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		if (Definition.SkillId.IsNone() ||
			Definition.DisplayName.IsEmpty() ||
			Definition.GoverningAttribute == ERPGSkillGoverningAttribute::None ||
			!Definition.SkillId.ToString().StartsWith(TEXT("Skill_")) ||
			SeenSkillIds.Contains(Definition.SkillId))
		{
			OutError = FString::Printf(
				TEXT("Entrée canonique invalide ou dupliquée : '%s'."),
				*Definition.SkillId.ToString());
			return false;
		}
		SeenSkillIds.Add(Definition.SkillId);
	}
	return true;
}

void FRPGSkillCatalogAuthoring::ConfigureSkill(
	URPGSkillAsset& SkillAsset,
	const FRPGCanonicalSkillDefinition& Definition)
{
	SkillAsset.SkillId = Definition.SkillId;
	SkillAsset.DisplayName = Definition.DisplayName;
	SkillAsset.GoverningAttribute = Definition.GoverningAttribute;
	SkillAsset.MaxRank = RPGSkillCatalogAuthoringPrivate::CanonicalMaxRank;
	SkillAsset.bAllowUntrainedChecks = Definition.bAllowUntrainedChecks;

	// Description et RequirementGrants appartiennent à des règles métier distinctes.
	// Un rerun de l'authoring ne doit jamais les effacer.
}

bool FRPGSkillCatalogAuthoring::IsCanonicalSkill(
	const URPGSkillAsset& SkillAsset,
	const FRPGCanonicalSkillDefinition& Definition)
{
	return SkillAsset.SkillId == Definition.SkillId &&
		SkillAsset.DisplayName.EqualTo(Definition.DisplayName) &&
		SkillAsset.GoverningAttribute == Definition.GoverningAttribute &&
		SkillAsset.MaxRank == RPGSkillCatalogAuthoringPrivate::CanonicalMaxRank &&
		SkillAsset.bAllowUntrainedChecks == Definition.bAllowUntrainedChecks;
}

bool FRPGSkillCatalogAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();
	if (!ValidateCanonicalCatalog(OutError))
	{
		return false;
	}

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	GetCanonicalDefinitions(Definitions);
	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		URPGSkillAsset* SkillAsset = RPGSkillCatalogAuthoringPrivate::FindOrCreateSkillAsset(Definition, OutError);
		if (!IsValid(SkillAsset))
		{
			return false;
		}

		SkillAsset->Modify();
		ConfigureSkill(*SkillAsset, Definition);
		if (!IsCanonicalSkill(*SkillAsset, Definition) || !SkillAsset->IsValidDefinition())
		{
			OutError = FString::Printf(
				TEXT("La compétence matérialisée '%s' est invalide."),
				*Definition.SkillId.ToString());
			return false;
		}

		if (!RPGSkillCatalogAuthoringPrivate::SaveSkillAsset(*SkillAsset, OutError))
		{
			return false;
		}
	}
	return true;
}
