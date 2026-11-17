# ARM-CLEAN06.4X.54 — Transaction native contrôlée des quatre règles partagées

**Parent :** `724e987c1b325eab1484bd51fc7c8d26710d8690` (X.53 validé SilentTask + LocalAuto).
**Nature du présent lot :** ajout du commandlet UE Editor et du runner Windows protégé, sans exécuter `-Apply` et **sans modifier les packages de contenu**.

## Périmètre strict et invariant métier

Quatre et exactement quatre fichiers Unreal LFS sont autorisés à changer **dans une transaction binaire ultérieure** :

| DataAsset / chemin sous Content | Identité conservée | Champ historique supprimé |
|---|---|---|
| `GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset` | `Item_Torch` | `Receptacle.AcceptedItems[0].ItemDefinition` |
| `GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset` | `Gem_Blue` | `Receptacle.AcceptedItems[0].ItemDefinition` |
| `GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset` | `Key_Copper` | `Lock.AcceptedKeyItems[0]` |
| `GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset` | `Key_Iron` | `Lock.AcceptedKeyItems[0]` |

Le C++ ne touche aucun autre champ d'énigme, GUID, placement, réceptacle initial, palette, loot ou SaveGame. Les deux réceptacles et les deux serrures possèdent déjà les IDs canoniques. Le commandlet refuse tout asset manquant, ID inconnu, clé inattendue, package dirty, état mixte (1–3 pointeurs), ou liste d'acceptation non exactement unitaire.

### Commandlet Editor C++

`GridArmClean064X54NativeSharedRules` fournit trois modes mutuellement exclusifs :

- `-Preflight -Output=<hors-projet>` : inspecte les vrais quatre packages, vérifie le registre de 34 définitions et produit un compte rendu sans écrire d'asset.
- `-Verify -Output=<hors-projet>` : même contrôle rechargé ; code 0 si zéro pointeur, code 4 si les quatre pointeurs anciens sont toujours présents.
- `-Apply -Output=<hors-projet> -ExpectedHead=<SHA40> -Backup0=... -Backup1=... -Backup2=... -Backup3=...` : refuse toute écriture tant que les **quatre** snapshots privés n'ont pas été vérifiés byte-for-byte contre chaque vrai fichier. Ensuite et seulement ensuite, retire les quatre pointeurs, revalide les IDs et appelle `UPackage::SavePackage` pour ces quatre packages. En cas d'échec partiel, le runner est responsable de restaurer les quatre originaux.

Un `-Apply` ne doit **jamais** être lancé isolément sans le runner protégé. Les anciens scripts X.72/X.83 et anciens commandlets X.71/X.81 ne sont pas autorisés pour cette opération.

### Runner Windows PowerShell

`Scripts/ApplyARM_CLEAN06_4X54_NativeSharedRules.ps1` :

1. refuse le worktree sale, une branche autre que `master`, un HEAD divergent, un Unreal Editor ouvert, un snapshot dans le dépôt, un asset non hydraté LFS ou un fichier hors liste ;
2. avance uniquement en fast-forward vers `-ExpectedHead`, compile UE5.5.4 dans le checkout principal, puis contrôle les quatre SHA-256 inchangés ;
3. lance le préflight (défaut sans `-Apply`) et `-Verify` ;
4. sur `-Apply`, place quatre snapshots privés SHA-256 **hors dépôt**, passe leurs chemins à l'UE natif, vérifie le rapport de quatre sauvegardes ;
5. rouvre les assets via un **second processus UE** `-Verify`, exige quatre changements Git et la suite Automation `Grimrock` complète ;
6. avec `-Publish` explicite seulement, vérifie le SHA distant inchangé, crée **un commit binaire indépendant de quatre fichiers exactement** et fait un push non forcé sur `master` ;
7. en cas d'échec avant commit, restaure les quatre snapshots et contrôle les quatre hashes ainsi qu'un worktree propre ; après commit local, ne réécrit jamais l'historique automatiquement.

Sans `-Apply`, les seuls fichiers écrits sont les rapports de préflight privés dans `StateRoot`. `-Apply` sans `-Publish` laisse volontairement les quatre vrais assets modifiés après validation, pour inspection ; préférer la procédure complète `-Apply -Publish` une fois le préflight approuvé et les deux statuts Windows du code confirmés.

## Exécution opérateur (uniquement après validations Windows du SHA de code)

**Les exemples ci-dessous ne constituent aucune exécution effectuée par ChatGPT.** Fermer UE. Réserver un emplacement de backups privé hors `D:\Development\GrimrockPrototype` (et qui n'est pas non plus son parent) ; conserver les copies après publication.

Préflight lecture seule :

```powershell
.\Scripts\ApplyARM_CLEAN06_4X54_NativeSharedRules.ps1 `
  -RepoRoot D:\Development\GrimrockPrototype `
  -EngineRoot D:\UE_5.5 `
  -StateRoot D:\GrimrockNativeSnapshots `
  -ExpectedHead <SHA40_DU_COMMIT_CODE_X54>
```

Puis **seulement** si le préflight indique `old=4`, que le checkout est propre et que les contrôles Windows du code sont verts, transaction native avec publication explicite :

```powershell
.\Scripts\ApplyARM_CLEAN06_4X54_NativeSharedRules.ps1 `
  -RepoRoot D:\Development\GrimrockPrototype `
  -EngineRoot D:\UE_5.5 `
  -StateRoot D:\GrimrockNativeSnapshots `
  -ExpectedHead <SHA40_DU_COMMIT_CODE_X54> `
  -Apply -Publish
```

## Critères de réception / limites

- Le présent commit de **code** doit obtenir `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success` sur son SHA exact. La compilation et les tests X.51/X.52/X.53 ne prouvent pas l'exécution du commandlet natif ou du script.
- Le futur commit **binaire** doit contenir uniquement les quatre `.uasset`, avec contrôle Git LFS et nouvel ensemble de deux statuts Windows succès. Le test natif X.48 doit donner 59 références (contre 63 avant).
- Recette PIE manuelle ciblée ensuite : torch holder accepte la torche, Guardian accepte la gemme bleue, mauvaises offrandes refusées, cuivre/fer ouvrent la bonne serrure seulement, portes et consommation de clés inchangées, Save/Continue inchangé.
- **Pas de suppression de champs UPROPERTY legacy** ni migration des SaveGames v27 dans ce jalon ; 18 contenus initiaux, 3 overrides, 33 palette et 5 loots restent séparés.
- Aucun changement de texte UI/WBP : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.

**ARM-CLEAN06.4 PARTIELLE ; ARM-CLEAN06.5 BLOQUÉE.**
