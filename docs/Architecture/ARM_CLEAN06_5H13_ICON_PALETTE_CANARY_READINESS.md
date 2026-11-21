# ARM-CLEAN06.5H.13 — Préparation du retrait natif isolé de la palette d'actions

**Parent :** `c6af46f579caacb6b655c5af0403b2c2081a5617` (deux statuts Windows SUCCESS).

## Pourquoi un seul canari ?

Le rapport complet du 21.11 indique que `/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons` est encore présent, mais **n'a aucun référent AssetRegistry** (hard, soft ou all). Les tests Editor 06.2 ont déjà été déconnectés de ce package : ils utilisent une palette transiente reconstruite depuis `DA_ItemSystem.WeaponVisualCatalogue.ActionIcons`. Le test H.5F vérifie l'absence d'ancienne UPROPERTY dans le widget et de référent de package.

Ce bon résultat n'autorise pas une suppression via Git ou une purge groupée. Le package doit faire l'objet d'une opération **native Unreal Editor, séparée et sauvegardée**. Les 48 anciennes définitions, les deux anciens catalogues sources, les deux Readables sauvegardés et le redirecteur du couteau restent strictement hors du retrait éventuel de palette.

## Préparation des tests sans changer l'existant

Les garde-fous ne peuvent pas imposer éternellement 54 candidats après le retrait autorisé de la palette. H.13 permet désormais **exactement deux états** pour l'audit :

- **54 candidats :** 48 définitions + 2 catalogues sources + 2 Readables + 1 redirecteur + 1 palette historique ;
- **53 candidats :** la même répartition, **sans la seule palette** (zéro ou un, jamais plus, aucun autre groupe facultatif).

Ce contrat est vérifié dans le test native `CLEAN06_5A.NativeRetirementEvidence`, le script d'audit complet `AuditARM_CLEAN06_5_Retirement.ps1` et le planificateur non destructif `PlanARM_CLEAN06_5H12_Retirement.ps1` ; ce dernier ajoute une fixture synthétique pour les **deux** états 54/53, plus ses refus sur sauvegarde altérée et référent externe.

Les preuves de parité 7/7 et les deux Readables protégés demeurent des conditions obligatoires. Les schémas `retirement-evidence/v1` et `read-only-retirement-plan/v1` ne changent pas ; `deletion_authorized=false` persiste. Aucun `.uasset`, `.umap`, `.sav` ni Editor Utility n'est modifié.

## Avant un éventuel retrait réel

Contrôler toutes les références C++ textuelles et dynamiques, les références AssetRegistry hard/soft et les chemins sérialisés ; sauvegarder le package et le dépôt ; opérer en Unreal Editor natif (pas de suppression brute `git rm`), corriger les redirecteurs, puis exécuter la suite Grimrock et un vrai Cook/Shipping + PIE + Save/Continue. Le rapport transmis a été produit **avant** H.13 ; toute transaction future exige un audit frais sur son SHA exact. Un premier résultat positif n'est pas une approbation automatique.

**H.13 se limite à préparer les gardes pour l'absence future d'un seul asset. Il ne retire pas l'asset.**
