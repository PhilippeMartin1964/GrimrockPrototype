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

## Démarrage runtime

Le lancement automatisé de `L_GrimrockRuntime` avec `UnrealEditor-Cmd`, en mode jeu sans rendu ni son, se termine avec un code de sortie `0`. Le monde est créé avec `BP_GrimrockGameMode` et aucun plantage n'est observé.

Le journal signale :

- l'absence de `PlayerStart` utilisable ; la carte contient toutefois un pawn placé ;
- une dépendance manquante de `BP_Item_Torch` : `/Game/F_FreeFlameFx_Pack/FX/NS_Flame_8_Torch` ;
- un conflit de nom d'exposition Python autour de `EGridItemTransferResult`, sans incidence sur cette passe runtime.

La dépendance Niagara manquante bloque la validation visuelle complète de la torche. Elle doit être restaurée, remplacée ou retirée de `BP_Item_Torch` selon l'intention du projet.

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

## Conclusion

Le socle statique du donjon de test est exploitable : les assets principaux se chargent, le niveau ne contient aucune erreur de validation et le runtime démarre sans plantage. La passe ne permet pas de conclure sur les interactions jouées tant qu'une session PIE manuelle n'a pas couvert la checklist et que la dépendance visuelle manquante de la torche n'a pas été traitée.
