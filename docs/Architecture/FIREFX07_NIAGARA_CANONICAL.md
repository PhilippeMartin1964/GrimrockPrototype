# FIREFX07 — Diagnostic visible et pipeline canonique des effets Niagara

## Correctif du bouton de diagnostic

`BP_GridLevelRuntimeActor → Details → Log Ground Fire Niagara Bindings`
présente maintenant **une boîte de dialogue** avec les mêmes informations
que l'Output Log. La fonction n'est plus déclarée `const` et écrit aussi
une ligne de confirmation dans `LogTemp` en plus de `LogGridSurfaceRuntime`.

Le rapport indique : acteur inspecté, véritable `FireVfxProfile` de cette
instance, système Niagara résolu, surface demandée et échelle XYZ calculée,
noms `User.*` présents et correspondances avec les champs du profil.
Un profil absent signale une erreur **visible**, même sans Niagara installé.
Si aucun dialogue ne s'ouvre, vérifier que l'on est dans la bonne instance
de l'acteur, hors PIE, et que le C++ actualisé a été compilé dans l'éditeur.
Le test de réflexion vérifie que la fonction est bien exposée via
`FUNC_BlueprintCallable`. `CallInEditor` est un spécificateur d'éditeur
et **non** un drapeau de `EFunctionFlags` ; le test ne simule pas un clic.

## Calibrer 200 × 200 cm

Pour `CellSize=200`, `GroundFireCoverage=1` et
`GroundFireScale=1`, le contrat demandé est `200 × 200 cm` :

- Si `SpawnBox` est compatible (Vector3), le runtime transmet une
  zone XY de **200 × 200 cm** directement à `User.SpawnBox` et
  conserve `ScaleXY=1` (pas de double application).
- Sinon, le C++ utilise le repli par transformation : avec
  `NiagaraReferenceWidthCm=180`, `ScaleXY ≈ 1.111`.
  Ce repli suppose que les particules suivent l'échelle locale.

FIREFX08 sait désormais transmettre `SpawnBox` avec la largeur XY demandée,
si sa variable est de type `Vector3`. Si ce contrôle est disponible, le code
n'applique plus une seconde fois la couverture dans l'échelle XY.
Si les particules possèdent ensuite une vitesse horizontale, leur silhouette
visible peut dépasser le volume d'émission. Le diagnostic permet de confirmer
la compatibilité effective du système.

## Architecture canonique retenue (FIREFX08–09)

Le système Niagara commercial reste une **ressource partagée**. Son chemin
est défini dans le profil versionné, jamais codé en dur dans le runtime.

- **Interface FireFX réellement observée dans L_Dungeon** :
  `SpawnBox`, `FlameRate`, `FlameSizeMin/Max`, `SmokeRate`,
  `Tint`, `Wind`, `EmberRate`, `GlowSize`, etc. (29 variables).
  Leurs **types** doivent être consultés dans le nouveau diagnostic.
- **DA_GroundFire** : `GroundFireCoverage`, `FlameHeightCm`,
  `FlameDensity`, `SmokeDensity`, `FireTint`, lumière et audio.
  `bAutoBindKnownFireControls=true` relie automatiquement les contrôles
  compatibles `SpawnBox`, `FlameRate`, `SmokeRate` et `Tint`.
- **Surcharges génériques** : `User Parameter Overrides` est une liste
  `ParameterName + ValueType + valeur` dans le Data Asset ;
  seuls les noms et types réellement disponibles sont transmis.
  Ces surcharges sont prioritaires sur les commandes automatiques.
- **Composants isolés** : le C++ applique chaque profil à sa propre
  instance `UNiagaraComponent` sans modifier le système partagé.
  Un autre futur profil de torche ou brasier pourra définir d'autres
  valeurs pour le même Niagara.
- **Collections de paramètres mondiales** : réserver les Niagara
  Parameter Collections aux variables partagées (vent/météo globale),
  pas aux dimensions particulières de chaque flamme.

Les `User Parameters` doivent réellement être utilisés par les modules
du Niagara : les exposer ne garantit pas leur influence visuelle.

## Limites actuellement non résolues

Le C++ relie de façon typée les quatre commandes connues et laisse
la possibilité d'overrides explicites. Les tests automatiques ne disposent
pas du pack commercial. Ils ne peuvent donc pas confirmer les types de ses
29 paramètres, ni démontrer que les modules Niagara utilisent chaque entrée.

La hauteur `FlameHeightCm` transforme le composant en Z ; des émetteurs
World Space peuvent ignorer cette transformation. Il faut alors régler
`FlameSizeMin/Max` après contrôle de leur type et de leur sémantique
dans Niagara.

La fumée peut aussi être générée ponctuellement par `SmokeBurst` :
`bEnableSmoke=false` met à zéro `SmokeRate`, mais ne modifie pas
automatiquement les bursts. Aucun asset du pack n'a été modifié.
