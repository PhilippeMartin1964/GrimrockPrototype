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

Pour `CellSize=200` et `GroundFireCoverage=1`, le contrat demandé est
`200 × 200 cm`. Si le Niagara natif couvre environ 180 × 180 cm,
`NiagaraReferenceWidthCm=180` et `GroundFireScale=1` donnent
`ScaleXY ≈ 1.111`. Ce résultat est uniquement la **transformation du
composant**, et suppose que les particules suivent l'échelle locale.

La couverture effective d'un effet qui émet des particules dans l'espace
monde, dans une sphère ou avec des vitesses latérales ne peut pas être
garantie par cette transformation : son `Box Location` doit recevoir les
dimensions de la case via des paramètres Niagara d'entrée.

## Architecture canonique proposée

1. **Système Niagara source réutilisable** : `NS_FFX_Ground_Fire` reste dans
   les packs tiers `Content/FireFX` ou `Content/F_FreeFlameFx_Pack`, ignorés par Git.
2. **Interface Niagara typée** : des `User.*` réellement connectés aux
   modules d'émetteurs (`User.SpawnSize` Vector3,
   `User.FlameHeight` Float, `User.FlameDensity` Float,
   `User.SmokeDensity` Float, `User.FireTint` LinearColor).
   Ces noms sont un *contrat de conception proposé*, pas une affirmation
   sur les paramètres existants de FAB.
3. **Profils Grimrock versionnés** : `DA_GroundFire`, puis profils séparés
   pour une torche, un brasier, un feu mural, un feu de camp, etc.
   Chacun peut réutiliser le **même** Niagara source avec des paramètres
   différents, sans faire varier les autres instances.
4. **Composant d'application C++ unique** : chargé de valider le type et
   l'existence de chaque paramètre, d'appliquer les paramètres à
   `UNiagaraComponent` **avant** `Activate()`, puis de gérer le cycle
   de vie, les budgets, sons et lumières.
5. **Instance de jeu** : contexte spatial (cellule 200 × 200, position,
   orientation), choix du profil, éventuelles surcharges locales très
   limitées et optionnelles. L'asset Niagara et les profils ne sont
   jamais mutés à l'exécution.

Une `Niagara Parameter Collection` convient pour des valeurs mondiales
communes (vent global, météo, ambiance), **pas** pour distinguer
les dimensions d'une torche et d'un feu au sol utilisant simultanément
le même système. Utiliser des `User Parameters` par composant pour cela.

## Limites actuellement non résolues

Le C++ du projet sait appliquer hauteur/couverture par transformation et
transmettre des paramètres utilisateurs **si présents**. En revanche,
la densité réelle, la fumée et la couleur du pack FAB ne sont pas
garanties tant que leurs entrées ne sont pas connectées dans Niagara.
Aucun chemin tiers n'a été introduit ni asset commercial modifié.
