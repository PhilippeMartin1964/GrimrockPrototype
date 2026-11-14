# ARM-CLEAN06.3AK — Parité des niveaux dans l'aperçu d'objets libres

**Dépendance native :** ARM-CLEAN06.3AJ, commit `08f56ec`,
build UE5.5.4 et suite complète `Grimrock` verts.

## Écart constaté

Après 06.3AJ, `AGridLevelRuntimeActor::AddPlacedItemActor`
détermine le type arme/non-arme uniquement via le
`DA_ItemSystem` pour les `ItemId` enregistrés.
`FGridEditorLooseItemPreviewReadModel` continuait pourtant
d'interpréter `ItemLevel=0` à partir de
`UGridItemDefinitionAsset::bUseWeaponCatalogue`.

Un ancien flag à `false` sur une vraie ARM affichait donc
un aperçu N0 invalide malgré un runtime qui matérialise correctement
N3 (ou un autre niveau par défaut authoré). Un flag à `true`
sur les flèches déclenchait dans l'éditeur une fausse promotion
N0→N3 que le gameplay ne fait pas. L'éditeur pouvait alors
présenter un mesh ou une décoration différent du vrai placement.

## Résolution

`FGridEditorLooseItemPreviewReadModel::Resolve` conserve sa
détection des `ItemId` présents dans le registre, puis délègue
le calcul **du niveau de placement authoré** à
`FGridWorldItemLevelReadModel::ResolveAuthoredPlacement`,
exactement comme `AGridLevelRuntimeActor::AddPlacedItemActor`.

- Vrai ARM N1–N20 : conservation de l'authored level.
- Vrai ARM authoré N0 : héritage **une fois** du
  `DefaultWeaponItemLevel` historique, validé par le
  registre. Ce défaut ne constitue pas une deuxième source
  de statistiques, seulement une transition du contenu existant.
- Vraies flèches : niveau N0 réel même si un ancien booléen
  est erroné ; aucun profil N1 inventé.
- Identité enregistrée au niveau invalide (N21, N0
  avec default ARM invalide, ou N1 sur les flèches) :
  aperçu refusé **sans retour au mesh historique**.
- Identité non enregistrée : comportement Legacy temporaire
  déjà présent en 06.3Z et inchangé.

Aucune modification au contenu du `LevelAsset`,
`FGridLooseItemInstance`, à la physique, aux objets
persistés, aux meshes, aux Blueprints ni aux sauvegardes.
Le modèle est une lecture pure d'un registre déjà configuré.

## Régression

Étendre le test existant
`Grimrock.RPG.ARM.CLEAN06_3.EditorLooseItemCanonicalPreview`
en conservant le vrai `DA_ItemSystem` et les cas N1/N20/N0
d'ARM054/031/075. Mettre volontairement
`bUseWeaponCatalogue=false` sur une arme et
`bUseWeaponCatalogue=true` sur des flèches pour prouver
l'autorité du type canonique ; ajouter défaut N0 invalide
et N1 invalide des flèches.

La suite `Grimrock` complète doit rester verte,
dont `CanonicalWorldItemLevelBySource` (AJ) et
les tests historiques `PlacedItems.*`.
Une capture dans `L_Dungeon` hors PIE reste utile pour
confirmer visuellement le mesh, distincte des tests
`-NullRHI`.

## Localisation

Aucun nouveau texte joueur ou Widget Designer.
**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
