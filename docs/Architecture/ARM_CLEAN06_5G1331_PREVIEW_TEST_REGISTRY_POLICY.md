# ARM-CLEAN06.5G.133.1 — Correction du test preview après refus de l'armure inconnue

**Parent:** `de043a5f1e18485b92b43db124170373dcf127bf` ; **diagnostic:** issue [#218](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/218), compilation réussie mais un test Automation en échec : `Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentPreviewReadModel`.

Le contrat 5G.133 interdit à une identité hors `DA_ItemSystem` de gagner des armures, bonus et résistances depuis un ancien DataAsset en jeu configuré. La fixture `GridARMCLEAN063AEEquipmentPreviewTests.cpp` attendait encore le contraire : armure +4, réserve 3 et résistance feu +14 avec un registre valide.

Le correctif rétablit la bonne frontière de test :

- **Registre configuré + ItemId inconnu** : aucun bonus, aucune réserve, aucune résistance, et seul l'ItemId technique est affiché.
- **Automation isolée sans registre** : l'ancien +4, le restant 3, la résistance 14 et le nom historique sont toujours vérifiés.
- Les niveaux ARM N1 et N20, les protections consommées, le nom du catalogue et l'invariance GUID restent couverts par le même test.

Ce lot **ne modifie que le test et la documentation**, ni production C++, ni WBP, ni DataAsset ni SaveGame. Attendre `SilentTask` et `LocalAuto` SUCCESS sur le nouveau SHA avant 5G.134.
