# RPG-TALENT-FIX01 — D05 Repousser les morts-vivants : dégâts fixes

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS — 8 octobre 2026**

## Contrat

```text
Repousser les morts-vivants
Dégâts : 4 Sacré fixes
Scaling Sagesse : aucun
Scaling Skill : aucun
```

Le contrôle secondaire reste inchangé :

- uniquement Morts-vivants ;
- zone rayon 2 centrée sur le groupe ;
- si armure magique épuisée après les dégâts :
  - poussée forcée d'une case à l'opposé du groupe, si possible ;
  - Initiative -4 pendant 1 round.

## Cause

`MakeHolyAttack` configure légitimement un scaling Sagesse de base pour les
autres attaques sacrées du Prêtre. Repousser les morts-vivants réutilisait ce
helper sans neutraliser ce scaling alors que RPG02 impose 4 dégâts fixes.

## Correctif

Le Talent conserve le helper commun puis fixe explicitement :

```cpp
Action.OffensiveProfile.DamageScalingAttribute =
    EGridAttackScalingAttribute::None;
```

Aucun `switch(TalentId)` runtime n'est ajouté et les autres sorts sacrés ne sont
pas modifiés.

## Tests

Le test authoring exige désormais :

- MinDamage = 4 ;
- MaxDamage = 4 ;
- OffensiveProfile.DamageScalingAttribute = None ;
- aucun DirectDamageScaling attribut/Skill.

Le test de production impose le même contrat à `DA_Class_Priest`.

## Matérialisation attendue

Utiliser l'autorité existante :

```powershell
.\Scripts\AuthorRPGPriest.ps1 -EngineRoot D:\UE_5.5
```

Le script exige un working tree propre, recompile l'Editor, rematérialise les
assets Prêtre puis lance `Grimrock.RPG.RPG03.9.5`.

Les binaires ne sont commités qu'après lecture du `git status --short`.


## Validation finale

Validation utilisateur reçue après rematérialisation Prêtre :

```text
Grimrock.RPG.RPG03.9.5
Succeeded              : 21
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Les huit binaires attendus ont été rematérialisés puis commités dans :

```text
6dc8f98d RPG-TALENT-FIX01 materialize Turn Undead fixed damage
```

D05 est clos.
