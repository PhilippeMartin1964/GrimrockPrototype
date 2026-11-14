# ARM-CLEAN06.3AI — Identités, niveaux et métadonnées des réceptacles

## Constats

`AGridReceptacleActor::AddContainedItem` lisait encore
`TryGetWeaponCatalogueProfile`, `Weight` et `DisplayName` des
anciens `DA_Weapon_*` ; puis
`TryInsertItemInstanceFromCursor` réécrivait les valeurs avec des
snapshots du curseur, parfois incohérents avec `DA_ItemSystem`.
`TryTakeItemAtIndex` réécrivait également le poids et le nom avec
la définition historique avant l'inventaire.

Le point sensible : **0 n'a pas la même signification** pour un
placement nouvellement authoré (héritage de niveau d'arme) et pour
un `ItemLevel` **déjà persisté** (une arme N0 est invalide).

## Modèle de lecture transitoire

`FGridReceptacleItemMetadataReadModel::Resolve` reçoit
`UGridItemSubsystem`, `ItemId`, `ItemLevel`, l'ancienne
définition éventuelle, et `bAllowAuthoredDefault`.

- ARM connu N1–N20 : niveau, nom et masse issus du **seul**
  `DA_ItemSystem` à son niveau réel.
- Nouveau contenu de niveau 0 : l'ancienne définition fournit
  seulement le `DefaultWeaponItemLevel` **pour une arme**,
  dont le niveau N1–N20 est alors vérifié dans le registre.
- Curseur et sauvegarde : `bAllowAuthoredDefault=false` ;
  un ARM persisté à N0 échoue **avant** mutation d'un réceptacle,
  sans promouvoir silencieusement N1.
- Flèches et objets non-ARM connus : **N0 réel**, indépendamment
  d'un ancien `bUseWeaponCatalogue` contradictoire.
- Objets inconnus du registre : anciens niveaux et métadonnées
  conservés provisoirement, y compris entrées ID-only.
- Objets connus invalides : aucun fallback aux données du vieux
  catalogue, sortie vide et aucune création de contenu.

Le retour d'insertion transporte désormais le **niveau fixé**
réel de l'objet contenu. Une entrée de sauvegarde reste à son
niveau déjà fixé, sans recalcul sur son défaut auteur.

Les éventuels `DisplayName` personnalisés conservés dans les
snapshots du curseur/sauvegarde ne sont **pas écrasés** avant
la conversion 06.4. En revanche, les messages de pose/retrait
présentent le nom canonique quand l'`ItemId` est enregistré.
La masse statique en cache vient toujours du registre dans ce
cas. États de lumière, identités GUID, quantités, informations
lisibles et offsets restent préservés.

## Tests

Nouveau test
`Grimrock.RPG.ARM.CLEAN06_3.CanonicalReceptacleMaterialization` :
vraie racine `DA_ItemSystem`, ARM014/031/075/081 N1/N20,
héritage N6, rejet N0 persisté, ID-only, flèches N0 et
Legacy ; anciennes données volontairement contradictoires.
Les tests d'intégration déjà présents
`Grimrock.RPG.ARMEMENT01.8.Receptacles.*` vérifient la pose,
le transfert, le retour en inventaire et le Save/Restore du
niveau fixe et du GUID.

**Aucun** DataAsset, `.uasset`, `.umap`, SaveGame,
composant visuel, Niagara ou quantité de stack n'est converti
ou supprimé.

## Texte et localisation (workflow WBP)

`docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relu.
Le contenu métier `DisplayName` vient du DataAsset canonique ;
les messages utilisent les `NSLOCTEXT` existants
`ReceptacleItemInserted` et `ReceptacleItemRemoved`.
Aucun nouvel élément, texte, clé ou Designer WBP.
**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**

Après push : requérir `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` verts sur le SHA exact ; le rendu
des messages en PIE/fr/en reste à vérifier séparément.
