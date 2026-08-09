# MON12.11 — Validation complète des raccourcis

MON12.11 ferme le jalon MON12 avec une matrice de non-régression couvrant
les dix raccourcis configurables par personnage.

| Scénario | Test autoritaire |
|---|---|
| Déplacement d'une arme et échange de deux slots occupés | `Grimrock.Monsters.MON12.11.HotbarValidation` |
| Consommable accepté, quantité décrémentée et dernier exemplaire retiré | `Grimrock.Monsters.MON12.8.4.QuickItemEffectAndUnassignment` et `Grimrock.Monsters.MON12.8.7.InventoryThrowableHotbar` |
| Action refusée sans consommation, sans dépense de PA et sans perte du raccourci | `Grimrock.Monsters.MON12.11.HotbarValidation` |
| Sérialisation SaveGame et restauration des identités stables | `Grimrock.Monsters.MON12.8.1.SaveMemoryRoundTrip` et `Grimrock.Monsters.MON12.11.HotbarValidation` |
| Barre indépendante pour chaque personnage | `Grimrock.Monsters.MON12.11.HotbarValidation` |
| Fin du combat : actions conservées mais non exécutables | `Grimrock.Monsters.MON12.11.HotbarValidation` |
| Ciblage manuel, validation et annulation | `Grimrock.Monsters.MON12.10.ActionPaletteTargeting` |
| Retrait du raccourci sans déplacement ni suppression de l'objet source | `Grimrock.Monsters.MON12.11.HotbarValidation` |

Le test MON12.11 est transversal : il échange une arme et une action abstraite,
provoque un refus autoritaire, restaure un instantané sauvegardé, change de
personnage, retire le raccourci, puis termine le combat. Les tests spécialisés
MON12.8–12.10 restent responsables des détails propres aux consommables, à la
sérialisation binaire et au ciblage de cellule ou de zone.
