# MON10.3 — Infrastructure VFX des monstres et du combat

## 1. Objectif

MON10.3 fournit une infrastructure C++ modulaire, orientée données et
entièrement optionnelle pour les VFX de combat des monstres. Aucun Niagara
System n’est créé, importé ou requis par le jalon.

## 2. Architecture

- `GridMonsterVFXTypes.h` définit les événements, données, requêtes et le
  sélecteur déterministe.
- `GridMonsterVFXComponent.h/.cpp` sélectionne, charge, signale et produit
  éventuellement les effets.
- `GridMonsterDefinitionAsset` porte Alert, Hurt et Death.
- `FGridMonsterAttackDefinition` porte Attack, ImpactHit et ImpactMiss.
- Le TurnManager et les composants de combat/mort appellent uniquement les
  événements déjà validés par le gameplay.

## 3. Événements VFX

Les six événements sont `Alert`, `Attack`, `ImpactHit`, `ImpactMiss`, `Hurt` et
`Death`. MON10.3 ne crée aucun événement Idle VFX.

## 4. Définitions orientées données

`FGridMonsterVFXEventDefinition` contient des références souples vers plusieurs
Niagara Systems, le choix attaché ou monde, un socket, un offset, une rotation,
une échelle et un cooldown. Une définition vide est valide et produit
simplement `false`, sans avertissement.

La validation ne charge aucun asset. Elle refuse une entrée explicitement
vide, les nombres non finis, une composante d’échelle non strictement positive
et un cooldown négatif.

## 5. Compatibilité de `ImpactVFX`

Le champ historique `TSoftObjectPtr<UNiagaraSystem> ImpactVFX` est conservé.
Pour un impact réussi, `ImpactHitVFXDefinition` est prioritaire, puis
`ImpactVFX` sert de variante unique avec transform identité et sans cooldown.
`ImpactVFX` n’est jamais utilisé pour un impact manqué.

## 6. Sélection déterministe

`FGridMonsterVFXSelector` combine `ResolvePersistenceId()`, `MonsterId`,
l’événement et son occurrence transitoire. Il utilise un `FRandomStream` local.
Il ne lit ni ne modifie `CombatRandomStream`, `EncounterRandomSeed` ou une
donnée sauvegardée, et n’utilise pas `FMath::Rand`.

## 7. Séparation du gameplay

Les VFX ne décident aucune action, ne lancent aucun jet, ne calculent et
n’appliquent aucun dégât, ne modifient aucune phase et ne retardent jamais le
combat. `bActiveAttackImpactCommitted` et `bDeathCommitted` restent les seules
autorités d’unicité logique.

## 8. Spawn attaché

Alert, Attack, Hurt et Death peuvent utiliser
`UNiagaraFunctionLibrary::SpawnSystemAttached`. Le composant privilégie le
`SkeletalMeshComponent`, applique le socket et le transform relatif, puis
utilise le RootComponent en repli. La surcharge UE 5.5.4 avec échelle,
`EAttachLocation::KeepRelativeOffset`, auto-destruction et
`ENCPoolMethod::None` est utilisée.

## 9. Spawn monde

Les impacts sont toujours produits avec `SpawnSystemAtLocation`, même si la
définition demande un attachement. La position logique vient du `PartyPawn`,
avec la position du monstre en repli. Aucun raycast ou calcul de collision
n’est ajouté.

## 10. Offsets, rotation, échelle et sockets

Les impacts ajoutent `LocationOffset` à la position fournie et s’orientent du
monstre vers la cible lorsque ce vecteur est exploitable. `RotationOffset` et
`Scale` sont ensuite appliqués. Un socket `None` est autorisé, y compris pour
un effet attaché.

## 11. Impacts réussis et manqués

`Result.bHit` choisit exclusivement `ImpactHit` ou `ImpactMiss`. La requête
expose l’attaque, le résultat réel et l’index du personnage ciblé. Deux
notifications d’impact ne créent qu’une résolution et qu’une requête VFX.

## 12. Blessure et mort exclusives

Une réussite sans dégât appliqué ne produit pas Hurt. Des dégâts non mortels
produisent Hurt ; des dégâts mortels produisent Death sans Hurt supplémentaire.
Une restauration morte n’émet aucun événement.

## 13. Données physiques et élémentaires

La requête expose `DamageType` et `PhysicalSubtype`. Le DataAsset choisit donc
librement sang, poussière, étincelles, feu, glace ou poison. Le composant ne
contient aucune règle spéciale pour le sang ou un élément.

## 14. Futur feedback visuel de cible

MON10.3 ne crée ni widget ni post-process. Une future couche Blueprint pourra
utiliser `TargetCharacterIndex`, `FGridAttackResult`, le critique, les dégâts
appliqués et la position monde pour produire flash, vignette ou feedback
spécifique au personnage.

## 15. Delegate Blueprint

`OnVFXSpawnRequested` reçoit exactement une
`FGridMonsterVFXSpawnRequest` par requête acceptée. Le delegate, les compteurs
et `LastSpawnRequest` restent actifs lorsque `bNativeSpawnEnabled=false`.

## 16. Nettoyage des Niagara Components

Les composants natifs sont suivis uniquement par références faibles. Les
références invalides sont compactées lors d’une requête. `StopAllMonsterVFX`,
la désactivation du niveau et `EndPlay` désactivent/détruisent les composants
encore actifs sans empêcher leur auto-destruction.

## 17. Absence de Tick

`UGridMonsterVFXComponent` désactive Tick. Les cooldowns sont vérifiés seulement
au moment d’une requête.

## 18. Absence de persistance

Requêtes, compteurs, occurrences, cooldowns et références Niagara sont
transitoires. Aucun champ VFX n’est ajouté à `FGridRuntimeMonsterState`,
`FGridLevelRuntimeState`, `UGridDungeonRuntimeState` ou la sauvegarde du groupe.

## 19. Tests

La suite `Grimrock.Monsters.MON10.VFX` couvre validation, sélection
déterministe, Alert, Attack/Impact, exclusivité Hurt/Death, restauration morte,
données de requête, désactivation du rendu natif, absence de Tick/persistance
et nettoyage. Elle utilise uniquement des `UNiagaraSystem` transitoires avec
`bNativeSpawnEnabled=false`.

## 20. Configuration manuelle future du Rat géant

Noms recommandés, sans création par ce jalon :

```text
Content/GrimrockPrototype/VFX/Monsters/RatGiant/
    NS_Rat_Alert
    NS_Rat_Attack_Bite
    NS_Rat_Impact_Blood
    NS_Rat_Impact_Miss
    NS_Rat_Hurt
    NS_Rat_Death
```

Variantes futures possibles :

```text
NS_Rat_Impact_Fire
NS_Rat_Impact_Ice
NS_Rat_Impact_Poison
```

## 21. Procédure PIE

1. Créer ou importer manuellement les Niagara Systems.
2. Ouvrir `DA_MON_RatGiant`.
3. Assigner `AlertVFX`.
4. Assigner `HurtVFX`.
5. Assigner `DeathVFX`.
6. Ouvrir `Attack_Bite`.
7. Assigner `AttackVFXDefinition`.
8. Assigner `ImpactHitVFXDefinition`.
9. Assigner `ImpactMissVFXDefinition`.
10. Démarrer une nouvelle partie.
11. Déclencher un combat.
12. Vérifier un seul AlertVFX par Rat.
13. Terminer la phase du joueur.
14. Vérifier AttackVFX au début de la morsure.
15. Vérifier ImpactHit ou ImpactMiss au moment réel de l’impact.
16. Vérifier que l’impact ne se joue qu’une fois.
17. Blesser un Rat sans le tuer.
18. Vérifier HurtVFX.
19. Tuer le Rat.
20. Vérifier DeathVFX sans HurtVFX supplémentaire.
21. Sauvegarder et recharger.
22. Vérifier qu’un Rat mort ne rejoue aucun effet.
23. Désactiver le niveau runtime.
24. Vérifier qu’aucun Niagara actif ne subsiste.
25. Filtrer l’Output Log sur `GridMonsterVFX`.
26. Vérifier les séquences et l’absence de doublons.

## 22. Limites du jalon

MON10.3 ne crée aucun asset, HUD, widget, post-process, decal, camera shake,
hit stop, effet permanent, table élémentaire globale ou VFX d’attaque du
joueur. Le chargement est synchrone uniquement pour la variante choisie. Un
chargement asynchrone sera étudié dans MON10.5 ; aucun pooling personnalisé
avancé ou état VFX sauvegardé n’est ajouté.
