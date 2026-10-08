# RPG-TALENT-FIX04 — D06 Réaction en chaîne / SurfaceEffects

Date : **8 octobre 2026**  
État : **SOURCE PRÊTE — validation locale requise**

## Problème

Réaction en chaîne est correctement authorée comme réaction :

```text
Trigger          = SurfaceReaction
Limit            = OncePerAction
SourcePolicy     = QuickItem
RequiredSourceTag= QuickItem.Bomb
Damage modifier  = +25 %
Radius modifier  = +1
Recursive        = false
```

Mais le TurnManager n'émettait `SurfaceReaction` que pour
`Action.Definition.SurfaceInteraction`.

Les bombes passent par `SurfaceEffects`, donc une Bombe incendiaire posée sur
Poison/Oil/PoisonCloud/Ice bypassait le trigger.

## Correctif générique

`FGridCombatSurfaceResolver::ResolveAppliedSurfaceReaction()` relie un
`FGridCombatSurfaceEffectProfile` à la table canonique C6 :

```text
SurfaceEffect Fire -> Interaction Fire
SurfaceEffect Ice  -> Interaction Ice
autres surfaces    -> aucune interaction inventée
```

Le TurnManager utilise ensuite **le même** chemin de réaction :

```text
ExistingSurface
-> détection réaction canonique
-> SurfaceReaction event
-> C4 reaction ledger
-> OncePerAction
-> réponse +25 % / +1 si Réaction en chaîne
-> mutation C6
```

Le même `ActionInstanceId` est conservé sur toutes les cellules de la bombe.

## Préservation de la table canonique

Après réaction :

- si le résultat a le même type que la surface authorée, le profil explicite
  fournit sa durée / son payload périodique ;
- si le résultat diffère, il est conservé.

Exemple :

```text
Poison + Fire Bomb -> Fire
    -> profil Fire Bomb peut fournir la surface Fire finale

Ice + Fire Bomb -> Water
    -> Water est conservée ; la Fire Bomb ne réécrase pas immédiatement Water
```

## Limite déjà connue

La magnitude de base d'une explosion de surface n'est toujours pas inventée :
RPG02 ne la chiffre pas. Le résultat de réaction porte correctement les
modificateurs de Réaction en chaîne ; un futur exécuteur de magnitude explicite
pourra les consommer.

Ce point est distinct de D06 et reste cohérent avec RPG03.6 / RPG03.9.6A.

## Validation attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6A"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6B1"
```

Aucun DataAsset n'est modifié par ce ticket : l'authoring de Réaction en chaîne
et les tags `QuickItem.Bomb` étaient déjà corrects.
