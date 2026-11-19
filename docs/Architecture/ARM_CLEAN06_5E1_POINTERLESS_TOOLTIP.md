# ARM-CLEAN06.5E.1 — Infobulles et lecture indépendantes des anciennes définitions

Parent `59202029bb9fb81403b349ebb5623d446f73fac9` : deux validations Windows SUCCESS. Le retrait de RehydrateOwnedItemDefinitions est prématuré : GetTooltipView quittait avant ses informations métier si l'ancien pointeur manquait ; la projection d'arme et comparaison globale imposaient aussi ce pointeur.

## Cutover
- Les profils ARM N1–N20 et les comparaisons (y compris la vraie source du sac) se résolvent avant toute exigence de définition historique. Les identités non inscrites conservent leur ancien contrat exact.
- Les contributions utilisent les modèles communs 5D.4/5D.5. Les textes de surface et la possibilité de lier une action utilisent le vrai QuickItem canonique et les permissions d'interaction existantes, sans inventer de raccourci.
- La lumière vient du profil canonique et de l'état mutable de l'instance. Le texte par défaut et la capacité de lecture canonique sont partagés par le tooltip, le menu et son exécution ; priorité au texte d'instance, puis au ReadableContentAsset, puis à la définition.
- Les non-ARM physiques N1 sont refusés pour ces nouvelles lectures de capacité. Aucun changement de format SaveGame v27, de WBP ou de package.

## Validation
3AA étendu aux sources absentes ou contradictoires et aux comparaisons sans pointeur ; test 5E1 en monde avec le vrai registre : comparaison épée N20/N1, torche N0, huile N0 et description localisée, refus N1. Contrôles historiques conservés. Deux statuts du SHA exact avant poursuivre.

## UI et localisation
Contrats UI_UMG_PRESENTATION_CONTRACT et UI_WBP_LOCALIZATION_WORKFLOW relus. Pas d'authoring WBP (WBP_ItemTooltip existant), pas de nouveau texte/clé ni de changement de hiérarchie. Les FText DataAsset et clés dynamiques déjà présentes restent les seules autorités (dont Combat.QuickItem.Surface.OilUsage). UI.csv → ST_UI → WBP → Game/fr/en : inchangés, aucune matérialisation ou nouvelle traduction requise ; rendu réel PIE/fr/en non attesté. Ne pas conclure à un retrait global de Rehydrate : les autres consommateurs doivent être examinés séparément avant de supprimer les anciens fichiers.
