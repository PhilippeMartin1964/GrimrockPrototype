# MON11.1 — Ciblage des monstres et pipeline de requête d’attaque

## Périmètre

MON11.1 introduit la demande d’attaque d’un personnage contre le monstre qui
occupe la cellule directement devant le groupe. Le jalon s’arrête à
l’acceptation d’une requête structurée : il ne choisit pas encore une arme, ne
fait aucun jet et n’applique aucun dégât.

Aucun asset `Content/`, DataAsset de monstre, mapping d’entrée, son, VFX,
montage ou Animation Blueprint ne fait partie de ce jalon.

## Responsabilités

- `UGridTurnManagerComponent` valide la phase, l’attaquant, le passage, la
  cible et la limite d’une attaque engagée par personnage et par phase joueur.
- `AGrimrockPartyPawn` fournit la cellule et l’orientation du groupe.
- `UGridPartyInventoryComponent` fournit
  `FGridCharacterInventoryState`, l’index sélectionné et les points de vie de
  l’attaquant. Un personnage n’est pas un Actor.
- `AGridLevelRuntimeActor` calcule la cellule voisine et valide le passage,
  notamment les murs et portes fermées.
- `UGridMonsterOccupancySubsystem` est l’autorité qui retourne l’occupant
  exact de la cellule cible.
- `AGridMonsterActor` représente la cible et fournit son identité persistante,
  son activation runtime, son état et ses points de vie.
- `AGrimrockPlayerController` ne fait que relayer la commande temporaire
  NumPad 7 en build non Shipping.

Ni `UGridMonsterCombatComponent`, ni l’inventaire, ni le PlayerController, ni
le RuntimeActor ne possèdent la requête.

## Structure de la requête

`FGridPlayerAttackRequest` est une donnée transitoire Blueprint contenant :

- un `RequestId` unique ;
- le numéro de manche ;
- l’index et le `CharacterId` de l’attaquant ;
- le `TargetMonsterId` persistant ;
- la cellule du groupe et la cellule cible ;
- l’orientation du groupe ;
- la portée provisoire de 1 cellule ;
- `AttackId = None` pour MON11.1.

Une requête valide exige des GUID valides, une manche positive, un index
d’attaquant défini, une orientation cardinale et une portée positive.

`OnPlayerAttackRequested` est diffusé exactement une fois après acceptation.
`LastPlayerAttackRequest` et le motif du dernier refus restent transitoires et
ne sont pas sauvegardés.

## Raisons de refus

| Raison | Signification |
| --- | --- |
| `None` | Requête acceptée. |
| `TurnManagerNotInitialized` | Le TurnManager n’est pas initialisé. |
| `CombatInactive` | Aucun combat n’est actif. |
| `NotPlayerPhase` | La phase courante n’est pas `PlayerPhase`. |
| `PartyUnavailable` | Le runtime, le Pawn ou l’inventaire du groupe manque. |
| `PartyBusy` | Le groupe n’est pas exactement au repos. |
| `InvalidAttacker` | L’index ou le `CharacterId` est invalide. |
| `AttackerDefeated` | Le personnage n’a plus de points de vie. |
| `AttackerAlreadyActed` | Le personnage a déjà engagé une attaque pendant cette phase joueur. |
| `InvalidFacing` | L’orientation n’est pas cardinale. |
| `TargetCellUnavailable` | La cellule avant ou l’autorité d’occupation est indisponible. |
| `PassageBlocked` | Un mur, une porte fermée ou une cellule non franchissable bloque le passage. |
| `NoMonsterInFront` | Aucun monstre n’occupe la cellule avant. |
| `TargetNotInEncounter` | Le monstre trouvé ne fait pas partie de la rencontre. |
| `TargetInactive` | Le monstre est désactivé, hors niveau runtime ou sans identité valide. |
| `TargetDefeated` | Le monstre est mort ou à zéro point de vie. |
| `TargetOutOfRange` | La distance de grille n’est pas exactement égale à 1. |

Un refus conserve une requête de sortie invalide, ne consomme aucune action,
ne diffuse aucun événement d’acceptation et ne modifie aucune statistique.

## Ciblage par cellule et occupation

Le ciblage suit une chaîne unique :

1. lire la cellule et l’orientation du `AGrimrockPartyPawn` ;
2. obtenir la cellule avant avec `TryGetNeighborCell()` ;
3. vérifier le passage avec `CanMove()` ;
4. demander l’occupant exact à `GetOccupantAtCell()` ;
5. vérifier que cet Actor appartient à `CombatMonsters`.

Aucun visibility trace, parcours global d’Actors ou choix automatique d’une
autre cible n’est effectué. Un monstre latéral est donc ignoré.

## Définition provisoire de « peut agir »

Pour MON11.1, un personnage peut engager une attaque lorsque le combat est
actif, la phase est `PlayerPhase`, le groupe est au repos, son index et son
identité sont valides, ses points de vie sont positifs et il n’a pas déjà
engagé une attaque dans cette phase.

L’ensemble des `CharacterId` engagés est réinitialisé au démarrage du combat,
au début de la première manche, au retour de chaque nouvelle `PlayerPhase`, à
l’abandon et à la fin du combat. La fin de phase reste une commande explicite :
le TurnManager ne termine pas automatiquement la phase lorsque tous les
personnages ont agi.

## Commande NumPad 7

En build non Shipping, NumPad 7 appelle
`RequestSelectedCharacterAttack()`. Cette fonction lit uniquement
`GetSelectedCharacterIndex()` puis délègue à
`RequestCharacterAttack()`.

Le diagnostic `[GridPlayerAttack]` indique le résultat, la raison, l’index de
l’attaquant, le `TargetMonsterId` et la cellule cible. Les bindings natifs de
NumPad 1 à 6 restent inchangés. Aucun `InputAction` ni
`InputMappingContext` n’est créé.

## Tests automatisés

Les quatre tests sont enregistrés sous `Grimrock.Monsters.MON11` :

- `Targeting` vérifie la cellule avant, l’occupant exact, l’exclusion latérale,
  les passages bloqués et les cellules vides ;
- `RequestValidation` couvre les phases, les attaquants invalides ou vaincus
  et les cibles hors rencontre, désactivées ou inactives ;
- `RequestAcceptedWithoutDamage` vérifie tous les champs et l’absence stricte
  de modification de la santé, des armures et de l’état du monstre ;
- `PerCharacterActionGate` vérifie la barrière par personnage, son
  renouvellement à la phase suivante et la neutralité des refus.

Les tests emploient un `UWorld`, un `UGridLevelAsset` et des objets transitoires
sans asset `Content/`, rendu, audio ou GPU.

## Validation PIE

1. Ouvrir une scène existante avec un Rat géant sans modifier la carte.
2. Placer le groupe dans la cellule adjacente, face au Rat, sans mur ni porte
   fermée.
3. Démarrer le combat avec NumPad 1 et attendre `PlayerPhase`.
4. Appuyer sur NumPad 7 et vérifier l’acceptation `[GridPlayerAttack]`, les
   identités et la cellule cible.
5. Vérifier que points de vie, armures et état du Rat restent inchangés.
6. Appuyer une seconde fois et vérifier `AttackerAlreadyActed`.
7. Terminer avec NumPad 2, attendre la prochaine `PlayerPhase` et vérifier
   qu’une nouvelle demande est acceptée.
8. Tourner le groupe hors du Rat et vérifier `NoMonsterInFront`.
9. Vérifier qu’un mur ou une porte fermée produit `PassageBlocked`.
10. Vérifier les refus en Exploration et en `EnemyPhase`.

Aucun son, VFX ou animation d’attaque de personnage n’est attendu.

## Éléments différés

- MON11.2 définira le profil offensif d’arme et la sélection d’attaque.
- MON11.3 appellera le résolveur, produira les jets et appliquera les résultats.
- MON11.4 traitera le retour visuel, sonore et l’intégration complète de
  présentation.

Les règles avant/arrière, cooldowns, dégâts, blessures, morts et événements de
combat du personnage ne sont pas implémentés dans MON11.1.
