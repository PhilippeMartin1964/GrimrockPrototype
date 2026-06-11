# Rapport de régression du donjon de test

## Résumé

La passe couvre l'inspection des assets, la validation du niveau et un démarrage runtime automatisé sans rendu. La carte atteint le monde de jeu sans plantage et la validation ne signale aucune erreur bloquante.

Les interactions clavier, souris et interface n'ont pas été exécutées en PIE : l'environnement automatisé utilisé peut charger la carte et démarrer le runtime, mais ne permet pas de piloter ni d'observer fiablement ces interactions. Elles restent donc marquées **NON COUVERT** dans la checklist, et non comme des échecs.

## Configuration observée

La carte `/Game/GrimrockPrototype/Maps/L_GrimrockRuntime` contient :

- un `BP_GridLevelRuntimeActor` configuré directement avec `DA_GridLevel_00` ;
- aucun `DungeonAsset` sur cet acteur ;
- un `BP_GrimrockPartyPawn` placé dans la carte ;
- aucun `AGridLevelEditorActor`, ce qui est normal pour une carte runtime.

Le niveau est une grille `32 x 32` de `200 uu`, avec un départ en `(28, 23)` orienté au nord. Il contient `16` objets et `3` liens. Les mécanismes représentés sont des portes, un bouton secret, un levier, trois réceptacles, une inscription et un escalier. Il ne contient ni item placé, ni plaque de pression, ni trigger.

## Validation du niveau

Résultat : `0` erreur, `11` avertissements, `1` information.

| Catégorie | Sévérité | Nombre | Synthèse |
|---|---:|---:|---|
| Noyau | Avertissement | 2 | `DungeonAsset` absent ; cible de transition non vérifiable. |
| Palette | Avertissement | 7 | Cinq archétypes attendus absents de la palette ; deux identifiants `DoorStone` obsolètes. |
| Murs | Avertissement | 1 | `57` arêtes partagées directionnelles. |
| Liens | Avertissement | 1 | Un porte-torche émet `ItemRemoved` sans lien `ItemInserted`. |
| Archétypes | Information | 1 | `Button_Normal` définit un mesh ou matériau fixe inhabituel pour son type. |

Les arêtes directionnelles signifient que le résultat d'un déplacement peut dépendre de la cellule source. La première divergence signalée se trouve entre `(28, 20)` et `(28, 21)`, avec `None` d'un côté et `Solid` de l'autre.

Les deux objets qui utilisent encore `PaletteEntryId = DoorStone` sont les portes placées en `(28, 27)` nord et `(25, 30)` ouest.

## Démarrage runtime initial

Lors de la passe initiale, le lancement automatisé de `L_GrimrockRuntime` avec `UnrealEditor-Cmd`, en mode jeu sans rendu ni son, s'est terminé avec un code de sortie `0`. Le monde a été créé avec `BP_GrimrockGameMode` et aucun plantage n'a été observé.

Le journal signalait :

- l'absence de `PlayerStart` utilisable ; la carte contient toutefois un pawn placé ;
- une dépendance manquante de `BP_Item_Torch` : `/Game/F_FreeFlameFx_Pack/FX/NS_Flame_8_Torch` ;
- un conflit de nom d'exposition Python autour de `EGridItemTransferResult`, sans incidence sur cette passe runtime.

Cette dépendance historique a été remplacée dans `BP_Item_Torch` par le commit `bb9941e`. La section de suivi ci-dessous décrit le résultat de cette correction.

## Couverture fonctionnelle

Les trois liens du niveau couvrent :

- `Activated -> Open` ;
- `ItemRemoved -> Open` ;
- `Activated -> Toggle`.

Aucun lien conditionnel n'est présent. Les conditions par définition d'item, tag, type, quantité, poids et inversion ne sont donc pas couvertes par ce donjon. Il en va de même pour les plaques de pression, triggers, items placés dans le monde, inventaire plein, refus de réceptacle et portes verrouillées.

Les scénarios précis à rejouer manuellement sont listés dans [TEST_DUNGEON_PASS_CHECKLIST.md](TEST_DUNGEON_PASS_CHECKLIST.md).

## Correction isolée

`InferValidationCategory` classait certains avertissements de palette dans `Doors` ou `Receptacles`, car ces mots étaient testés avant `ObjectPalette` et `PaletteEntry`. L'ordre des tests a été corrigé dans `GridLevelEditorActor.cpp`.

Cette modification ne change aucune logique de gameplay ni le contenu du niveau. Une nouvelle validation confirme que les sept messages concernés sont maintenant classés dans `Palette`.

## Mise à jour torche / Niagara

Le commit de départ vérifié est `bb9941e061ed3b8c2849f5d20a578835b0500969` (`Adapt images and level to docs`).

La référence de `BP_Item_Torch` a bien été déplacée :

- ancien chemin absent : `/Game/F_FreeFlameFx_Pack/FX/NS_Flame_8_Torch` ;
- nouveau chemin : `/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch`.

Le Blueprint et le système Niagara se chargent, et aucun warning ne vise encore l'ancien chemin du système. L'intégration reste toutefois **partiellement résolue** : le nouveau système Niagara conserve deux dépendances secondaires absentes du dépôt :

- `/Game/F_FreeFlameFx_Pack/Materials/MI_Distortion_1` ;
- `/Game/F_FreeFlameFx_Pack/Materials/MI_Flame_8`.

Le démarrage automatisé de `L_GrimrockRuntime` se termine avec un code de sortie `0`, sans plantage. Les logs ne contiennent plus l'ancienne dépendance vers `F_FreeFlameFx_Pack/FX/NS_Flame_8_Torch`, mais produisent deux `LoadErrors` pour les matériaux ci-dessus. Aucun autre warning Niagara critique ni spam propre à la torche n'a été relevé.

La validation exécutée depuis `L_GrimrockEditor`, avec `DA_Dungeon_01`, `DA_GridLevel_00` et `DA_ObjectPalette_Default`, donne :

- `0` erreur ;
- `10` avertissements : `7` Palette, `2` Murs et `1` Liens ;
- `1` information : Archétypes.

Le niveau contient désormais `21` objets et `6` liens. Le warning du porte-torche qui émet `ItemRemoved` sans lien `ItemInserted` reste présent et cohérent avec la configuration actuelle.

Les tests PIE interactifs n'ont pas été exécutés. L'environnement automatisé permet de charger les assets et le runtime, mais pas de contrôler visuellement la flamme, son attachement, son retrait, son redépôt ou l'absence de double effet. Ces points ne sont donc pas marqués comme validés.

Une tentative de resauvegarde aurait supprimé les références manquantes sans fournir de matériaux de remplacement. Cette modification n'a pas été conservée, car elle aurait pu masquer le warning tout en rendant l'effet incomplet ou invisible. Aucun `.uasset` n'a été modifié pendant cette passe.

## Conclusion

Le socle statique du donjon de test est exploitable : les assets principaux se chargent, le niveau ne contient aucune erreur de validation et le runtime démarre sans plantage. La référence du système Niagara est corrigée, mais la validation visuelle reste bloquée par deux matériaux absents. Une session PIE ne pourra conclure sur la flamme et les transferts de torche qu'après restauration ou remplacement validé de ces dépendances.
