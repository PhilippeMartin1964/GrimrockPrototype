# TRAP01.7.1 — Détail des jets de désamorçage

**Statut : code publié sur GitHub master, validation Unreal Engine 5.5.4 / PIE à faire.**

## Format demandé et adopté

La notification est **un événement unique de trois lignes**, après un jet de
désamorçage effectivement résolu, quel que soit le chemin de déclenchement
(clic gauche sur piège détecté ou clic droit sur le kit de désamorçage).

```text
Elias désamorce le piège.
(1d20)=12 + (Piège / désamorçage)=3 + (DEX)=2 + (Classe)=0 + (Groupe)=0 = 17
Difficulté : 14 — Réussite !
```

Un échec reprend les mêmes cinq termes, même nuls, puis affiche
`Difficulté : N — Échec !`. Les modificateurs négatifs sont conservés,
et les nombres reflètent le résultat déterministe du service de compétences,
jamais un nouveau tirage ou des valeurs calculées par le widget.

## Données — pas de changement des règles RPG

`FRPGSkillCheckResult` expose maintenant `ClassModifier` et `GroupModifier`
(en `BlueprintReadOnly`). Le premier est agrégé à chaque
`FRPGSkillProgressionModifier.CheckModifier` applicable au personnage ;
le second est défini à partir de
`FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier`, qui respecte
les groupes de non-cumul et l'état vivant des membres.

Le champ historique `ProgressionModifier` demeure **strictement égal à**
`ClassModifier + GroupModifier`. `Total` garde la formule historique
`Roll + Rank + AttributeModifier + ProgressionModifier` et
`FRPGSkillCheckService` garde ses tirages, conditions, sélection du membre,
et calcul de succès/échec inchangés.

Aucun talent, DataAsset, effet de groupe ou widget nouveau n'est créé.
La différence entre classe et groupe est purement une **traçabilité du jet**.

La résolution du désamorçage continue d'utiliser sa copie d'état
restreinte aux porteurs du kit existant ; TRAP01.7.1 ne redéfinit pas
l'éligibilité des bonus de groupe.

## Tests

- `Grimrock.TRAP01.7.DisarmFeedbackBreakdown` : exemple exact, trois lignes,
  cinq termes, zéro toujours visible, échec et bonus négatif.
- `Grimrock.TRAP01.7.1.ClassGroupComponents` : bonus personnel +2,
  groupe +2 sans double comptage avec deux sources identiques, suppression
  du bonus lorsque les membres correspondants ne sont plus vivants.
- `Grimrock.MON20.6.Skills.SkillCheckFormula` : contrôle des champs
  à zéro et du total préexistant.

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.7"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.6.Skills"
```

**Recette PIE :** vérifier qu'un piège caché ne révèle aucun chiffre,
que les deux voies d'interaction affichent les trois lignes, que les
modificateurs de classe et de groupe sont cohérents, et que le journal
présente correctement les sauts de ligne. TRAP-SEARCH01 n'est pas concerné.
