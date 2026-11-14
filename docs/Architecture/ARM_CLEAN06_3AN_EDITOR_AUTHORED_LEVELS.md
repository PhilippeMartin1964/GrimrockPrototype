# ARM-CLEAN06.3AN — Validation canonique des niveaux authorés dans l'éditeur

**Base vérifiée :** ARM-CLEAN06.3AM, commit `7089121`, avec
`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` verts.

## Écart supprimé

Les prévisualisations des objets libres ont été mises en conformité en
06.3AK, mais les chemins d'édition écrivaient encore le niveau d'après
`bUseWeaponCatalogue` / `TryGetWeaponCatalogueProfile` de `DA_Weapon_*`.
Ce contrat obsolète bloquait une arme enregistrée dont le booléen ancien
était faux et pouvait autoriser un niveau erroné pour les munitions.
Le champ `ItemLevel` des objets libres, générateurs et contenus initiaux
de réceptacles devait respecter les mêmes règles **de nouvelle création**
que le monde et le kit de départ.

## Nouveau garde-fou

`FGridEditorCanonicalItemLevelPolicy` valide en lecture seule le
`ItemId` présent dans le **même** `DA_ItemSystem` configuré :
- ARM enregistré : N1–N20 explicite, ou N0 auteur héritant une seule fois
  d'un `DefaultWeaponItemLevel` valide. Le flag `bUseWeaponCatalogue`
  ne décide jamais du type réel.
- Non-ARM enregistré (flèches) : seul N0 accepté ; le spinner des niveaux
  d'armes n'est pas proposé.
- Identité enregistrée invalide : refus, sans ancien catalogue en secours.
- Identité encore inconnue : maintien provisoire des anciennes règles de
  niveaux pour le contenu non converti ; suppression prévue en 06.4/06.5.
- Registre absent ou invalide : édition refusée plutôt qu'un retour
  silencieux à une autorité obsolète.

`AGridLevelEditorActor::ResolveEditorItemRegistry` réutilise la
configuration du système d'objets au démarrage ; dans les mondes
`EditorPreview` dépourvus de `GameInstance`, un registre transitoire
est mis en cache et conservé par `UPROPERTY(Transient)`. Ce registre
ne constitue **pas** un catalogue secondaire et ne crée aucun snapshot.
Le réglage de niveau des `FGridLooseItemInstance` et `FGridItemSpawnInstance`
ainsi que l'édition dans `SGridEditorObjectInspectorPanel` utilisent
le même garde-fou. Les données réellement écrites et leurs transactions
d'édition restent celles du `UGridLevelAsset`.

Aucun `.uasset`, `.umap`, runtime SaveGame, équipement, mesh ni palette
n'est converti ou supprimé par ce jalon.

## Régression

`Grimrock.RPG.ARM.CLEAN06_3.EditorCanonicalAuthoredItemLevels`
utilise le vrai `DA_ItemSystem` et vérifie ARM014 N0/N1/N20,
default invalide, flèches N0 versus N1 avec booléens obsolètes inversés,
rejet N21 et absence de registre. Un monde `EditorPreview` transitoire
vérifie l'écriture de `FGridLooseItemInstance` et
`FGridItemSpawnInstance` via `AGridLevelEditorActor`.
Les suites historiques `Grimrock.RPG.ARMEMENT01.8.PlacedItems.*`,
`ItemSpawns.*`, `Receptacles.*` et `Grimrock` globale restent actives.

**Hors portée :** appréciation du spinner visuel dans l'éditeur et
conversion binaire des anciens niveaux. La recette PIE est distincte
des tests `-NullRHI`.

**Localisation :** seule une infobulle Slate réservée à l'éditeur est
clarifiée ; aucune chaîne joueur/WBP n'est modifiée.
`UI.csv → ST_UI → WBP → Game/fr/en` : inchangé.
