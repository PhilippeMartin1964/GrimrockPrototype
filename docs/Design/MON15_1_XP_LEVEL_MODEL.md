# MON15.1 — XP & Level Model

Statut : **VALIDÉ sous Unreal Engine 5.5.4 — Automation Tests fournis le 15 août 2026**.

Ce document décrit le modèle de progression introduit par MON15.1 et sa validation. Les quatre Automation Tests dédiés ont été exécutés sous Unreal Engine 5.5.4 et ont tous retourné `Success`. Les trois tests de non-régression `Grimrock.CharacterCreation.CC2.*` fournis dans la même session ont également retourné `Success`.

Aucun log séparé de commande de compilation n'a été fourni avec cette validation ; le présent document n'affirme donc pas qu'une commande de build distincte a été exécutée. Il constate uniquement les résultats UE5.5.4 effectivement fournis.

---

## 1. Audit du modèle existant

MON15.1 réutilise les fondations existantes au lieu de créer un second état RPG.

L'état persistant reste :

```cpp
FGridCharacterInventoryState::Level
FGridCharacterInventoryState::Experience
```

`Experience` est interprétée comme une **XP totale cumulative**. Elle n'est jamais remise à zéro lors du passage d'un niveau.

La création de personnage existante initialise déjà :

```cpp
Level = 1;
Experience = 0;
```

et calcule les statistiques dérivées avec :

```cpp
URPGCharacterRulesLibrary::CalculateDerivedStats(
    Attributes,
    ClassDefinition,
    Level)
```

`URPGClassAsset` reste l'autorité des valeurs de classe telles que `HealthAtLevelOne`, `HealthPerLevel`, `ManaAtLevelOne` et `ManaPerLevel`.

MON15.1 ne modifie ni la création de personnage, ni l'inventaire, ni l'équipement, ni le combat, ni la hotbar, ni le chargement de sauvegarde.

---

## 2. Autorité des règles de progression

Les règles XP/niveau vivent dans :

```text
URPGCharacterRulesLibrary
```

Cette bibliothèque était déjà l'autorité des règles RPG pures. Aucune nouvelle classe, structure runtime, DataAsset ou subsystem n'est ajouté pour MON15.1.

Il ne doit exister aucune copie de la courbe XP dans le HUD, les monstres, le combat, `GridPartyInventoryComponent` ou les widgets.

---

## 3. Niveaux supportés

Le modèle MON15.1 supporte :

```text
Niveau minimum : 1
Niveau maximum : 20
```

Le niveau maximum est une règle système centralisée dans `RPGCharacterRulesLibrary.cpp`.

---

## 4. Courbe XP

La courbe est déterministe et calculée, sans table de données dupliquée.

Pour un niveau `L` compris entre 1 et 20 :

```text
XP cumulative(L) = 1000 × (L - 1) × L / 2
```

Le coût du passage du niveau `L` au niveau `L + 1` est donc :

```text
1000 × L
```

Exemples :

| Niveau atteint | XP totale requise | XP depuis le niveau précédent |
|---:|---:|---:|
| 1 | 0 | 0 |
| 2 | 1 000 | 1 000 |
| 3 | 3 000 | 2 000 |
| 4 | 6 000 | 3 000 |
| 5 | 10 000 | 4 000 |
| 10 | 45 000 | 9 000 |
| 15 | 105 000 | 14 000 |
| 20 | 190 000 | 19 000 |

Chaque seuil est strictement supérieur au précédent.

---

## 5. Normalisation

L'XP valide est bornée à :

```text
0 .. 190000
```

Les helpers purs normalisent une XP utilisée pour un calcul :

```text
XP < 0       -> 0
XP > 190000  -> 190000
```

La demande de seuil d'un niveau invalide est également bornée :

```text
Level < 1   -> seuil du niveau 1
Level > 20  -> seuil du niveau 20
```

La validation de cohérence est volontairement plus stricte : une valeur brute d'XP négative ou supérieure au plafond est considérée invalide même si elle pourrait être normalisée pour un calcul.

---

## 6. Helpers introduits

`URPGCharacterRulesLibrary` expose les helpers purs suivants :

```cpp
GetMinimumLevel()
GetMaximumLevel()
GetCumulativeExperienceRequiredForLevel(Level)
GetLevelForExperience(TotalExperience)
GetExperienceInCurrentLevel(TotalExperience)
GetExperienceRemainingToNextLevel(TotalExperience)
IsMaximumLevel(Level)
NormalizeExperience(TotalExperience)
IsLevelExperienceConsistent(Level, TotalExperience)
```

### Reconstruction Level <- Experience

`GetLevelForExperience()` permet de reconstruire le niveau attendu depuis l'XP cumulative. Cette reconstruction servira aux validations et aux futures migrations, sans remplacer le champ `Level` persistant existant.

### Cohérence Level / Experience

`IsLevelExperienceConsistent()` retourne vrai uniquement si :

- `Level` appartient à `[1, 20]` ;
- `Experience` appartient déjà à `[0, 190000]` ;
- le niveau reconstruit depuis `Experience` est identique au `Level` sérialisé.

MON15.1 ne corrige pas automatiquement un état incohérent et ne déclenche aucun level-up.

---

## 7. Niveau maximum

À partir de 190 000 XP :

```text
Level = 20
XP restante vers le niveau suivant = 0
```

L'XP de calcul est plafonnée à 190 000. MON15.1 n'introduit pas d'overflow XP au-delà du niveau maximum.

---

## 8. SaveGame

Aucun champ persistant n'est ajouté, supprimé ou renommé.

`FGridCharacterInventoryState::Level` et `FGridCharacterInventoryState::Experience` existaient déjà et restent inchangés.

Par conséquent MON15.1 :

- ne modifie pas `UGrimrockPartySaveGame` ;
- ne change pas `CurrentSaveVersion` ;
- ne déclenche aucune migration automatique au chargement.

La version de sauvegarde reste :

```text
CurrentSaveVersion = 3
```

Les migrations et politiques de restauration avancées restent prévues pour MON15.6.

---

## 9. Hors périmètre confirmé

MON15.1 n'effectue pas :

- d'attribution d'XP à la mort d'un monstre ;
- de lecture runtime de `ExperienceReward` ;
- de level-up automatique ;
- de recalcul de PV/mana après gain de niveau ;
- de modification du loot ;
- de compétence, don, talent ou spécialisation ;
- de déblocage de sort/capacité ;
- d'interface de montée de niveau ;
- de modification de WBP, `.uasset` ou `.umap`.

Ces comportements appartiennent à MON15.2+.

---

## 10. Tests MON15.1

Suite dédiée :

```text
Grimrock.RPG.MON15.1.ProgressionCurve
Grimrock.RPG.MON15.1.LevelFromExperience
Grimrock.RPG.MON15.1.ProgressionBoundaries
Grimrock.RPG.MON15.1.ExistingCharacterState
```

Résultats UE5.5.4 fournis le 15 août 2026 :

```text
Grimrock.RPG.MON15.1.ProgressionCurve          Success
Grimrock.RPG.MON15.1.LevelFromExperience       Success
Grimrock.RPG.MON15.1.ProgressionBoundaries     Success
Grimrock.RPG.MON15.1.ExistingCharacterState    Success
```

Les tests sont indépendants des WBP, maps et DataAssets de production.

Ils couvrent les seuils, les bornes, la monotonie, la reconstruction du niveau, la cohérence d'un `FGridCharacterInventoryState` existant et l'absence de mutation de ses attributs, inventaire et hotbar par les helpers purs.

### Non-régression création de personnage

La même validation fournie contient également :

```text
Grimrock.CharacterCreation.CC2.CreateInitialCharacter          Success
Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically  Success
Grimrock.CharacterCreation.CC2.RejectSecondCreation             Success
```

Ces résultats confirment que MON15.1 ne casse pas les contrats CC2 couverts par cette suite.

---

## 11. Validation MON15.1

La porte de sortie MON15.1 est franchie sur la base des résultats UE5.5.4 fournis :

1. les quatre tests `Grimrock.RPG.MON15.1.*` réussissent ;
2. les trois tests `Grimrock.CharacterCreation.CC2.*` exécutés en non-régression réussissent ;
3. aucun comportement MON15.2 n'a été introduit prématurément ;
4. aucun `.uasset`, `.umap` ou WBP n'a été modifié ;
5. aucun changement du format SaveGame n'a été introduit.

MON15.1 est donc **VALIDÉ ET CLOS**.

Le prochain sous-jalon autorisé est :

```text
MON15.2 — Attribution XP après combat
```
