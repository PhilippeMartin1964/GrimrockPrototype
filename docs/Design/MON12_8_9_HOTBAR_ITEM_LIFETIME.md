# MON12.8.9 — Unicité des armes et durée de vie des consommables

## Résultat

MON12.8.9 fixe deux invariants autoritaires de la barre personnelle :

1. une instance d'arme ne peut être assignée qu'à un seul slot à la fois ;
2. une même définition de consommable ne peut elle aussi occuper qu'un slot ;
3. chaque consommation acceptée retire l'action de la barre.

Ces règles appartiennent à `UGridPartyInventoryComponent`. Elles s'appliquent
donc au glisser-déposer du HUD, aux touches numériques, à la sauvegarde et à
tout futur écran qui utilisera les mêmes opérations.

## Dépôt répété d'une arme

Une arme équipée est identifiée par `PreferredSourceRuntimeId`. Lors d'une
affectation, `SetCharacterCombatHotbarBinding()` cherche ce même identifiant
dans les neuf autres slots :

- l'ancien binding est effacé ;
- le binding est écrit dans le slot cible ;
- une seule notification d'inventaire est diffusée.

Le dépôt se comporte donc comme un déplacement, même lorsqu'il provient de
nouveau du menu d'équipement. L'arme elle-même reste équipée et n'est ni
dupliquée, ni transférée, ni déposée au sol.

Le déplacement ou l'échange d'un raccourci existant continue d'utiliser
`MoveOrSwapCharacterCombatHotbarBinding()`.

## Consommation

Les potions, parchemins, shurikens non équipés et autres actions `QuickItem`
sont identifiés par `SourceDefinitionId`.

- déposer de nouveau la même définition déplace son binding vers le slot
  cible ;
- toute consommation acceptée remet le binding à vide ;
- les exemplaires restants demeurent dans l'inventaire sans être assignés ;
- une action refusée conserve le binding et toutes les quantités ;
- ajouter plus tard une nouvelle pile ne restaure aucun ancien binding.

Le `TurnManager` notifie l'inventaire après chaque consommation acceptée. Le
HUD reçoit donc l'état final et vide le slot sans attendre sa réouverture.
Une action refusée ne consomme rien et ne modifie aucun binding.

## Sauvegardes existantes

La restauration normalise les anciennes barres avant validation :

- la première occurrence d'une arme est conservée et ses doublons sont
  effacés ;
- les bindings `QuickItem` sans quantité disponible sont effacés.

Les autres bindings et les dix indices restent inchangés. Aucune nouvelle
version de sauvegarde n'est nécessaire, car la structure sérialisée ne change
pas.

## Widget Blueprint

Aucune modification de WBP n'est nécessaire. Les slots restent les mêmes ;
seul leur modèle persistant est corrigé et le rafraîchissement existant de
`WBP_GridCombatHud` projette immédiatement le nouvel état.

## Tests automatisés

Filtre principal :

```text
Grimrock.Monsters.MON12.8.9
```

Il couvre :

1. le second dépôt de la même arme et l'unicité de son `RuntimeObjectId` ;
2. le déplacement du binding lors du second dépôt de la même définition ;
3. la suppression du binding après une consommation acceptée alors qu'un
   exemplaire reste en inventaire ;
4. la suppression après consommation du dernier exemplaire ;
5. l'absence de réactivation automatique avec une pile de remplacement ;
6. la normalisation des anciennes sauvegardes.

Les filtres `Grimrock.Monsters.MON12.8.4` et
`Grimrock.Monsters.MON12.8.7` vérifient également la disparition effective
d'une potion et d'un shuriken depuis le HUD.
