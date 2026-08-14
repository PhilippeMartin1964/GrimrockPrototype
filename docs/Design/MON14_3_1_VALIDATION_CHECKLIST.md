# MON14.3.1 — Validation Checklist

## 1. Synchronisation

```powershell
git pull
```

Vérifier que `master` pointe sur le commit MON14.3.1 annoncé.

## 2. Compilation UE 5.5.4

```powershell
D:\UE_5.5\Engine\Build\BatchFiles\Build.bat GrimrockPrototypeEditor Win64 Development "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -WaitMutex -NoUBA -NoUBALocal -Log="D:\Development\GrimrockPrototype\Saved\Logs\UBT-MON1431.log"
```

Attendu : compilation sans erreur UHT/C++/linker.

## 3. Tests automatisés ciblés

Exécuter :

```text
Grimrock.Editor.MON14.3.1
```

Attendu :

```text
PatrolRouteEditingModel  Success
PatrolRouteGuards        Success
```

Puis exécuter :

```text
Grimrock.Monsters.MON14.3
Grimrock.Monsters.MON14.2
Grimrock.Monsters.MON14.1
```

Attendu : aucune régression.

Enfin, si le temps le permet :

```text
Grimrock.Monsters.MON
```

## 4. Validation manuelle — affichage

1. ouvrir la carte d'édition habituelle ;
2. activer le Grimrock Grid Editor ;
3. sélectionner un `MonsterSpawn` possédant déjà des `PatrolWaypoints` ;
4. vérifier que la route est visible sans appuyer sur `P`.

Attendu :

- marqueurs cyan ;
- labels `#1`, `#2`, ... ;
- segments de route ;
- flèches de Facing lorsqu'il est défini.

## 5. Validation manuelle — création

1. sélectionner un `MonsterSpawn` sans route ;
2. appuyer sur `P` ;
3. cliquer une première cellule ;
4. cliquer une deuxième cellule ;
5. cliquer une troisième cellule.

Attendu :

- le HUD affiche `PATROL ROUTE EDIT` ;
- premier clic : 1 waypoint, mode `None` ;
- deuxième clic : 2 waypoints, mode `Loop` ;
- troisième clic : 3 waypoints ;
- le dernier waypoint ajouté est sélectionné en jaune ;
- l'asset est dirty.

## 6. Sélection sans duplication

Cliquer sur une cellule contenant déjà un waypoint.

Attendu :

- aucun nouveau waypoint ;
- le waypoint existant devient jaune ;
- le nombre de waypoints reste identique.

## 7. Facing

Avec un waypoint sélectionné, appuyer plusieurs fois sur `F`.

Attendu :

```text
None -> North -> East -> South -> West -> None
```

Une flèche doit suivre visuellement chaque Facing cardinal.

## 8. WaitSeconds

Avec un waypoint sélectionné :

- `+` augmente l'attente de 0,5 s ;
- `-` la diminue de 0,5 s.

Attendu :

- le HUD actualise `Wait=...s` ;
- la valeur ne descend jamais sous 0.

## 9. Réorganisation

Avec le waypoint #2 sélectionné :

- `PageUp` le déplace vers #1 ;
- `PageDown` le redéplace vers #2.

Attendu :

- les labels changent d'ordre ;
- la géométrie de la route suit immédiatement ;
- le waypoint déplacé reste sélectionné.

## 10. Loop

Avec au moins 3 waypoints et `Loop` :

Attendu :

- segments #1 -> #2 -> #3 ;
- segment pointillé de fermeture #3 -> #1 ;
- flèche de direction sur la fermeture.

## 11. PingPong

Appuyer sur `M` jusqu'à `PingPong`.

Attendu :

- aucun segment #3 -> #1 ;
- les segments #1/#2 et #2/#3 montrent les deux directions.

## 12. Suppression

Sélectionner un waypoint puis `Delete`.

Attendu :

- le waypoint disparaît ;
- la route est immédiatement redessinée ;
- s'il ne reste qu'un waypoint, `PatrolMode` repasse à `None`.

## 13. Navigation viewport

Pendant l'édition de route :

- utiliser le clic droit pour déplacer/orienter la caméra.

Attendu : la navigation viewport reste normale et aucun waypoint parasite n'est créé.

## 14. Protection des autres outils

Entrer en route avec `P`, puis cliquer plusieurs cellules.

Attendu :

- aucune cellule/wall/object n'est peint ou effacé ;
- aucun connector n'est créé ;
- seuls les waypoints changent.

## 15. Sortie

Appuyer sur `P` une seconde fois.

Attendu :

- le HUD de route disparaît ;
- la route reste visible en lecture seule ;
- les clics retrouvent le comportement normal du Grid Editor.

Quitter complètement le Grid Editor puis le rouvrir.

Attendu : le mode d'édition de route ne reste pas bloqué actif.

## 16. Undo / Redo

Après plusieurs ajouts, suppressions, changements de Facing et d'attente :

- utiliser Undo ;
- utiliser Redo.

Attendu : les données et le dessin du viewport reviennent aux états précédents sans corruption.

## 17. Validation runtime

Après avoir créé une route valide, lancer le jeu/PIE avec le `MonsterSpawn` en `Idle`.

Attendu : MON14.3 consomme directement la route éditée et le monstre patrouille selon le mode choisi, sans conversion ni asset intermédiaire.
