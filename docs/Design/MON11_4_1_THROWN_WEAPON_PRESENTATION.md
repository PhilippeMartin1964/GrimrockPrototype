# MON11.4.1 — Véritable présentation des armes de jet

## Motif du correctif

MON11.4 créait bien un `HeldItemActor`, mais lui transmettait une définition
nulle. L’acteur existait donc sans mesh. Le style `Throw` utilisait en outre
la même interpolation sinusoïdale que les armes tenues : le shuriken avançait
puis revenait exactement dans la main comme un boomerang.

MON11.4.1 corrige ces deux défauts sans modifier les formules de combat.

## Visuel tenu

`AGrimrockPartyPawn::EquipHeldItem()` résout maintenant la véritable
`UGridItemDefinitionAsset` dans le registre d’inventaire, avec repli sur le
runtime de niveau. Le mesh affiché est :

1. `EquippedMesh` lorsqu’il est assigné ;
2. sinon `WorldMesh`.

`HeldItemActor` reste exclusivement visuel. Il ne devient jamais propriétaire
de l’instance équipée.

## Lancer accepté

Le style `Throw` avec `bAnimateHeldItem=true` suit ce chemin :

1. le `TurnManager` accepte et résout une seule attaque ;
2. `OnPlayerAttackRequested` sélectionne le profil `Throw` ;
3. une unité est extraite du slot offensif par
   `UGridPartyInventoryComponent` ;
4. `AGrimrockPartyPawn` crée un `AGridThrownItemActor` depuis la position du
   visuel tenu, ou depuis la caméra en repli ;
5. une pile équipée conserve sa quantité restante ;
6. le dernier exemplaire libère le slot et efface le visuel tenu ;
7. `OnPlayerAttackResolved` indique au projectile s’il doit s’arrêter sur la
   cible touchée ;
8. l’impact convertit le projectile en pickup de monde récupérable.

Si la création du projectile échoue après l’extraction, l’unité est restaurée
dans le même slot. Il n’existe donc ni perte ni duplication.

Un refus tel que `AttackerAlreadyActed`, `PassageBlocked` ou
`TargetOutOfRange` ne diffuse pas `OnPlayerAttackRequested` : aucun projectile
n’est créé et l’équipement ne change pas.

## Hit et miss

Sur un hit, le projectile observe sa trajectoire après la physique. Lorsqu’un
segment atteint ou dépasse le point de cible, il arrête son mouvement et
devient un pickup dans la cellule du monstre.

Sur un miss, cette interception est désactivée. Le projectile continue jusqu’à
une collision avec le décor ou le sol. À défaut, `ThrowLifeSeconds` provoque
sa conversion à la position atteinte, avec repli sur la cellule source si la
position n’appartient pas à une cellule valide.

Le projectile ne reste pas encore planté dans le SkeletalMesh du monstre.
Cette amélioration visuelle est différée ; l’objet tombe au sol et demeure
récupérable.

## Autorité de combat

`AGridThrownItemActor` ne contient aucune formule d’attaque et n’inflige aucun
dégât. Sa collision ne déclenche ni `ResolveAttack()`, ni
`ApplyAttackResult()`, ni Hurt, ni Death.

L’ordre gameplay de MON11.4 reste :

1. un seul `ResolveAttack()` ;
2. Attack ;
3. journal ;
4. un seul `ApplyAttackResult()` ;
5. Impact ;
6. feedback ;
7. Victory éventuelle.

Le résultat est donc déjà déterminé lorsque le projectile visible se déplace.
Le feedback et l’impact média restent synchrones avec le résultat de combat ;
ils ne sont pas retardés jusqu’à la collision visuelle.

## Diagnostics

NumPad 7 ajoute :

| Champ | Signification |
| --- | --- |
| `HeldItemMotionStarted` | doit rester `false` pour `Throw` |
| `ThrownItemLaunchRequests` | nombre de présentations Throw demandées |
| `ThrownItemLaunchStarted` | création native réussie pour la dernière attaque |
| `ThrownItemLaunchCount` | nombre total de projectiles créés |

Les logs `GridInventory EquipmentWorldTransfer`, `GridPlayerAttack Throw` et
`GridInventory Throw Impact` permettent de suivre l’unité depuis l’équipement
jusqu’au pickup de monde.

## Tests automatisés

`Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle` n’utilise aucun
asset Content et vérifie :

- préférence `EquippedMesh`, puis repli `WorldMesh` ;
- rejet d’un profil `Throw` non throwable ;
- extraction d’une unité empilée avec identité propre ;
- restauration exacte après échec simulé ;
- visuel tenu doté d’un mesh ;
- une requête de lancer et un seul projectile créé ;
- absence de mouvement sinusoïdal pour `Throw` ;
- décrément de la pile équipée ;
- interception de la cible sur hit ;
- conversion en pickup récupérable dans la cellule cible.

Le filtre `Grimrock.Monsters.MON11` contient désormais 20 tests et le filtre
global `Grimrock.Monsters.MON` en contient 92.

## Procédure PIE

1. Équiper une pile d’au moins deux shurikens en `MainHand`.
2. Vérifier le shuriken visible devant la caméra.
3. Placer le Rat géant à deux ou trois cellules dans l’axe.
4. Démarrer le combat et attendre `PlayerPhase`.
5. Appuyer une fois sur NumPad 7.
6. Vérifier le départ du projectile sans retour dans la main.
7. Vérifier `HeldItemMotionStarted=false`,
   `ThrownItemLaunchStarted=true` et un compteur égal à 1.
8. Vérifier que la pile équipée a diminué d’une unité.
9. Sur hit, récupérer le shuriken dans la cellule du Rat.
10. Sur miss, rechercher le shuriken au sol ou contre le premier obstacle.
11. Réessayer pendant la même phase : aucun nouveau projectile et aucune
    décrémentation.
12. Interposer un mur : `PassageBlocked`, aucun projectile et aucune mutation.
13. Lancer le dernier shuriken : `MainHand` devient vide et le visuel disparaît.

## Limites différées

- shuriken visuellement planté dans le mesh du monstre ;
- synchronisation différée d’Impact et du feedback avec l’arrivée visuelle ;
- rotation spécialisée du shuriken autour de son axe ;
- munitions séparées de l’arme ;
- récupération automatique ;
- projectile de dégâts ou résolution de combat par collision.
