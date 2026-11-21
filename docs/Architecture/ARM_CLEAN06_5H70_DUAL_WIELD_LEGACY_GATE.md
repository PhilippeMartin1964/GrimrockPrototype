# ARM-CLEAN06.5H.70 — Double maniement : aucun repli historique en runtime

## Portée

`FGridDualWieldPolicy::ResolveHandSource` avait déjà une voie canonique pour chaque `ItemId` connu du registre, mais un identifiant **inconnu** pouvait encore emprunter `FindItemDefinition`, `CombatActions` et `ResolveEquippedOffensiveProfile` en jeu. Cette seconde autorité contredisait la règle « zéro legacy dans le runtime final ».

Le lot protège toute cette branche sous `WITH_DEV_AUTOMATION_TESTS`, avec vérification stricte de `GIsAutomationTesting` et **absence du registre**. En Shipping, le dual-wield retourne `false` lorsque ses deux sources ne proviennent pas des identités ARM canoniques ; aucun ancien profil d'attaque ne peut ressusciter par défaut.

## Tests et garanties

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalDualWieldIndependentLevels` conserve la paire ARM001 N3 / ARM013 N19, niveaux et GUID, les attaques indépendantes, les présentations, 3 PA et 50 % de dégâts OffHand. Un nouveau cas refuse un ItemId inconnu avec registre configuré, puis rétablit la paire canonique. Les tests MON12 du double maniement et des quatre sons distincts, déjà convertis en H.65/H.66, sont conservés.

Aucun changement de représentation WBP, aucune nouvelle chaîne UI, aucun impact direct sur une sauvegarde, `.uasset`, `.umap` ou son. `UI.csv → ST_UI → WBP → Game/fr/en` inchangé. La branche historique continue d'exister en source **uniquement** pour les fixtures Automation sans GameInstance/registre.

H.71 nécessite SilentTask et LocalAuto `SUCCESS` sur le SHA H.70, ainsi que le contrôle global `Grimrock`. Le retrait des 53 packages historiques et le réaudit des sept sauvegardes ne sont pas autorisés par ce changement.
