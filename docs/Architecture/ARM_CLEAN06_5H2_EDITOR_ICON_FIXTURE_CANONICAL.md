# ARM-CLEAN06.5H.2 — Découplage des tests Editor de la palette historique

**Parent :** `11628391943503fa3f037d47183a55f0a5fe8017`, validé SilentTask et LocalAuto. Cette évolution ne comporte aucun asset ou sauvegarde.

## Objectif et périmètre

Les huit tests de la famille 06.2, `062K/P/Q/U/V/W/X/Y`, chargeaient encore directement le DataAsset historique `DA_EquippedWeaponActionIcons`. Même si le HUD a déjà été converti, ce chemin de test restait un obstacle avant toute suppression native de la palette.

`GridEditorCanonicalIconFixture.h` charge uniquement le vrai root `DA_ItemSystem`, vérifie son `WeaponVisualCatalogue`, ses **12 catégories d'icônes** et l'absence de `None`. La fixture copie cette map canonique vers un objet `UGridEquippedWeaponActionIconPalette` **transient**, retenu temporairement par `TStrongObjectPtr` pendant chaque test. Il ne lit jamais le package historique et n'utilise pas `SavePackage`.

Le reste des tests (invariants, préflight, répétitions refusées, contrôles de non-modification et comparaisons) demeure inchangé. Le test `DetachedIconPaletteSchema` reste le contrôle de référence de l'ancienne liaison WBP.

## Portée réelle de la preuve

Le test de projection historique alimenté par la fixture **ne prouve plus la parité indépendante entre la palette d'origine et la destination** ; il prouve seulement la compatibilité du code de projection avec une entrée issue du canonique. Les preuves historiques de migration déjà archivées continuent d'exister ; ne pas requalifier cette fixture en nouvel audit de la source originale.

La classe C++ `UGridEquippedWeaponActionIconPalette` et les helpers de préflight sont encore présents pour les tests transitoires. La suppression de ces types/fonctions relève d'un lot distinct, après traitement des autres consumers d'assets historiques.

**Interdits** : suppression de `DA_EquippedWeaponActionIcons.uasset`, des catalogues `DA_WeaponCatalogue` et `DA_WeaponVisualCatalogue`, du redirecteur ThrowingKnife ou des deux Readables contenus dans une sauvegarde v27 ; modification des `.sav`, des WBP, des niveaux ou du texte visible.

## Validation

Attendre `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur **le nouveau SHA**, puis poursuivre le recensement des chargements des 48 définitions historiques avant toute migration de packages via Unreal Editor. Aucun effacement autorisé sur la base du rapport `5A` seul.
