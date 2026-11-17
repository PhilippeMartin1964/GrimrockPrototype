# ARM-CLEAN06.4X.53 — Inspecteur de serrure : blocage définitif de l'ajout des anciens pointeurs

**Parent :** `e4537dbc8de1148232fbc89cc9116c0ef6eeac4a` (X.52, deux statuts Windows SUCCESS).
**Portée :** panneau Slate existant, extension du test Editor X.50, documentation seulement. Aucun package Unreal ou SaveGame écrit.

## Défaut et correctif

Dans X.50, `BuildLockBehaviorSection` masquait le picker historique seulement si `Lock.AcceptedKeyIds` était non vide. La suppression du dernier ID réaffichait `Add Accepted Key Asset` et `BuildItemDefinitionAssetPicker`, pouvant réintroduire un `UGridItemDefinitionAsset*` dans un placement désormais canonique.

X.53 remplace tout ce chemin d'édition par **un affichage en lecture seule** des noms d'assets historiques, pour les overrides anciens comme nouveaux, qu'il y ait zéro ou plusieurs IDs. Les commandes d'édition `Accepted Key Ids` et le toggle canonique X.50 restent disponibles. Les instances anciennes à pointeurs ne sont jamais réécrites par la simple ouverture du panneau.

Les tests de `Grimrock.RPG.ARM.CLEAN06_4X50.PointerlessLockOverrideAuthoring` vérifient un override vidé de ses IDs (aucun repli vers les anciens defaults), la conservation d'un override historique, et une lecture des deux véritables définitions de serrure compatible avec leur état natif AVANT (un pointeur cohérent) ou APRÈS (zéro pointeur) conversion. Les invariants de conversion des quatre packages restent contrôlés par X.51/X.52 ; un état partiellement migré demeure interdit.

## Contrôles

- Compilation UE5.5.4 + suite Grimrock via `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`, tous deux requis SUCCESS sur le **SHA exact X.53**.
- Contrôle ultérieur dans l'éditeur : supprimer le dernier `Accepted Key Id` puis vérifier qu'aucun picker/add d'asset ne réapparaît ; vérifier un ancien override en lecture seule.
- Contrat `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relu à l'ouverture et à la clôture : aucun nouveau texte, aucune clé créée ni modifiée. `UI.csv → ST_UI → WBP → Game/fr/en` est inchangé, sans matérialisation de String Table nécessaire.
- La migration des quatre `.uasset` partagés exige toujours une transaction UE native distincte, copies privées SHA256, nouvelle ouverture en second processus UE, tests et recette PIE. **63 pointeurs natifs restants ; zéro migration avec ce code.**

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
