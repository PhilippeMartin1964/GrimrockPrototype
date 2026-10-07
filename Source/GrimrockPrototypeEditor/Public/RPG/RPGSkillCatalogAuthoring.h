#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"

class URPGSkillAsset;

/** Définition Editor-only d'une entrée du catalogue canonique de compétences. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGCanonicalSkillDefinition
{
	FName SkillId = NAME_None;
	FText DisplayName;
	ERPGSkillGoverningAttribute GoverningAttribute = ERPGSkillGoverningAttribute::None;
	bool bAllowUntrainedChecks = true;
};

/**
 * Authoring Editor-only du catalogue Skills de production.
 *
 * Le catalogue provient de RPG_Class_Progression_1_20_v0_1.md.
 * Il ne définit aucune économie de points de compétence et ne remplace jamais
 * FGridCharacterInventoryState::SkillRanks comme autorité des rangs.
 */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGSkillCatalogAuthoring
{
	static const TCHAR* ProductionFolder();

	/** Retourne les 25 compétences canoniques dans l'ordre de la spécification. */
	static void GetCanonicalDefinitions(TArray<FRPGCanonicalSkillDefinition>& OutDefinitions);

	static bool TryGetCanonicalDefinition(FName SkillId, FRPGCanonicalSkillDefinition& OutDefinition);
	static bool ValidateCanonicalCatalog(FString& OutError);

	/**
	 * Applique uniquement les champs définis par le catalogue canonique.
	 * Description et RequirementGrants sont volontairement préservés.
	 */
	static void ConfigureSkill(URPGSkillAsset& SkillAsset, const FRPGCanonicalSkillDefinition& Definition);

	static bool IsCanonicalSkill(const URPGSkillAsset& SkillAsset, const FRPGCanonicalSkillDefinition& Definition);

	/** Crée/met à jour puis sauvegarde les 25 DataAssets de production. */
	static bool AuthorProductionAssets(FString& OutError);
};
