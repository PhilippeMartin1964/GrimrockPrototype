# RPG-TALENT-FIX03 — D03 Sabotage d'objet du monde

Date : **8 octobre 2026**  
État : **AUDITÉ / DIFFÉRÉ — dépendance au flux générique d'aptitudes hors combat**

## Contrat

Le Talent Sabotage possède deux usages distincts :

1. **combat** : cible Mechanical/Construct, test Intelligence + Mécanique, puis
   `Status_Sabotaged` en cas de succès ;
2. **hors combat** : un objet de monde explicitement sabotable reçoit son
   événement `Sabotaged` après un Skill Check Mécanique réussi.

L'usage combat est déjà raccordé.

## Primitives de monde déjà présentes

```text
UGridWorldObjectDefinitionAsset
    bCanBeSabotaged
    SabotageDifficulty

AGridLevelRuntimeActor
    GetRuntimeObjectSabotageDifficulty(ObjectId, OutDifficulty)
    ExecuteRuntimeObjectSabotage(ObjectId)
        -> ExecuteLinksFromRuntimeObject(ObjectId, Sabotaged)
```

Le Skill `Skill_Mechanics` existe aussi dans le catalogue de production.

## Élément manquant

Aucun caller actuel ne réalise la chaîne :

```text
joueur choisit une aptitude hors combat
-> cible un objet de monde
-> récupère SabotageDifficulty
-> résout Skill_Mechanics
-> en cas de succès appelle ExecuteRuntimeObjectSabotage
```

Le moteur actuel d'interaction souris exécute les interactions propres aux
acteurs ; il ne fournit pas encore un pipeline générique d'aptitude active
hors combat ciblant un objet.

## Décision

Ne pas créer un chemin spécial :

```cpp
if (TalentId == "Talent_Rogue_Saboteur_Sabotage")
```

ni placer un Skill Check Mécanique directement dans `AGridWallLockActor` ou un
autre acteur arbitraire.

D03 est **différé** au futur système générique d'aptitudes hors combat. Ce
système devra réutiliser :

- l'autorité de Talent/action existante ;
- `FRPGSkillCheckService` ;
- le catalogue `URPGSkillAsset` de production ;
- les deux API sabotage déjà présentes dans `AGridLevelRuntimeActor`.

Aucun C++, DataAsset ou UMG n'est modifié par ce ticket documentaire.
