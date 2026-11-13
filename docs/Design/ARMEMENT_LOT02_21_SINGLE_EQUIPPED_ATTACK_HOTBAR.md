# ARMEMENT-LOT02.21 — Une seule attaque normale contextuelle par configuration équipée

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/tests historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


> **Évolution 02.19.2.2 (10.11.2026) :** la règle d'une seule attaque contextuelle sur le raccourci 1 est inchangée. Ses **impacts et sons** peuvent désormais être séquencés si l'action a plusieurs résolutions, selon [ARMEMENT-LOT02.19.2.2](ARMEMENT_LOT02_19_2_2_SEQUENCED_COMBAT_PRESENTATION.md). Aucun bouton MainHand/OffHand supplémentaire n'est réintroduit.

**Décision utilisateur du 10.11.2026.** Ce document annule uniquement l'ancienne projection automatique des coups simples MainHand/OffHand dans des raccourcis secondaires (jalons 02.19.1 et 02.19.2). Il conserve les données, les résultats de combat et les icônes validées.

## Règle d'interface

**Le personnage n'a qu'une seule action d'attaque normale visible automatiquement dans la barre, sur le raccourci 1 (`PrimaryAttack`).** Cette action est déduite des armes effectivement équipées, de leurs actions réellement autorisées et, le moment venu, de leurs prérequis. Les emplacements 2+ restent libres pour les raccourcis personnalisés de sorts mémorisés, capacités, objets et actions spéciales.

| Équipement autoritaire | Raccourci 1 | Projection automatique en 2+ |
| --- | --- | --- |
| Deux armes de mêlée à une main éligibles | **Attaque double** (`Attack_DualWield`), 3 PA | **Aucune** attaque simple de dague ou Main-gauche |
| Une arme offensive en MainHand, OffHand vide | Attaque de cette arme | Aucune |
| MainHand offensive + bouclier/orbe passif | Attaque MainHand | Aucune attaque/defense supplémentaire fictive |
| MainHand seule, arme à deux mains/hast/arc/arbalète/focaliseur | Attaque normale propre à l'arme si le catalogue la propose | Aucune copie d'attaque |
| MainHand vide, arme offensive autorisée en OffHand | Attaque OffHand | Aucune |
| Aucune attaque d'arme équipée | Mains nues (`Attack_Unarmed`) | Aucune |

Le **Voleur/Ombre N1** possède provisoirement l'aptitude à utiliser l'attaque double avec ARM001 MainHand et ARM013 OffHand. Cette règle reste générale pour les classes équipées de deux armes réellement compatibles ; **aucun nouveau talent** ni suppression de Requirement RPG n'est introduit.

Une action spéciale (parade, blocage, coup de bouclier, etc.) pourra bénéficier d'un raccourci distinct **lorsque sa mécanique est réellement authorée et disponible**. Elle ne doit pas être confondue avec une seconde *attaque normale*.

## Simplification du code

- `FGridCombatHudViewModelBuilder::ProjectPrimaryEquippedAttack` conserve **uniquement** la projection de l'alias `PrimaryAttack` (priorité : double, MainHand, OffHand, mains nues). Son ancien nom `ProjectEquippedHandAttacks` et sa boucle de suggestion des coups simples ont été supprimés.
- Les champs UI `bTransientMainHandSuggestion` et `bTransientOffHandSuggestion`, le traitement spécial clic droit / drag/drop et les mentions de tooltip propres à ces suggestions ont été supprimés. Moins de branches UI et aucun binding créé automatiquement.
- `BuildHotbarActions` continue de charger les affectations réellement sauvegardées. **Les raccourcis explicitement créés par le joueur restent respectés** (y compris une ancienne attaque simple épinglée avant ce changement) : ils ne sont ni effacés ni migrés silencieusement. Il s'agit d'un choix explicite, pas d'une action normale ajoutée automatiquement.
- Le catalogue autoritaire `GetAvailableCombatActions` conserve les deux sources offensives MainHand/OffHand. **Elles sont nécessaires au moteur de l'attaque double** pour résoudre les deux jets, dégâts, effets, résistances, coûts et réactions ; les supprimer casserait le combat et n'est pas requis pour simplifier le HUD.
- Le système `UGridEquippedWeaponActionIconPalette` (02.20) demeure inchangé. Le raccourci 1 utilise sa catégorie et son icône dédiée ; `Image_ActionIcon` reste l'unique widget d'image.
- Aucune modification de PA, d'actions de classe, de compétences, de SaveGame, du pipeline d'armement ou des DataAssets.

## Tests

Le test historique `Grimrock.RPG.ARMEMENT.LOT02.19.1.HandAttackProjection` est **actualisé** : il vérifie désormais qu'une attaque primaire unique est affichée en priorité, qu'aucune suggestion ne remplit les raccourcis 2+ et que les anciens bindings explicites restent intacts.

`Grimrock.RPG.ARMEMENT.LOT02.19.1.ShadowN1OffHandCombat` couvre les véritables ARM001/ARM013 et l'attaque combinée, le non-affichage des attaques simples, le coût 3 PA, ainsi que l'incompatibilité bouclier/deux mains.

`Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost` conserve la vérification des deux résolutions et du coût unique et ajoute l'absence des raccourcis automatiques en 2 et 3.

Après push : compilation UE5.5.4 + suite automatisée `Grimrock` sur le SHA exact, avec les deux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`. Test graphique PIE séparé.

## Contrat de localisation et d'UI

`docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relus en début de jalon.

- Aucun changement du Designer, `WBP_GridCombatHudAction.uasset`, aucun widget ajouté ni supprimé.
- Aucun nouveau texte joueur, clé ST_UI ou DataAsset : le tooltip utilise exclusivement les clés déjà existantes. Le hint « supprimer le raccourci » n'est plus montré pour le raccourci 1 protégé ; les autres bindings personnalisés continuent de l'afficher.
- **Chaîne : `UI.csv` inchangé → `ST_UI.uasset` inchangé → WBP inchangé → `Game/fr/en` inchangé pour ce jalon**. Aucun nouvel authoring de traduction requis, et l'audit anglais des textes historiques reste distinct.

## Recette PIE d'acceptation

1. Voleur/Ombre, ARM001 + ARM013 : **un seul raccourci d'attaque de base**, le 1 avec l'icône d'attaque double. Les cases 2 et 3 sont vides en l'absence de personnalisation.
2. Exécuter 1 : une attaque combinée coûtant 3 PA, deux frappes distinctes dans les résultats (leur séquencement temporel/audio relève toujours du jalon 02.19.2.2).
3. Équiper une seule arme, un bouclier passif, une arme à deux mains, une arme de jet ou une arme magique : toujours **une seule action normale projetée** sans boutons de main indépendants.
4. Faire glisser une capacité, un sort ou un consommable sur les raccourcis libres : ils ne doivent jamais être remplacés par le système d'armement. Vérifier la conservation après Save/Continue.
5. Contrôler les anciens raccourcis **explicitement personnalisés** de sources d'armes : ne pas supprimer leurs bindings pendant cette migration. Ils restent des choix de l'utilisateur, et non des suggestions d'attaque automatique.
6. Vérifier l'icône de famille réelle provenant de `DA_EquippedWeaponActionIcons`, sans nouvel élément UMG.

**Hors périmètre :** la séquence temporelle des deux impacts, leurs sons et animations (02.19.2.2), la création réelle d'une parade/blocage (02.19.3), l'évolution future de l'arbre de compétences.
