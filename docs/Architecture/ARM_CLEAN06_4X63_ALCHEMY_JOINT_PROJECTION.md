# ARM-CLEAN06.4X.63 — projection conjointe des dix objets d'alchimie

Parent : `2497018bbc89616ce8f551e9faf13641677265dc` (X.62, SilentTask + LocalAuto SUCCESS). Le propriétaire a transmis les deux preuves natives le 18 novembre 2026 ; audit code 0, dépôt propre. Rapport privé `D:\GrimrockNativeSnapshots\ARM_CLEAN06_4X62\AlchemyEvidence-20261118-201511678.json`, SHA256 `48b8b1878da3b50376f01b6ea1c64d5e903433d3dd6f38f85221350a81b931ca` (copie reçue). Ne pas recopier le rapport privé complet dans le dépôt.

## État constaté

38 définitions, 19 actions compactes, 4 détaillées, 5 présentations compactes, 1 détaillée. Les dix projections individuelles sont exactes ; aucune collision avec les actions existantes. Oil déjà canonique et aucune différence d'action. Aucun des dix ne possède mesh, icône, attaque équipée, poids non nul, pile >1 ou présentation d'attaque. Aucun référent package direct, mais usages C++ conservés (dont kit Grenadier pour FireBomb). Les quatre élixirs partagent exactement l'ancien ID, chacun avec un StatusApplications distinct. Les actions ont les PA/cooldowns/quantités réels ci-dessous ; pas de changement d'équilibrage.

| ItemId | ActionId cible (préfixe Action_Alchemist_) | PA | Cooldown | Catégorie |
|---|---|---:|---:|---|
| Item_Antidote | Antidote | 1 | 0 | Potion |
| Item_Bomb_Fire | FireBomb | 2 | 0 | Misc |
| Item_Bomb_Toxic | ToxicBomb | 2 | 0 | Misc |
| Item_DefensiveElixir_Fire | DefensiveElixir_Fire | 1 | 0 | Potion |
| Item_DefensiveElixir_Ice | DefensiveElixir_Ice | 1 | 0 | Potion |
| Item_DefensiveElixir_Lightning | DefensiveElixir_Lightning | 1 | 0 | Potion |
| Item_DefensiveElixir_Poison | DefensiveElixir_Poison | 1 | 0 | Potion |
| Item_Flask_Acid | AcidFlask | 2 | 1 | Misc |
| Item_Flask_CorrosiveCloud | CorrosiveCloud | 3 | 2 | Misc |
| Item_Panacea | Panacea | 2 | 3 | Potion |

Chaque source consomme exactement une unité. N0 physique préservé. Les quatre statuts d'élixir : restauration de 4 points d'armure magique et résistance élémentaire +25 % pour 3 rounds. Les textes FText existants sont préservés y compris les clés de localisation ; les nouveaux ActionId ne remplacent pas les DisplayName. La Panacea contient un filtre de retrait explicite Status_Silence, sans package natif correspondant (dix autres statuts explicites valides). La classe Priest le mentionne également dans un filtre de retrait. Aucun asset Silence/Silenced dans le dossier de statuts au parent. Ne pas inventer de statut, ne pas supprimer le filtre dans une migration structurelle : la référence n'est PAS un effet appliqué, elle reste prévue pour un statut éventuel. Documenter cet écart de contenu ; revue séparée de cohérence des statuts avant finalisation du jeu.

## Projection conjointe protégée

`GridEditorAlchemyCutoverPlan.h` : charge l'evidence réelle X.62, verrouille le root 38 et les comptes 19/4/5/1, onze IDs, coûts PA/cooldowns, type, poids, capacité, source QuickItem, absence de présentation native, présence exacte des quatre collisions et Oil déjà résolu. Un seul root transitoire contient les 38 pointeurs ordonnés d'origine et 10 définitions UGridConsumableDefinitionAsset nouvelles. Un catalogue d'actions *transitoire*, un catalogue de présentations *transitoire* ; les deux catalogues d'armement inchangés. Ajout de dix actions détaillées (4 -> 14), aucune nouvelle action compacte, aucune nouvelle présentation. Reprend le plan éprouvé X.62 (copies des propriétés visuelles/physiques etc.) et exige la parité réfléchie intégrale de l'action effective après résolution N0 ; seule mutation autorisée ActionId des quatre élixirs. Aucune clé partagée ancienne dans le registre cible. Liaison au profil existant PRES_AttackNoFX pour les dix (PresentationProfileId de l'action effective reste None). Ne pas transformer ce lien en nouvelle animation ou effet. Refus de destination persistée ou déjà remplie. Le lot n'écrit aucun .uasset.

Tag réservé InventoryCategory.Potion (6 définitions), catégorie Misc inchangée pour les 4 restantes ; le tag s'ajoute uniquement à la définition d'objet, pas à l'action SourceTags originelle. La projection UI explicite conserve l'ancien classement, sans donner de permission de consommer/jeter/équiper à un objet qui n'a pas d'action autorisée. Le validateur refuse les tags de catégorie contradictoires et ces tags sur un ARM. Pas de refonte des catégories des autres objets existants.

## Sauvegardes v27 et identité d'action

`GridEditorAlchemyActionIdentity.h` : helper **Editor-only** de conversion d'une copie FGridPartyInventoryState. Pour ActionId historique partagé, la SourceDefinitionId exacte sélectionne l'un des quatre ActionId cibles ; vérifie SourcePolicy=QuickItem, slot valide, non primaire, cohérence de la liaison. Une identité étrangère/ambiguë ou déjà renommée vers un autre élément est refusée. Parcourt ActiveCharacters et CharacterPool ; copy-first, atomicité de la mutation en mémoire, préservation des indices, GUIDs, autres raccourcis, itemIDs, quantités, données du personnage. Idempotent. Aucune compatibilité runtime par alias, aucun ancien catalogue ajouté, aucun fichier joueur lu/écrit. SaveVersion=27 conservé. Ce helper n'est PAS encore le convertisseur complet de fichiers : il faut implémenter et tester l'opération sur copies réelles, la validation avant/après et la stratégie de bascule avant d'activer les quatre définitions natives. Les cooldowns de base sont 0 pour les quatre ; la preuve de compatibilité des états persistés reste une obligation de cette prochaine étape.

## Automation (à valider sur SHA du lot)

- Grimrock.RPG.ARM.CLEAN06_4X63.JointAlchemyProjection : 48 définitions, 14 détails ; 11 owners alchimie dont Oil ; action exacte champ par champ ; identité unique, absence d'alias partagé, échec de résolution croisée et invalide N21, aucune modification du root original.
- Grimrock.RPG.ARM.CLEAN06_4X63.SavedElixirBindingCopy : quatre variants dans actifs/réserve, comparaison de tous les champs réfléchis avec seules quatre substitutions attendues, idempotence, refus d'identité/politique ambiguë et rollback en mémoire.
- Grimrock.RPG.ARM.CLEAN06_4X63.JointAlchemyHotbar : vrai composant inventaire et GI monde isolé, onze actions assignées par API de production canonique, identité source/action exacte, quantité inchangée à l'affectation.
- Grimrock.RPG.ARM.CLEAN06_4X63.PotionPresentationDoesNotGrantUse : tag explicite et filtre Consumables existant, aucune capacité inventée, conflit refusé. Ce test n'est pas une preuve de rendu PIE.

## Suite réelle (pas de nouveau Go requis)

Attendre les deux validations Windows exact-SHA, puis préparer la transaction native pour les 10 créations, le root et le catalogue d'actions (12 packages), protéger les autres catalogues, les 11 sources anciennes, les statuts, les niveaux et versions de sauvegarde. Adapter les gates historiques à l'état exact 38/48 sans relâcher les invariants ; l'ancienne commande X.62 reste épinglée à 38 ou son test passe par une vue vérifiée. Traiter auparavant le risque SaveGame des quatre identités et le kit Grenadier/authoring. Tester le véritable exécuteur pour effets Ally/Hostile/Area, consommation, scaling, statuts, PA, cooldowns, surfaces ; la projection et l'affectation de hotbar seules ne constituent pas ces validations. L'outil historique Alchemist doit cesser de recréer ou modifier les anciennes sources au cutover, sans supprimer les règles de classe. Tous les assets locaux et saves utilisateur restent untouched dans X.63. Pas de PIE requis pour cette projection seule.

UI/localisation : procédure UI_WBP_LOCALIZATION_WORKFLOW relue. Catégorie et libellés existants, FText source préservés ; aucune nouvelle clé de texte visible, aucun stockage UI supplémentaire, aucune modification UI.csv/ST_UI/WBP/Game-fr/en. Rendu à vérifier au cutover natif.

**X.63 = code de projection et remapping de copies, pas un Apply. Registre persistant 38, 107/107 références X.48. Projection 48/14 seulement transitoire. ARM-CLEAN06.4 partielle, 06.5/06.6 ouvertes.**
