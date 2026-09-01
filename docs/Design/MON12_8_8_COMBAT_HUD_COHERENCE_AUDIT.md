# MON12.8.8 — Audit de cohérence du HUD de combat

> **Obsolète depuis HOTBAR01.2 — Remove Legacy Action Palette (01.09.2026).**  
> La palette d’actions intermédiaire a été supprimée de l’architecture runtime. Ne pas recréer `Panel_ActionPalette` dans `WBP_GridCombatHud`. Les actions sont affectées directement depuis leur source métier (inventaire, Spellbook, futures pages de capacités) vers la barre `1–9,0`. `Panel_Targeting` reste actif pour les actions nécessitant une cible.


## Conclusion

MON12.7 et MON12.8 utilisent bien une seule chaîne autoritaire : le HUD projette
le catalogue du `TurnManager`, tandis que l'inventaire conserve uniquement les
dix identités persistantes. Aucun widget ne paie de PA, de mana ou d'objet.

Le contrôle des quatre assets versionnés montre cependant que plusieurs
fallbacks C++ compensent encore un Widget Blueprint historique. Le runtime reste
fonctionnel, mais trois WBP doivent être finalisés dans Unreal Editor avant de
pouvoir supprimer ces fallbacks.

| Widget Blueprint | État contrôlé | Mise à niveau |
| --- | --- | --- |
| `WBP_GridCombatActionPanel` | déjà nettoyé ; aucun `Button_MainHand` ou `Button_OffHand` | aucune modification structurelle |
| `WBP_GridCombatHud` | `Panel_Actions` est encore un `WrapBox`; `Panel_ActionPalette` et les widgets de ciblage sont absents | à modifier |
| `WBP_GridCombatHudAction` | `Text_ShortcutNumber` est absent | à modifier |
| `WBP_GridCombatHudInitiativeSlot` | `ProgressBar_Health` est absent | à modifier |

Les `.uasset` ne doivent pas être modifiés sans Unreal Editor 5.5.4.

## Mise à niveau de `WBP_GridCombatHud`

1. Remplacer le `WrapBox` nommé `Panel_Actions` par une `Horizontal Box` vide
   portant exactement le même nom et avec `Is Variable` activé.
2. Ajouter au-dessus de la barre un `WrapBox` ou une `Horizontal Box` vide nommé
   exactement `Panel_ActionPalette`, puis activer `Is Variable`.
3. Ajouter un panneau d'information initialement `Collapsed`, nommé
   `Panel_Targeting`, contenant :
   - `Text_TargetingInstructions` ;
   - `Text_TargetingCell`.
4. Conserver les trois classes :
   - `Party Member Panel Widget Class = WBP_GridCombatActionPanel` ;
   - `Action Widget Class = WBP_GridCombatHudAction` ;
   - `Initiative Slot Widget Class = WBP_GridCombatHudInitiativeSlot`.
5. Laisser `Panel_PartyMembers`, `Panel_Actions`, `Panel_ActionPalette` et
   `Panel_Initiative` vides : le C++ possède leurs enfants dynamiques.

Après cette modification, `HorizontalBox_Hotbar_Runtime` et
`Panel_ActionPalette_Runtime` ne seront plus nécessaires. Ils restent pour le
moment des protections de compatibilité avec l'asset versionné actuel.

## Mise à niveau de `WBP_GridCombatHudAction`

Ajouter un `TextBlock` nommé exactement `Text_ShortcutNumber`, avec `Is Variable`
activé, dans un coin du cadre. Le C++ y écrit `1` à `9`, puis `0`, et le masque
automatiquement lorsque le même WBP sert d'entrée de palette.

Conserver `Button_Action` hit-testable et ne créer aucun événement Blueprint :
clic, drag, drop, exécution et clic droit sont routés nativement.

## Mise à niveau de `WBP_GridCombatHudInitiativeSlot`

Ajouter une `Progress Bar` nommée exactement `ProgressBar_Health`, avec
`Is Variable` activé, sous le portrait. La valeur initiale peut être `1.0`; le
C++ écrit ensuite le ratio de PV et la couleur rouge.

`Text_State` est conservé, mais il n'affiche que `ACTIF`. Les entrées futures ne
répètent ni `Party / Monster`, ni `Waiting`.

## Clic droit sur un raccourci

Un slot de barre ne contient jamais l'objet. Il contient seulement un
`FGridCombatHotbarBinding` permettant de retrouver l'action au moment de son
exécution.

| Source du raccourci | Effet du clic droit |
| --- | --- |
| potion, parchemin ou shuriken d'inventaire | binding supprimé ; pile inchangée dans l'inventaire |
| arme en main droite ou gauche | binding supprimé ; arme toujours équipée |
| `Attack_Unarmed`, capacité ou sort | binding supprimé ; action toujours disponible dans la palette |

Il n'existe aucun transfert vers le monde, aucun appel de dépôt et aucune
consommation. Le tooltip du slot rappelle désormais cette règle.

## Nettoyage C++

- suppression de la copie `bCombatActive` et de la règle
  `bCollapseOutsideCombat` dans chaque panneau enfant ;
- suppression des rafraîchissements HUD redondants sur
  `OnPlayerAttackResolved`, `OnPlayerAttackRejected`, `OnActionStarted` et
  `OnActionCompleted` ;
- conservation des événements qui correspondent réellement aux données
  affichées : inventaire, état de tour, PAM, initiative, combattant actif,
  état/vie d'un combattant, phase et fin du combat ;
- masquage des libellés répétés `Waiting` et `Completed` dans les quatre
  panneaux de statut.

## Tests

Filtre ajouté :

```text
Grimrock.Monsters.MON12.8.8
```

Le test `ClearShortcutKeepsItemSource` vérifie qu'une désaffectation conserve la
pile d'inventaire, sa quantité, son identifiant runtime et l'arme équipée. Seul
le binding devient vide.
