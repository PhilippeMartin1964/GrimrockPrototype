# RPG03.9.6A — Support générique Alchimiste

Date : **6 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : `RPG03.9.5` Prêtre validé 21/21 et matérialisé  
Statut : **CODE — VALIDATION UE UTILISATEUR REQUISE**

## Objet

Ajouter uniquement les primitives génériques manquantes avant l'authoring des 15 Talents Alchimiste.

Aucun `TalentId` Alchimiste n'est interprété au runtime, aucune recette n'est inventée et aucun `.uasset` n'est créé.

## 1. Coût de traversée des surfaces

`FGridCombatSurfaceEffectProfile`, `FGridCombatSurfaceConversionProfile` et l'état SaveGame de surface portent désormais un coût de traversée authorable.

Ce coût est payé :

- en **PAM** par le groupe lors d'une translation normale ;
- en **PA** par un monstre lorsqu'il entre dans la cellule ;
- jamais par un déplacement forcé du groupe.

Cela permet à `Surface_Oil` d'authorer le `+1` demandé sans hard-code sur le type Oil.

Les sorties d'une réaction canonique ne conservent pas implicitement le coût de la surface précédente. Une conversion peut au contraire authorer explicitement le coût de sa sortie.

## 2. Interaction de surface authorable

`FGridCombatActionDefinition::SurfaceInteraction` raccorde une action Cell/Area au résolveur C6 existant.

`EGridCombatSurfaceInteraction::AnyCanonical` choisit déterministement la première interaction valide :

```text
Fire -> Ice -> Lightning -> Wind
```

Ce mode est destiné à `Action_Alchemist_Catalyst`.

Une action dont l'interaction est son seul payload est rejetée au ciblage si aucune réaction canonique n'est possible. Les ressources ne sont donc pas payées dans ce cas.

## 3. SurfaceReaction / OncePerAction

C4 peut maintenant porter comme réponse :

```text
SurfaceReactionDamagePercentModifier
SurfaceReactionAreaRadiusModifier
```

Le TurnManager émet un événement `SurfaceReaction` avant la mutation C6 réelle. Le ledger C4 applique `OncePerAction` avec le même `ActionInstanceId` pour toutes les cellules résolues.

Cela fournit à Réaction en chaîne le contrat générique : filtre `QuickItem.Bomb`, +25 % dégâts de réaction, +1 rayon, une seule fois par action et anti-récursion par défaut.

## 4. Limite volontaire : magnitude d'explosion

C6 expose déjà `bExplosive`, le type de dégâts, le modificateur et le rayon, mais la règle des 90 Talents ne chiffre aucune magnitude de base universelle pour une explosion de surface.

RPG03.9.6A **n'invente aucune constante de dégâts**. Le bonus est résolu dans le résultat de réaction ; une future magnitude explicitement authorée pourra le consommer sans changer le contrat C4/C6.

## 5. Recettes

Aucun moteur `RecipeId` n'existe actuellement. RPG03.9.6 ne crée pas un système Crafting parallèle. Les QuickItems de production seront authorés dans les jalons suivants.

## Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6A"
```
