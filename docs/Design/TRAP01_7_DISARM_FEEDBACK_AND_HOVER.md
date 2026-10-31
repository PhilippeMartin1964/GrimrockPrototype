# TRAP01.7 — Retour de désamorçage et infobulle de mécanisme

**Statut : C++ publié sur master ; compilation UE 5.5.4 et PIE à valider.**

## Contrat préservé

La sélection du meilleur personnage, son `Skill_Traps`, la Dextérité
(transitoirement projetée sur une copie de l'asset), le modificateur de
progression personnelle **et celui du groupe** restent intégralement
gérés par `FRPGSkillCheckService::TryResolveBestPartySkillCheck`.

Aucun nouveau jet de dés, calcul, bonus inventé, consommation d'outil ou
récompense d'XP n'est introduit. L'échec conserve le piège armé ; la
réussite le désamorce et déclenche les liens `Disarmed` comme avant.

## Retour joueur unifié

Les deux actions (clic gauche sur le piège révélé, ou clic droit sur le kit
de désamorçage dans l'inventaire) utilisent la **même fonction de formatage**
`GridDungeonTrapDisarmFeedback::Format`, uniquement après un jet résolu.

Exemple pour un succès :
`Elias désamorce le piège. Pièges : d20 12 + rang 3 + DEX +2 + progression -1 = 16 (difficulté 14).`

`ProgressionModifier` est le total résolu par le service canonique,
déjà composé de la progression du personnage et du bonus de groupe.
Il n'est **jamais recompté** dans le formateur. Le détail est affiché
seulement **après** le jet ; la difficulté d'un piège encore caché
n'apparaît jamais dans l'interface.

## Infobulle de monde (survol)

Sur un objet `AGridGenericObjectActor` qui porte le GUID d'un piège
révélé, `GetInteractionText` annonce suivant l'état :
- détecté : clic gauche pour tenter un désamorçage (compétence et outils) ;
- déclenché : cliquer pour examiner l'état ;
- désamorcé : confirmation de neutralisation ;
- désactivé : confirmation de désactivation.

Aucun texte de piège n'est retourné pour l'état `Hidden` ou `Unknown`.
Aucun DC, jet futur ou emplacement non découvert n'est exposé au survol.
Ce texte utilise le système de texte de survol de monde existant :
**pas de nouveau widget UMG** ni de duplication des règles RPG.

## Tests et recette

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.7"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP_PIT01"
```

En PIE, utiliser un kit de désamorçage et un piège détecté pour vérifier
les mêmes détails de calcul avec les deux interactions. Vérifier aussi la
présentation d'un piège déclenché/désamorcé et **l'absence de tooltip**
pour un piège non découvert.

La création éventuelle d'un **bouton explicite de recherche des environs**
reste un jalon séparé, non couvert ici.
