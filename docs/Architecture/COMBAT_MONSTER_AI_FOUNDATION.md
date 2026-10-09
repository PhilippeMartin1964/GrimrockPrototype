# Combat, monstres et IA — Fondation d'architecture

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Le SaveGame courant est **v24 exact-match**. Les anciennes mentions v9 dans
> cette fondation sont superseded.

## 1. Autorité combat

`UGridTurnManagerComponent` orchestre le combat déterministe sur grille.

Il porte notamment :

- rencontre active ;
- rounds ;
- initiative globale ;
- combattant actif ;
- états de tour des personnages ;
- tours monstres ;
- PA individuels ;
- PAM du groupe ;
- catalogue d'actions ;
- targeting ;
- exécution / fin de tour ;
- combat log.

Les Widgets ne calculent pas initiative, coûts ou légalité.

## 2. Économie d'action

Ressources possibles :

- PA ;
- PAM ;
- mana ;
- quantité d'item ;
- cooldown.

Le paiement est transactionnel : un refus ne doit pas laisser un paiement
partiel.

Les valeurs d'équilibrage restent des données/règles de domaine, jamais des
constantes dispersées dans UMG.

## 3. Catalogue d'actions

Le catalogue unifie les sources :

```text
Universal
Equipment / MainHand / OffHand / Unarmed
Class / Talent
Quick Item
Spell
```

Les bindings de la barre d'actions ne sérialisent pas une copie de
l'action disponible ; ils conservent une identité résolue contre le catalogue
courant.

## 4. Targeting et résolution

Le ciblage repose sur cellules, arêtes, portée, LOS et blocages de grille.
`GridCombatResolver` applique les résultats gameplay.

La présentation (HUD, animation, audio, VFX, projectile) observe la résolution ;
elle ne la décide pas.

## 5. Combat HUD

`UGridCombatHudWidget` est combat-only :

- 4 panneaux de membres ;
- initiative ;
- PAM ;
- fin du tour ;
- raison de refus ;
- targeting/preview.

`Panel_PartyMembers` et `Panel_CombatBottomRight` appartiennent au contrat
UI-COMBAT-UNIFY02. Navigation globale et action bar appartiennent au
`UGridPersistentHudWidget`.

## 6. Monster definitions

`UGridMonsterDefinitionAsset` porte les données authored d'une famille de
monstre. `AGridMonsterActor` projette l'état vivant et s'appuie sur des
composants spécialisés :

- Movement ;
- Behavior ;
- Combat ;
- Death ;
- Audio ;
- VFX ;
- Idle variation.

Le nombre de familles de production est une question de contenu, pas une raison
de créer une seconde architecture.

## 7. Occupation, pathfinding et perception

La grille reste autoritaire :

- occupation ;
- déplacement ;
- passabilité ;
- pathfinding ;
- LOS ;
- audition traversant les frontières ;
- cellule connue du groupe.

Le NavMesh ne remplace pas ces contrats.

## 8. Exploration AI

Le comportement couvre notamment :

```text
Idle
Alert
Pursuing
Attacking
Hurt
Dead
Patrol
Investigation
Alarm
```

`UGridMonsterBehaviorComponent` réconcilie l'état avec perception et mémoire
de la cible. Un monstre qui entend encore le groupe mais dont le chemin est
temporairement bloqué reste en investigation au lieu de déclencher une recherche
oscillante.

## 9. Encounters

`MonsterSpawn` et les groupes d'encounter gèrent spawn/despawn, vagues et
`StartEncounter`.

L'intention d'engagement passe par le pipeline d'engagement automatique et le
TurnManager, sans seconde autorité de combat.

## 10. Persistance

L'état vivant des monstres est capturé dans le DungeonRuntimeState, y compris
mort, ressources, état de placement, encounter et Status Effects.

Le Save durable est refusé pendant un combat actif lorsqu'un état intermédiaire
ne peut pas être restauré correctement.

Save courant : **v24 exact-match**.

## 11. Event → Command / Quest

Le bus peut recevoir des événements monstre et des commandes encounter. Les
commandes Quest de MON21.3 réutilisent le même dispatcher et délèguent au
`UGridQuestSubsystem`.

Aucun second bus n'est créé.

## 12. Invariants

1. TurnManager = autorité combat.
2. Grille = autorité spatiale.
3. Action refusée = aucune dépense partielle.
4. IA et présentation n'appliquent pas directement une résolution parallèle.
5. Mort/restauration ne doit pas réintroduire occupation/collision.
6. UI combat = projection.
7. Save v24 exact-match.
