# FIREFX09 — Surcharges de paramètres Niagara par profil, sans duplication

## Finalité

Les effets FireFX exposent des dizaines de paramètres `User.*`. Un
`DA_GroundFire` peut en utiliser un sous-ensemble, tandis qu'un
`DA_TorchFire` ou un autre profil versionné réutiliserait le même système
avec d'autres réglages.

La structure réutilisable `FGridNiagaraUserOverride` représente une
surcharge **nom + type + valeur**, déclarable dans `DA_GroundFire →
Settings → User Parameter Overrides`, et vérifiée à l'exécution contre les
véritables paramètres de `GroundFireSystem`. Formats pris en charge :
`Float`, `Integer`, `Boolean`, `Vector3`, `Color`.

## Priorités

1. Valeurs originales du système Niagara partagé (elles ne sont jamais
   modifiées dans le pack).
2. Ajustements automatiques du profil feu : `SpawnBox` (couverture),
   `FlameRate` (densité), `SmokeRate` (fumée), `Tint` (couleur).
3. Anciennes liaisons explicites optionnelles sur le profil.
4. **User Parameter Overrides** validés par nom + type, appliqués en dernier.

Les modifications sont appliquées séparément sur chaque
`UNiagaraComponent`, avant l'activation et lors des mises à jour PIE.
Plusieurs usages du même Niagara ne partagent donc pas les valeurs
de leurs instances.

## Exemple (à utiliser uniquement après examen des types du diagnostic)

Pour augmenter des particules de braises en gardant le feu de base :
`ParameterName=EmberRate`, `ValueType=Float`,
`FloatValue=12` **si** le diagnostic confirme que `EmberRate`
est bien un `Float`. Cet exemple est un choix artistique, non une
valeur recommandée du vendeur.

La même technique fonctionne pour `FlameSizeMin/Max`, `Wind`,
`GlowSize`, `HazeRate`, etc. seulement si leur type est compatible
avec un des formats pris en charge. Un paramètre absent ou de
mauvais type est simplement ignoré ; le diagnostic récapitule combien
de surcharges sont valides, mais le runtime ne transforme jamais
le système Niagara d'origine.

## Limitations

`User Parameter Overrides` permet de **fournir une valeur aux
paramètres exposés**, mais ne prouve pas à lui seul que les modules
internes du Niagara en tiennent compte. L'asset FAB doit les relier
à ses graphes, ce qui semble être le cas de nombreux contrôles
listés dans le diagnostic mais exige une vérification visuelle.
