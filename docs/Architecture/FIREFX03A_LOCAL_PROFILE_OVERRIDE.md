# FIREFX03A — Profils VFX privés sans référence publiée dans les Blueprints

## Pourquoi

`BP_GridLevelRuntimeActor` est versionné. Il ne doit pas sérialiser une référence
vers `/Game/FireFX` ou `Effects/Fire/Local` lorsque ces packages sont ignorés par Git.

## Configuration locale de l'éditeur UE5.5.4

1. Installer le pack commercial FireFX en local dans `Content/FireFX/` (hors Git).
2. Dans l'éditeur, créer le profil `GridSurfaceVfxProfileAsset` à
   `/Game/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire`.
3. Dans ses propriétés, sélectionner comme `GroundFireSystem` le vrai Niagara
   `NS_FFX_Ground_Fire` sous `/Game/FireFX/FX/`. Renseigner éventuellement
   `FireCrackleLoopSound` avec un son bouclant, spatialisé et attenué.
4. Créer **sur ce poste uniquement** le fichier
   `Saved/Config/GrimrockLocalVfx.ini` (dossier `Saved` déjà ignoré par Git) :
   
   ```ini
   [Grimrock.LocalVFX]
   FireVfxProfile=/Game/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire.DA_GroundFire
   ```

5. Laisser `BP_GridLevelRuntimeActor.FireVfxProfile` **vide**. Le runtime C++
   résout le profil privé uniquement dans l'éditeur (donc aussi en PIE).
6. Pour tester un profil catalyseur privé, ajouter éventuellement :
   `CatalystVfxProfile=/Game/GrimrockPrototype/Effects/Alchimie/Catalyst/DA_Catalyst_Default.DA_Catalyst_Default`
   après création effective de cet asset.

## Priorités de résolution

1. Profil explicite affecté au runtime/Blueprint (uniquement si versionnable).
2. Profil défini dans `Saved/Config/GrimrockLocalVfx.ini`, en édition UE.
3. Réglages C++ par défaut (aucun système Niagara si aucun profil).

Aucun chemin commercial n'est codé en C++. Aucun fichier privé n'est publié.
Le fichier local n'est pas lu dans les builds non éditeur et n'est pas une
solution de packaging du pack ; cette étape reste distincte et soumise à sa licence.

## Validation

Compilation UE5.5.4, suite Automation, puis contrôle visuel en PIE avec FireFX installé.
Une validation `-NullRHI` ne remplace pas la recette GPU/son.
