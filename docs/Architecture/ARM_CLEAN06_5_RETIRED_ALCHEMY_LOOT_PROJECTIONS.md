# ARM-CLEAN06.5 — Retrait des anciens plans de loot et d'alchimie

Les suites X.48, X.62, X.63 et X.65 gardent leurs noms Automation mais
interrogent la donnée réellement installée : 48 ItemIds dans DA_ItemSystem,
les tables de loot Gobelin/Rat, 11 QuickItems N0 avec leurs actions détaillées
et les trois kits de l'Alchimiste.

- X.48 vérifie les placements, réceptacles, palettes, clés et 5 loot N0
  sans reconstruire de candidat à partir d'anciens DA_Item_*.
- X.62 contrôle les 11 actions liées à leur source et refuse les collisions
  d'ActionId ayant des mécanismes différents.
- X.63 vérifie les 11 actions et le hotbar monde; la conversion en mémoire
  d'anciennes identités d'élixirs pour les sauvegardes v27 est conservée.
- X.65 vérifie que l'authoring des consommables historiques n'est plus
  autorisé avec une racine complète, refuse un état mixte et conserve
  les starters Grenadier/Transmuter en N0 via ItemSystem.

Les helpers de prévisualisation à usage unique
GridEditorAlchemyCutoverPlan, GridEditorAlchemyMigrationAudit,
GridEditorLootDefinitionProjection et GridEditorNativeStoneLootPlan
sont supprimés après conversion de leurs consommateurs.

GridEditorAlchemyActionIdentity et GridEditorAlchemySaveCopy sont
maintenus tant que les sept véritables sauvegardes privées v27 n'ont
pas passé le contre-audit, pour ne pas supprimer prématurément la
preuve du mapping des quatre identités d'élixirs.

Aucun package natif, WBP ou fichier .sav n'est modifié. Les Readables
des sauvegardes et les 53 candidats AssetRegistry restent protégés.
