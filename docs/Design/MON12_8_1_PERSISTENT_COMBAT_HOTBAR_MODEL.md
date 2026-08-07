# MON12.8.1 — Modèle persistant de barre de raccourcis de combat

## Résultat

MON12.8.1 ajoute dix raccourcis configurables à chaque
`FGridCharacterInventoryState`. Une nouvelle partie crée les indices `0` à
`9`, tous vides. La barre appartient au personnage et non au HUD ni au groupe :
changer de personnage changera donc de barre lors de MON12.8.2.

Ce jalon ne modifie aucun Widget Blueprint et n'implémente ni glisser-déposer,
ni touches numériques, ni résolution d'un raccourci contre le catalogue.

## Données persistées

Chaque entrée est un `FGridCombatHotbarBinding` contenant uniquement :

- `SlotIndex` : index stable de `0` à `9` ;
- `ActionId` : identité logique de l'action, par exemple
  `Attack_Unarmed` ;
- `SourcePolicy` : `Universal`, `Equipment`, `Ability`, `Spell` ou
  `QuickItem` ;
- `SourceDefinitionId` : DataAsset ou définition source stable ;
- `PreferredSourceRuntimeId` : instance préférée, obligatoire pour une arme
  afin de distinguer deux objets de même type ;
- `PreferredEquipmentSlot` : emplacement préféré de l'équipement.

Le modèle ne sérialise jamais `FGridAvailableCombatAction`. Les PA et mana
courants, la quantité disponible, la raison d'indisponibilité, la cible
suggérée et les cooldowns restent des projections runtime du catalogue
MON12.6.

Pour les futurs consommables, `SourceDefinitionId` sera l'identité principale
et `PreferredSourceRuntimeId` pourra rester vide. Le raccourci pourra ainsi
rester configuré quand la quantité atteint zéro, puis se réactiver quand un
objet identique revient dans l'inventaire.

## Initialisation et validation

`UGridPartyInventoryComponent::InitializeCharacterDefaults()` garantit dix
slots pour chaque personnage actif. Les personnages de réserve sont également
normalisés.

Le composant expose les opérations autoritaires suivantes :

- `GetCombatHotbarSlotCount()` ;
- `GetCharacterCombatHotbarBinding()` ;
- `SetCharacterCombatHotbarBinding()` ;
- `ClearCharacterCombatHotbarBinding()`.

Une affectation impose l'index demandé au binding. Une action universelle ne
possède aucune source. Une action non universelle exige une définition source ;
une action d'équipement exige aussi l'identifiant runtime de l'objet préféré.
Toute modification diffuse `OnPartyInventoryChanged` pour préparer le
rafraîchissement événementiel du futur HUD.

## Sauvegardes

`UGrimrockPartySaveGame::CurrentSaveVersion` passe de `2` à `3`. Les versions
`1` et `2` restent compatibles.

Une ancienne sauvegarde désérialise naturellement `CombatHotbarSlots` comme un
tableau vide. `RestorePartyInventoryState()` convertit alors ce tableau en dix
slots vides avant la validation atomique. Une barre présente mais mal formée
est rejetée et l'état précédent du groupe reste intact.

La prochaine sauvegarde réécrit le slot au format version `3` sans créer de
second objet `SaveGame`.

## Tests automatisés

Le filtre suivant couvre le jalon :

```text
Grimrock.Monsters.MON12.8.1
```

Les tests vérifient :

1. dix slots vides à la création ;
2. l'indépendance des barres entre personnages ;
3. affectation et suppression via le composant ;
4. sérialisation en mémoire de toute l'identité d'une arme ;
5. migration d'une sauvegarde sans barre ;
6. rejet atomique d'une barre invalide.

## Suite MON12.8.2

MON12.8.2 pourra remplacer la liste automatique du HUD par dix widgets fixes,
affichés dans l'ordre clavier `1 2 3 4 5 6 7 8 9 0`, puis connecter le
glisser-déposer aux quatre opérations du composant. La résolution contre
`FGridAvailableCombatAction` restera un traitement runtime distinct.
