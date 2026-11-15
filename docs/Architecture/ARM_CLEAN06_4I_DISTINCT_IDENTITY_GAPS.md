# ARM-CLEAN06.4I — Identifier les ItemId distincts réellement manquants

**Parent vérifié :** ARM-CLEAN06.4H,
`aedd0d06a23ba59da9a05b89221979246e8ecbe0`,
les trois checks Windows SUCCESS, issue
https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/141.

## Périmètre observé en UE5.5.4

06.4H a chargé les vraies données de 4 niveaux et de la palette :

| Famille | Références | ID connu du registre | ID absent |
|---|---:|---:|---:|
| Objets libres `LooseItemInstances` | 44 | 31 | 13 |
| Contenus initiaux `ReceptacleInitialContent` | 18 | 1 | 17 |
| Palette physique directe | 33 | 23 | 10 |
| **Total** | **95** | **55** | **40** |

Ces **40 références** ne correspondent pas nécessairement à
40 `ItemId` différents, et une même identité peut être
associée à plusieurs anciens assets. On ne doit donc pas
créer arbitrairement 40 nouvelles définitions.

## Commandlet et garanties

`UGridArmClean064IIdentityGapCommandlet` lit
`world-reference-private.json` produit par 06.4H :
les 95 chemins d'assets, ItemId, niveaux, quantités,
GUID et états sont les entrées exactes et **fermées**.
Le script `Scripts/AuditARM_CLEAN06_4I_DistinctItemIds.ps1`
vérifie le schéma, le SHA-256 du rapport signé par 06.4H,
le verrou de conversion fermé et les 84 fichiers binaires
de 06.4G **en source et en sauvegarde privée**.

Unreal Editor 5.5.4 charge ensuite :
- les 95 définitions historiques référencées ;
- le vrai `DA_ItemSystem` et ses 23 définitions canoniques ;
- les propriétés sérialisées `FProperty` de chaque
  `UGridItemDefinitionAsset`, exportées sans interprétation.

Le commandlet **ne crée, ne modifie et ne sauvegarde** aucun
DataAsset, niveau, palette, Blueprint ou SaveGame.
Il vérifie que tous les `ItemId` et états concordent avec
le rapport 06.4H et rejette une disparition de source.

### Distinction des résultats

1. **Identités manquantes distinctes :** combien d'`ItemId`
   réellement absents du registre pour les 40 occurrences.
2. **Définitions sources multiples :** combien d'identités
   manquantes ont plusieurs vieux assets comme sources.
3. **Conflits UPROPERTY :** si deux assets partagent un
   `ItemId` mais ont des propriétés historiques divergentes,
   le rapport privé conserve l'identité et le champ témoin ;
   aucun choix automatique n'est permis.
4. **N0 et N1–N20 :** répartition des niveaux d'instances
   réellement authorés, sans remplacer N0 par N1.
5. **Catégorie armes / non-armes :** déduite des champs
   historiques réels, pas d'une convention de nommage.
6. **IDs canoniques existants :** comparaison limitée de
   DisplayName, Description, Weight, Tags, archétype,
   Icon et WorldMesh hors armes ; les divergences sont
   des questions d'authoring, pas des migrations.
7. **Source exacte :** toutes les UPROPERTY historiques,
   y compris politiques de lancer, physique, sprites,
   SFX, VFX et informations d'équipement, sont conservées
   en privé en vue d'un futur mapping champ par champ.

Un `ItemId` identique n'est **pas** une preuve de même
action, masse Chaos, politique de mains, coût PA, visuel,
ni règle de pression ou d'utilisation en puzzle.

## Publication et sécurité

Le commit dont le sujet exact est
`chore(ARM-CLEAN06.4I): count distinct unresolved ItemIds and preserve source fields`
déclenche le contrôle seulement après Build + Automation
`Grimrock`.

L'issue publique `[ARM-CLEAN06.4I]` expose **uniquement
les compteurs** (IDs distincts, conflits, N0/fixes,
armes/non-armes). Aucun `ItemId`, nom d'item, chemin
de source, GUID, `FText` ou propriété concrète ne sort
du dossier privé `Runs`.

Le verrou final garde :
`migration_authorized=false`,
`allowed_binary_writes=[]`,
`allowed_package_deletions=[]`,
`allowed_savegame_writes=[]`.

Un succès de `UE5.5.4/ARM-CLEAN06.4I` signifie
que l'inventaire a été généré, **pas** que les nouvelles
définitions peuvent être écrites.

## Étape suivante

Regrouper les identités distinctes par familles d'authoring
(canoniques non-armement, munitions, consommables, armes),
faire la parité des champs **requis** dans les vraies
classes et catalogues, prouver les paramètres de lancer,
niveaux et états de réceptacles puis simuler les mutations
sur objets **transitoires UE Editor** avant toute sauvegarde
binaire. Les WBP/presets périphériques et les vraies
sauvegardes `.sav` auront besoin d'une protection dédiée.

`UI.csv → ST_UI → WBP → Game/fr/en` reste inchangé.
