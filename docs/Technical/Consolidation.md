# Consolidation du prototype — état de référence

La consolidation a fixé l’architecture courante, retiré les autorités historiques, caractérisé les comportements et réduit les coûts mesurés et l’outillage de transition. Ce document porte les conclusions durables. Les relevés intermédiaires et les inventaires détaillés sont [figés au SHA CLEANUP06](https://github.com/PhilippeMartin1964/GrimrockPrototype/blob/2941d211ded3ffca873065d93eb05a9a2c51c7b3/docs/Technical/Consolidation.md).

## Clôtures et décisions

| Lot | SHA de clôture | Résultat |
| --- | --- | --- |
| CLEANUP01 | `420dce2ee4f55c3630e8a99fa7ae9d77299a461e` | Modèle relationnel et inventaire initial. Validation de référence de 2 044 scénarios avec 48 avertis. La recette ARM-CLEAN06 a clos les preuves de retrait, de persistance et de compatibilité. |
| CLEANUP02 | `176bb664116181ba91078f1067feddba94358f60` | Nomenclature et 53 déplacements natifs. Identités durables protégées par des redirections exactes et 50 alias Primary Asset IDs. Les trois collisions ont été renvoyées à CLEANUP04-D. |
| CLEANUP03 | `36205b2d02e511c61119e5e3acf7ca2f85e49332` | Audit, priorités et plan A–G. Séparation entre coût démontré, risque à profiler et dette supposée. Le détail et la matrice des candidats sont conservés dans l’instantané. |
| CLEANUP04 | `2a8b78bc3bef9bbdc1e53c5913ab6df22bbd04c8` | A–G clos. 2 067 scénarios, huit B.1, 70 Blueprints, 53 assets, sept copies v27 et parité indépendante. Build/Cook/Stage/Package/Archive Shipping et contrôle des 2 112 entrées de référence. |
| CLEANUP05 | `29e89de112b6de8796c7ad0a8861437dc76be600` | Profilage D3D12 mesuré. Regroupement ciblé des rafraîchissements du HUD persistant. |
| CLEANUP06 | `2941d211ded3ffca873065d93eb05a9a2c51c7b3` | Validation Windows permanente. 59 → 33 scripts, préflight médian 1,31 → 0,27 s. Les deux statuts sont SUCCESS. 2 067 réussites, zéro échec, 48 scénarios avertis sous LocalAuto, mêmes résultats par scénario que CLEANUP05. |

### Décisions C++ à conserver

- A (`260ba8863364015eef135f34c1d15391402cfa21`) retire D01–D06. D07 (`FRPGCharacterPortraitOption`) reste public et réfléchi. La recherche textuelle ne suffisait pas à démontrer que son retrait était sûr.
- B (`02e2248583b49c0b6a2147d30d1e81836ab19044`) partage les invariants de rotation, ressources, combat et Save. B.1 (`f02bcb6f16671d5e504666c92d045e0fd21e9954`) fixe les transactions réentrantes par `CharacterId` stable, snapshots indépendants, réservations RAII, refus `TransactionConflict` et rollback contrôlé. Save/Continue/New Game refusent un état en transit. Les huit tests garantissent identités, quantités, propriétaires et chronologie.
- C (`40a8a7af62ace6953f7ba763af8fc4f08ba09371`) partage les helpers privés Editor de sauvegarde, compatibilité Lua et recherche d’acteur lorsque les contextes concordent.
- D.16 (`1495d62273809bcf3ad4a36c8be5b7d504fe8f0d`) ferme D08–D13. `UGridItemDefinitionAsset`, `UGridWeaponCatalogueAsset` et `UGridWeaponVisualCatalogueAsset` désignent les classes actives. Structures de progression et identités UHT préservées. Les deux types Asset Manager gardent leurs chaînes `GridCanonical…`. La barre rapide générique N0 (`617679bc6ffa83a5b29841646f9e57284cf2f30e`), le parchemin offensif QuickItem et les projections Torch/Book/Jewelry suivent les autorités actuelles.
- E (`3e65a988e431cfab6bec048a818c47826f48791f`) réutilise la résolution d’item au sein des projections UI et partage les mappings d’actions équivalents. Les diagnostics et les politiques distinctes, dont la reconnaissance de `DungeonTrap`, gardent leurs contrats.
- F (`89fdc7ac6fb3eecc19bd65eff2689706edb92174`) partage collecte et ordre déterministe des monstres et résolution du RuntimeActor, puis simplifie l’orchestration. Les refus, notifications, contrôles de possession et ordre des mutations restent en place.
- G ferme les interfaces et dépendances. Editor passe de 20 publiques/0 privées à 9/11. Runtime reste 8/3. GrimrockLua reste 2/0. Conserver les frontières de modules, `bUseUnity=false` et les réglages de compilation. U10 et les renommages purement esthétiques sont abandonnés comme refactorisations autonomes.

Relire le [modèle des objets](../Architecture/DataModel.md), le [contrat normatif](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) et la [recette utilisateur D.16](../History/ARM_CLEAN06.md#clôture-et-compatibilité).

## Références de preuve et protections

- `DA_ItemSystem` et ses quatre catalogues. L’audit natif exécuté sur CLEANUP06 constate 48 définitions, dont 22 ARM, 53 packages actifs, 14 actions détaillées et 466 couples item/niveau valides. Les 82 archétypes mécaniques incluent des lignes sans objet matérialisé. [Structure réelle](../Architecture/DataModel.md)
- Oracle indépendant du commit `c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95`, SHA-256 `03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940`. 1 640 profils dont 260 passifs, zéro écart numérique sur l’export séparé. Les preuves 2A/4A/4E gardent leurs empreintes propres. [Lecteur et commande en lecture seule](../SCRIPTS_REFERENCE.md#controles-specialises)
- SaveGame v27 exact-match, sept copies chargées et round-trips conduits lors de CLEANUP04. Les originaux et les deux Readables `DA_BookPage_Test` et `DA_Scroll_Test` sont protégés par empreintes. Les 3 ClassRedirects, 53 redirections d’objet et 50 alias préservent les références légitimes. Les 48 packages d’ancienne autorité restent exclus.
- Validation manuelle Shipping D.16 du 24 novembre 2026, dix parcours validés sur dix. Elle ne résout pas l’anomalie visuelle de ciblage reportée. La compilation de Blueprints et le passage NullRHI restent des preuves techniques. [Historique des parcours](../History/ARM_CLEAN06.md#clôture-et-compatibilité)
- Les reçus indépendants et les données privées restent sous `D:/GrimrockNativeSnapshots/` et dans les runs du poste Windows. La vérification de profils sur CLEANUP06 est disponible dans `CLEANUP06/profiles.log`. Le packaging de référence et les audits binaires sont dans `CLEANUP04G/` et `CLEANUP04D16/`.

## Performance et outillage permanents

CLEANUP05 a mesuré deux captures nominales en standalone Development D3D12 1080p Haut sur Ryzen 7 9800X3D/Radeon RX 9070 XT. Le p95 du tick est de 8,71 à 9,40 ms sur la capture finale, pour un seuil de 33,33 ms. Continue à froid 1,41–1,67 s, à chaud 1,19–1,21 s et New Game 1,09–1,10 s. Development Editor à froid 292,13 s, incrémental HUD 17,28 s, sans modification 0,41 s. Entre captures comparables, la fin de tour médiane passe de 57,02 à 40,99 ms et les rafraîchissements HUD combat de 133 à 10. Le gain est borné à l’opération mesurée. L’attente de rendu d’environ 110 ms en phase RPG/flux reste sans cause attribuée. Les captures Development ne certifient pas le rendu Shipping.

CLEANUP06 a retiré 27 scripts de migration et d’authoring sur échec et ajouté la garde permanente des lecteurs/cook d’huile. Les outils actifs sont décrits dans la [référence des scripts](../SCRIPTS_REFERENCE.md). La [tâche silencieuse](../Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md) compile Editor et Shipping et exécute Grimrock dans le worktree dédié. Elle refuse les modifications locales et exige un SHA et un résultat de fin cohérents. Le bootstrap préexistant altère quelques accents dans les diagnostics, sans modifier la sélection du SHA vérifiée.

## CLEANUP07 et CLEANUP08

CLEANUP07 fixe l’autorité documentaire par domaine. Le [point d’entrée](../README.md), la [synthèse d’architecture](../Architecture/PROJECT_SYNTHESIS.md) et la [roadmap](../Design/PROJECT_COMPLETION_ROADMAP.md) décrivent le projet courant. Les preuves uniques restent datées. Le détail des anciens jalons est consultable dans Git.

Le corpus passe de 1 442 à 1 303 fichiers, dont 1 327 à 1 188 Markdown. Le lot regroupe 141 rapports et crée deux pages dans `History/`. Les 258 encadrés historiques corrigés datent les anciens schémas sans réécrire les preuves. Aucun doublon exact n’a été trouvé. Les reçus de retrait 5A et 5H.25, les recettes distinctes et les décisions datées restent consultables.

| Famille condensée | Destination |
| --- | --- |
| 140 notes ARM-CLEAN06.5G | [Synthèse de migration](../History/ARM_CLEAN06.md) et [originaux figés](https://github.com/PhilippeMartin1964/GrimrockPrototype/tree/2941d211ded3ffca873065d93eb05a9a2c51c7b3/docs/Architecture) |
| Anciens schémas et guides ARMEMENT | [Modèle courant](../Architecture/DataModel.md) et [contrat d’édition](../Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md). Entrées historiques conservées pour leurs consommateurs. |
| Validation MON18.4 | [Contrat et recette de ciblage](../Design/MON18_4_TARGETING_INTEGRATION.md#validation-ue554) |
| Inventaires de transition, cartes détaillées et journal UI agrégé | Références actuelles par domaine et instantanés immuables, accessibles depuis leurs documents de synthèse. |

**CLEANUP08 reste ouvert.** La recette générale doit vérifier le produit de référence et les parcours intégrés avant reprise fonctionnelle. La consolidation documentaire n’ajoute aucune validation manuelle ni changement de gameplay.
