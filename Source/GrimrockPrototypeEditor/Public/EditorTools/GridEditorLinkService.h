#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

class AGridLevelEditorActor;
class UGridLevelAsset;

/**
 * Service unique de mutation des connecteurs de l'éditeur.
 *
 * MON19.2.1B impose une identité persistante exacte : les champs de condition
 * font partie du lien et deux variantes conditionnelles partageant le même
 * quadruplet source/event/cible/commande doivent pouvoir coexister.
 */
namespace GridEditorLinkService
{
	/** Normalise les champs inutilisés d'un nouveau lien avant son stockage. */
	GRIMROCKPROTOTYPEEDITOR_API FGridObjectLink NormalizeLink(const FGridObjectLink& Link);

	/** Vérifie uniquement les paramètres requis par la condition sélectionnée. */
	GRIMROCKPROTOTYPEEDITOR_API bool IsConditionConfigurationValid(const FGridObjectLink& Link);

	/** Vérifie source, event, cible, commande, condition et paramètres. */
	GRIMROCKPROTOTYPEEDITOR_API bool IsLinkSupported(const UGridLevelAsset& LevelAsset, const FGridObjectLink& Link);

	/** Comparaison strictement exacte : aucune normalisation implicite. */
	GRIMROCKPROTOTYPEEDITOR_API bool ContainsExactLink(const TArray<FGridObjectLink>& Links, const FGridObjectLink& Link);

	/** Normalise le nouveau lien, puis l'ajoute s'il n'existe pas exactement. */
	GRIMROCKPROTOTYPEEDITOR_API bool AddExactLink(TArray<FGridObjectLink>& Links, const FGridObjectLink& Link);

	/** Supprime une seule variante strictement exacte ; retourne 0 ou 1. */
	GRIMROCKPROTOTYPEEDITOR_API int32 RemoveExactLink(TArray<FGridObjectLink>& Links, const FGridObjectLink& Link);

	/** Mutation de niveau centralisée pour l'acteur éditeur. */
	GRIMROCKPROTOTYPEEDITOR_API bool CreateLink(AGridLevelEditorActor& EditorActor, const FGridObjectLink& Link);

	/** Suppression exacte, y compris pour un lien devenu invalide. */
	GRIMROCKPROTOTYPEEDITOR_API bool RemoveExactLink(AGridLevelEditorActor& EditorActor, const FGridObjectLink& Link);
}
