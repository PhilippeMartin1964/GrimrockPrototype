# RPG03.10 — Balance / cohérence globale / Automation / PIE

Date : **6 octobre 2026**  
Dépendance : `RPG03.9` — 6 classes / 90 talents conceptuels matérialisés  
Statut : **VALIDÉ — Grimrock.RPG.RPG03.10 6/6 et campagne globale Grimrock.RPG.RPG03 193/193, 0 warning, 0 échec, exit 0**

## But

RPG03.10 ne crée aucune nouvelle mécanique de classe.

Le jalon vérifie que l'ensemble RPG03 reste cohérent après l'authoring des 90 talents et que la projection réelle fonctionne dans `L_Dungeon` en PIE.

## Balance structurelle

Les invariants figés sont ceux déjà impliqués par la règle RPG03 :

- six classes de production ;
- quinze talents **conceptuels** par classe ;
- quatre-vingt-dix talents conceptuels au total ;
- dix Talent Points par classe au niveau 20 ;
- un point aux niveaux 2, 4, 6, 8, 10, 12, 14, 16, 18 et 20 ;
- trois choix conceptuels disponibles à chaque palier ;
- chaque Choice coûte un point.

Les variantes d'un même `ExclusiveChoiceGroupId` comptent comme **un** talent conceptuel. Cela couvre notamment les affinités Mage et les spécialisations d'armes sans gonfler artificiellement le total 90.

## Cohérence

Automation vérifie également :

- les six `DA_Class_*` chargent et sont valides ;
- les ActionIds de classe sont globalement uniques ;
- chaque Requirement d'action de classe est résoluble par la progression de cette classe ;
- les actions restent dans les bornes économiques actuellement définies par RPG03 :
  - AP 1..4 ;
  - Mana 0..16 ;
  - portée 0..6 ;
  - cooldown 0..5 ;
  - AreaRadius 0..2 ;
- les onze QuickItems Alchimiste matérialisés respectent les mêmes bornes et consomment un item ;
- tout Status effectivement appliqué par une action RPG03 possède un asset de production valide.

Ces bornes sont des **garde-fous de régression de la spécification actuelle**, pas un moteur de balance dynamique.

## PIE réel

Le test :

```text
Grimrock.RPG.RPG03.10.PIE.SixClassTalentRuntime
```

charge réellement :

```text
/Game/GrimrockPrototype/Maps/L_Dungeon
```

puis démarre un vrai monde `EWorldType::PIE`.

Dans le monde PIE uniquement, il construit transitoirement un groupe de six personnages niveau 20 :

- Warrior / Guardian ;
- Rogue / Assassin ;
- Ranger / Marksman ;
- Mage / Arcanist ;
- Priest / Restoration ;
- Alchemist / Grenadier.

Pour chaque personnage, le test sélectionne les cinq talents de la branche, reconstruit la projection MON15, puis vérifie :

- 5 talents sélectionnés lisibles par `FRPGTalentRuntimeService` ;
- 10 Talent Points accordés ;
- 5 points dépensés par une branche complète ;
- 5 points non dépensés conservés ;
- ClassId et talent terminal présents dans les RequirementIds runtime.

Le test remet ensuite l'état transitoire à zéro et termine PIE. Aucune sauvegarde ni asset n'est modifié.

## Campagne globale

Commande dédiée :

```powershell
.\Scripts\ValidateRPG03.ps1 -EngineRoot D:\UE_5.5
```

Elle compile l'Editor puis lance le filtre racine :

```text
Grimrock.RPG.RPG03
```

Ce filtre couvre C1..C8, l'authoring RPG03.9 et RPG03.10, PIE compris.

La clôture RPG03.10 est acquise par la sortie TD04.2 globale `Grimrock.RPG.RPG03` — 193/193.


## Correction de cohérence découverte par RPG03.10

La première campagne globale a révélé que les six `DA_Class_*` de production
avaient `ProgressionLevelGrants` vide. Les tests d'authoring de branches
injectaient localement des grants et masquaient donc cette lacune de production.

RPG03.10 centralise désormais l'autorité Editor dans :

```text
FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants()
```

et chaque authoring de classe l'appelle. Le commandlet
`RPGClassProgressionAuthoring` matérialise uniquement les six classes.

La règle `RPG_Class_Progression_1_20_v0_1.md` reste autoritaire :
10 Talent Points aux niveaux pairs 2..20. Les cinq paliers de choix restent
2/6/10/14/18.

`Equipment.Shield` n'est pas un requirement de progression : c'est un gate
runtime d'équipement. Le test de closure ne traite donc comme progression que
les requirements `Talent_*`.

`Action_Mage_Cataclysm` coûte 16 mana selon la spécification autoritaire ; la
borne de régression globale est donc 0..16.
