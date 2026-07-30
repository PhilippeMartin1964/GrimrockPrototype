# MON11.2 — Résolution des attaques du groupe

## Périmètre

MON11.2 étend la requête MON11.1 en une commande synchrone et atomique :
validation, résolution déterministe, journalisation, application au monstre et
notification du résultat. Le jalon utilise une attaque provisoire à mains nues.

Il ne crée aucun profil d’arme, asset, montage, son, VFX ou feedback visuel du
personnage.

## Architecture synchrone et atomique

`UGridTurnManagerComponent` conserve la responsabilité de la commande. Une
attaque acceptée suit l’ordre suivant :

1. validations de ciblage MON11.1 ;
2. construction des statistiques ;
3. résolution pure avec `FGridCombatResolver` ;
4. diffusion de `OnPlayerAttackRequested` ;
5. ajout de `AttackHit` ou `AttackMiss` au journal ;
6. application unique par `AGridMonsterActor::ApplyAttackResult()` ;
7. diffusion de `OnPlayerAttackResolved` ;
8. victoire éventuelle après le delegate résolu.

Un refus ne consomme ni action, ni tirage aléatoire, ne produit aucun delegate
d’acceptation et ne modifie pas le monstre.

## Attaque provisoire Attack_Unarmed

Le profil C++ transitoire est :

| Donnée | Valeur |
| --- | --- |
| `AttackId` | `Attack_Unarmed` |
| Type | `Physical` |
| Sous-type | `Bludgeoning` |
| Dégâts | 1 à 3 |
| Bonus de précision | 0 |
| Portée | 1 cellule |

`FGridPlayerAttackRequest::IsValid()` exige désormais un `AttackId` non vide.
Ce profil sera remplacé par les armes et l’équipement offensif dans MON11.3.

## Statistiques source

Le TurnManager appelle
`UGridPartyInventoryComponent::GetCharacterSummary()`. Ce résumé contient les
attributs finaux après bonus généraux d’équipement.

- `Accuracy` vient de `Summary.DerivedStats.Accuracy`.
- `DamageBonus` est le modificateur de
  `Summary.Attributes.Strength`, calculé par
  `URPGCharacterRulesLibrary::GetAttributeModifier()`.

Aucun emplacement `MainHand` ou `OffHand` et aucun profil offensif d’objet ne
sont consultés.

## Statistiques cible

La cible est construite depuis l’état runtime du monstre :

- évasion depuis `MonsterDefinition->Evasion` ;
- PV et armures depuis l’Actor ;
- résistance provisoire égale à zéro ;
- multiplicateur obtenu par
  `GetDamageMultiplier(Physical, Bludgeoning)`.

Une définition de monstre absente produit `TargetInactive` avant tout tirage ou
consommation d’action.

## Flux aléatoire de rencontre

`FGridCombatResolver::ResolveAttack()` reçoit directement
`CombatRandomStream`, déjà initialisé avec la graine stable de la rencontre.
Le flux n’est jamais réinitialisé par attaque. `RequestId`, créé avec
`FGuid::NewGuid()`, n’est jamais utilisé comme graine.

Le resolver reste pur : il calcule un `FGridAttackResult` sans modifier Actor,
inventaire ou phase.

## Application du résultat

Le résultat est appliqué exactement une fois avec
`AGridMonsterActor::ApplyAttackResult()`. Cette autorité existante retire
l’armure physique avant les PV, conserve l’armure magique pour une attaque
physique et déclenche l’état Hurt ou la mort logique.

Un miss reste une résolution acceptée : l’action est consommée, le résultat est
diffusé, mais le monstre ne change pas.

## Attaque, mort et victoire

La mort peut diffuser `OnMonsterDied` pendant l’appel à
`ApplyAttackResult()`. Pour le dernier monstre, le TurnManager diffère seulement
la clôture de victoire jusqu’après `OnPlayerAttackResolved`.

L’ordre du journal est :

1. `AttackHit` ;
2. `MonsterDefeated` ;
3. `Victory`.

L’ordre des événements est :

1. `OnPlayerAttackRequested` ;
2. application et mort logique éventuelle ;
3. `OnPlayerAttackResolved` ;
4. `OnCombatEnded(Victory)`.

Le composant de mort existant conserve la libération d’occupation, le loot, les
liens `MonsterDied` et ses gardes anti-doublon.

## Journal structuré

`FGridCombatLogFormatter::FormatPlayerAttack()` produit un texte français
localisable symétrique aux attaques de monstres. L’entrée conserve les
identités stables, les noms affichés, `Attack_Unarmed`, le résultat complet et
l’état vaincu de la cible.

L’entrée d’attaque est ajoutée avant l’application pour garantir l’ordre avec
`MonsterDefeated` et `Victory`. Les textes et le comportement des attaques de
monstres sont inchangés.

## Delegates

- `OnPlayerAttackRequested` signale la commande acceptée avant application.
- `OnPlayerAttackResolved` fournit la requête, le monstre cible et le
  `FGridAttackResult` après application logique.

Chaque delegate est diffusé exactement une fois par résolution acceptée. Les
compteurs et gardes sont transitoires et non sauvegardés.

## Diagnostic NumPad 7

En build non Shipping, NumPad 7 affiche l’acceptation, la raison de refus,
l’attaquant, `Attack_Unarmed`, la cible, les jets, la défense, le hit, le
critique, les dégâts d’armures et de PV, les PV avant/après et la défaite de la
cible.

Les bindings NumPad 1 à 6 et le clic gauche restent inchangés.

## Tests automatisés

Le filtre `Grimrock.Monsters.MON11` contient huit scénarios :

- ciblage et validations MON11.1 ;
- requête acceptée et appliquée ;
- barrière d’action par personnage ;
- mapping des statistiques ;
- déterminisme du flux de rencontre ;
- armure, miss naturel et critique ;
- mort logique et victoire différée.

Les tests utilisent des mondes, grilles, personnages et monstres transitoires,
sans asset `Content/`, rendu, audio ou GPU.

## Procédure PIE

1. Ouvrir une scène existante avec un Rat géant sans modifier la carte.
2. Placer le groupe face au Rat dans la cellule adjacente.
3. Démarrer le combat avec NumPad 1 et attendre `PlayerPhase`.
4. Noter les PV et armures, puis appuyer sur NumPad 7.
5. Vérifier `Attack_Unarmed` et le résultat complet.
6. Pour un miss, vérifier l’absence de mutation.
7. Pour un hit, vérifier armure physique puis PV.
8. Vérifier `AttackerAlreadyActed` au second essai du même personnage.
9. Sélectionner un autre personnage, puis vérifier son attaque.
10. Terminer la phase et vérifier le renouvellement à la phase suivante.
11. Continuer jusqu’à la mort du Rat.
12. Vérifier l’ordre `AttackHit`, `MonsterDefeated`, `Victory`.
13. Vérifier que loot et liens de mort ne s’exécutent qu’une fois.
14. Après la victoire, vérifier le refus `CombatInactive`.

Les réactions Hurt et Death existantes du monstre peuvent se déclencher. Aucune
présentation d’attaque du personnage n’est attendue.

## Éléments différés

- MON11.3 remplace le profil provisoire par l’équipement offensif.
- `Attack_Unarmed` reste le fallback lorsqu’aucune arme offensive n’est
  sélectionnée.
- MON11.4 ajoutera présentation, audio, VFX et feedback complet du personnage.
