# ARM-CLEAN06.4X.16.7.3 — Recette PIE après migration native des réceptacles

## Preuves automatiques

Parent X.16.7.2F : `ca32a56d825fa1f9e172556d258475a069ca1395`
— SilentTask SUCCESS, LocalAuto SUCCESS,
ARM-CLEAN06.4X72 SUCCESS.

Enfant binaire X.16.7.3 :
`d3ecdf6bb19b249c891db9e90e87413dbf3bc118`
— SilentTask SUCCESS et LocalAuto SUCCESS
**sur le SHA final**.

Le commit n'a changé que :
- `DA_Receptacle_TorchHolder.uasset`, Item_Torch ;
- `DA_Guardian.uasset`, Gem_Blue.

Pour chaque AcceptedItems[0], un CanonicalItemId est
maintenant physiquement enregistré ; les anciens
pointeurs ItemDefinition ont été conservés.
Ni les 4 niveaux, ni les deux serrures, ni
les SaveGames personnels n'ont été touchés.

## Vérifications en UE5.5.4

Après fermeture de l'Editor, dans un worktree
de développement propre uniquement :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
git lfs pull origin master
git log -1 --oneline
```

Recompiler puis lancer PIE. Vérifier :
1. Support de torche : `Item_Torch` accepté,
   autre objet rejeté, insertion/retrait fluide ;
   allumage au sol et rééquipement inchangés ;
2. Gardien : `Gem_Blue` acceptée, objet incompatible
   refusé, activation d'énigme et liens logiques
   inchangés ;
3. Pas de nouvelle restriction pour inventaire,
   potions, combat, déplacement ou clés.

Tester Save/Continue uniquement sur copie d'une
sauvegarde. Ne pas enregistrer involontairement de
vrais niveaux.

## Prochaine étape

Après accord PIE du propriétaire, migrer les
deux serrures dans une transaction binaire
**distincte**, avec préflight des ID acceptés,
sauvegardes SHA-256, relecture Unreal
indépendante, tests sur le SHA final
et recette de clés consommées/non consommées.

Ne pas restaurer/rejouer les migrations 06.4W,
06.4X.5 ni les déclencheurs X.16.7.2B/C/D.
