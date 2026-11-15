# ARM-CLEAN06.4C — Qualification privée des divergences et verrou de conversion

**Parent :** 06.4B `e2381de8b516bf826d7568c20bed943fca929413`,
trois checks Windows SUCCESS. Preuve réelle : issue
https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/135.

## Constat prouvé par 06.4B

| Contrôle | Résultat |
|---|---:|
| Nombre de packages ajoutés/supprimés | 0 |
| Types/classes des 56 assets historiques | 0 changement |
| Propriétés UPROPERTY sérialisées différentes | 8 |
| Graphes de dépendances différents | 4 |
| Référents AssetRegistry différents | 0 |
| Binaires historiques dont le SHA-256 change | 4 |
| Profils ARM N1–N20 dont les valeurs changent | 0 / 1640 |
| Packages historiques affectés | 4 |

Ce tableau NE démontre PAS l'équivalence des 56 assets avec la nouvelle
architecture `DA_ItemSystem`. Les valeurs concrètes différentes
sont conservées exclusivement dans le rapport privé de l'agent.

## Qualification 06.4C

Le script `Scripts/QualifyARM_CLEAN06_4C_Deltas.ps1` trouve
un seul `comparison-private.json` correspondant au SHA 06.4B,
vérifie ses identifiants 06.0/06.4A, son schéma, ses compteurs et
les empreintes des deux manifestes telles que publiées dans les
issues 84 et 134. Si une preuve diffère, le traitement échoue
avant de générer un plan.

Pour chaque différence réellement constatée, il prépare dans
`Runs/.../ARM-CLEAN06.4C` un rapport privé avec :

- package UE et domaine (arme/catalogue, item, grille, donjon, palette) ;
- chaque propriété, son ancienne et sa nouvelle valeur exportées UE ;
- domaine de triage **heuristique** : identité/niveau, balistique/
  projectile, gameplay, visuel/audio, contenu de niveau, métadonnées
  d'item ou inconnu ; ce tri n'est jamais une preuve de migration ;
- graphes de dépendances anciens/nouveaux, empreintes binaires
  anciennes/nouvelles ; chaque entrée conserve un statut
  `REVIEW_REQUIRED`.

Les seuls chiffres autorisés dans l'issue
`[ARM-CLEAN06.4C]` sont les totaux de ces domaines de tri.
Il est interdit d'y publier un `ItemId`, nom d'asset, chemin
personnel, valeur métier, texte localisé, SaveGame ou ancienne/
nouvelle référence précise.

## Verrou explicite avant la conversion

Le second artefact privé `conversion-gate-private.json` garde :

- `status = BLOCKED_PENDING_EXACT_VALUE_AND_REFERENCE_REVIEW` ;
- `mode = DRY_RUN_PLANNING_ONLY` ;
- listes **vides** de fichiers autorisés à écrire, à supprimer
  et de sauvegardes autorisées à changer ;
- preuve de parité ItemId/ItemLevel/GUID, vitesse et arcs de jet,
  paramètres de projectiles, poids, action PA, visuels et clés ;
- future conversion Editor-only avec backups, liste de fichiers
  fermée, arrêt sur divergence puis vérifications
  `AssetRegistry`, Save/Continue et PIE ;
- aucun contournement du vrai Unreal Editor pour des assets binaires.

**IMPORTANT :** un statut `UE5.5.4/ARM-CLEAN06.4C=success`
signifie seulement « toutes les divergences ont été classées dans
un rapport privé » ; cela ne valide pas les modifications,
n'autorise aucun `.uasset` écrit et ne démontre pas zéro legacy.

## Exécution et suite

Le commit dont le sujet exact est
`chore(ARM-CLEAN06.4C): classify private UE parity deltas without writes`
déclenche ce contrôle ponctuel après la compilation et toute la
suite `Grimrock`. Trois checks distincts sont exigés :
`UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
`UE5.5.4/ARM-CLEAN06.4C`, sur le SHA final exact.

**06.4D** : exploiter le classement, établir un mapping explicite
des champs et références et ajouter un simulateur Editor-only
**dry-run** sur les vrais assets, sans `SavePackage` initial.
N'autoriser la migration par écriture qu'après son rapport de
parité et une sauvegarde sûre des sources. 06.5 retire le legacy
uniquement après preuves, 06.6 couvre PIE/Chaos/Cook/Save.

Aucune nouvelle chaîne UI n'est créée ; le cycle
`UI.csv → ST_UI.uasset → WBP → Game/fr/en` reste inchangé.
