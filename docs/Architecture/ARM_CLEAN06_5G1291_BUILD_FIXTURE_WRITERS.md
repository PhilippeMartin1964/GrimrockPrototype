# ARM-CLEAN06.5G.129.1 — Compilation des fixtures après retrait physique des caches

**Parent :** `6abc1c6bb5823334b2bbb78151c6b5e3f631093c`; **diagnostic Windows :** [issue #217](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/217), `BUILD_FAILED`, huit erreurs `C2039` sur `FGridItemInstance`.

Le compilateur a trouvé des affectations des anciens membres `DisplayName` ou `Weight` dans des tests de hotbar et d'infobulle. La compilation de 5G.129 a donc échoué avant la phase Automation. Les huit affectations sont supprimées exactement aux emplacements signalés :

- `GridHotbarContext01Tests.cpp` : `Key.DisplayName` (ligne antérieure 188).
- `GridMonsterMON128HotbarTests.cpp` : `Potion.DisplayName` (3), `Weapon.DisplayName` (1), `EquippedWeapon.DisplayName` (1).
- `GridUIItem01Tests.cpp` : `Equipped.DisplayName` et `Equipped.Weight`.

Les définitions historiques de test conservent leurs données de présentation et de masse ; elles ne sont pas des caches d'instance. Aucun schéma réfléchi, propriété sauvegardée, Blueprint, asset binaire, texte visible ni ligne d'assertion de gameplay n'est changé. L'effacement physique dans `GridInventoryTypes.h` reste en place, garantissant qu'un nouvel accès C++ à ces membres provoque désormais une erreur de compilation.

**Gates :** `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success` sur le nouveau SHA. Attendre le cycle de compilation + suite `Grimrock` ; ne pas marquer 5G.129 comme validé indépendamment de ce correctif.
