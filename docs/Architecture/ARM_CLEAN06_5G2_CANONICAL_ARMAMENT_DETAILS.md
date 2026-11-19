# ARM-CLEAN06.5G.2 — Panneau Details des armes canoniques

Parent 4d12154b71e651082193e77517ddc5a00965c9ca : deux statuts SUCCESS après détachement natif. Ce lot remplace la personnalisation de l'ancienne classe UGridItemDefinitionAsset (qui recommandait DA_WeaponCatalogue et PlayerAttackPresentationProfile) par celle de UGridArmamentDefinitionAsset. Les six champs éditables restent ceux du schéma normatif, sans ajouter de nouveau pointeur ou de copie N1.

Le résumé N1/N20 lit exclusivement le même DA_ItemSystem configuré que le runtime via un registre transitoire ; un duplicata non enregistré est refusé même si son ItemId existe. Poids, dés, protections, PA et portée ne sont pas recopiés dans les assets. Boutons de navigation vers le registre et ses quatre catalogues existants ; aucune écriture. Les champs du Details panel sont Editor-only, aucun WBP/texte joueur/ST_UI/Game fr/en modifié.

Test Grimrock.Editor.ARM.CLEAN06_5G2.CanonicalArmamentDetails : 22 définitions réelles N1/N20, identité, niveaux, poids, absence de mutation, duplicata non enregistré refusé et sorties effacées. La compile Windows et le contrôle visuel Details restent à faire après publication. Aucun ancien package supprimé par ce lot.
