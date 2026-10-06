# RPG03.10 — Balance / cohérence globale / Automation / PIE

Date : **6 octobre 2026**  
Dépendance : `RPG03.9` — 6 classes / 90 talents conceptuels matérialisés  
Statut : **CODE — VALIDATION UE UTILISATEUR REQUISE**

## But

RPG03.10 ne crée aucune nouvelle mécanique de classe.

Le jalon vérifie que l'ensemble RPG03 reste cohérent après l'authoring des 90 talents et que la projection réelle fonctionne dans `L_Dungeon` en PIE.

## Balance structurelle

Les invariants figés sont ceux déjà impliqués par la règle RPG03 :

- six classes de production ;
- quinze talents **conceptuels** par classe ;
- quatre-vingt-dix talents conceptuels au total ;
- cinq points de talent par classe ;
- un point aux niveaux 2, 6, 10, 14 et 18 ;
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
  - Mana 0..15 ;
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

Dans le monde PIE uniquement, il construit transitoirement un groupe de six personnages niveau 18 :

- Warrior / Guardian ;
- Rogue / Assassin ;
- Ranger / Marksman ;
- Mage / Arcanist ;
- Priest / Restoration ;
- Alchemist / Grenadier.

Pour chaque personnage, le test sélectionne les cinq talents de la branche, reconstruit la projection MON15, puis vérifie :

- 5 talents sélectionnés lisibles par `FRPGTalentRuntimeService` ;
- 5 points accordés ;
- 5 points dépensés ;
- 0 point restant ;
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

La clôture RPG03.10 ne sera déclarée qu'après lecture de la sortie TD04.2 fournie par l'utilisateur.
