# ARM-CLEAN06.4X.16.4 — Préflight atomique des règles réelles (sans écriture)

**Contexte :** X.16.1–X.16.3 ont rendu possibles les
`CanonicalItemId` des règles d'acceptation, leurs éditions
par Selected Object et les contenus initiaux. X.18 a
corrigé le comportement lumineux de la torche et a
été validé par UE5.5.4 et en PIE.

## Périmètre

`FGridReceptacleAcceptanceMigrationPreview` construit
une **proposition transitoire**, jamais une écriture,
pour une liste `AcceptedItems` donnée. Il vérifie
chaque entrée sans changer son pointeur :
- identité canonique connue + pointeur absent/concordant :
  conserver strictement l'identité existante ;
- pointeur historique connu de `DA_ItemSystem` :
  proposer le `CanonicalItemId` exact sans inventer de niveau ;
- source vide, famille inconnue, identité canonique inconnue
  ou pointeur contradictoire : **bloquer toute la liste** ;
- garder ordre, doublons, cardinalité et références anciennes ;
  aucune conversion partielle possible.

Le moteur conserve autorité sur `bAcceptAnyItem`,
`MaxContainedItems`, `bOverrideReceptacleRules`, les GUID
et les liens logiques. Le préflight ne touche à aucun
de ces champs.

Les serrures `AcceptedKeyItems` / `AcceptedKeyIds`
sont comptabilisées **à part** : la migration des clés
ne doit pas être mêlée aux listes des réceptacles.

## Preuves

Deux tests :
- `Grimrock.RPG.ARM.CLEAN06_4X164.AtomicAcceptedItemMigrationPreview`
  vérifie l'ordre, les doublons, la non-mutation,
  l'atomicité et le refus des sources manquantes/inconnues ;
- `Grimrock.RPG.ARM.CLEAN06_4X164.RealReceptacleRulesDryRun`
  relit les quatre vrais niveaux et les vraies définitions
  via AssetRegistry, mesure sources, listes bloquées,
  pointeurs/ID de clés, et **n'enregistre aucun asset**.

Le journal Windows UE contient les **comptages réels** ;
le simple statut SUCCESS ne révèle pas les valeurs.
Une suite verte démontre l'absence d'écriture et les
contrats de preview, mais ne prouve pas que tous les
vrais assets soient déjà convertibles.

**Aucun** `.uasset`, `.umap`, `.sav`, WBP ou Blueprint
n'est écrit. Ne pas réexécuter ARM-CLEAN06.4W/X.5.
Avant une migration binaire ultérieure, récupérer le
résumé détaillé du test, isoler chaque source bloquante,
effectuer des copies de sauvegarde vérifiables et
contrôler la réouverture ainsi que les énigmes en PIE.
