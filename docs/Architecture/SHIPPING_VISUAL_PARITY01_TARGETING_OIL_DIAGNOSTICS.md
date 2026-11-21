# SHIPPING-VISUAL-PARITY01 — Visée de zone et flaque d'huile absentes en exécutable Win64 Shipping

**Signalement utilisateur, 21 novembre 2026 :** New Game, inventaire/combat, interaction et Save/Continue ont réussi dans l'exécutable compilé. Mais l'utilisateur ne voit ni la flaque d'huile ni les carrés de sélection jaune (centre) / verts (zone) qui apparaissent en PIE. Le précédent lot `ARMEMENT01.17.1.4.SHIPPING01` a ajouté l'inclusion forcée des deux matériaux de décalque au Cook, mais n'a pas corrigé le rendu constaté. **Ne pas confondre succès du Cook et preuve de rendu.**

## 1. Cause certaine : DrawDebugBox n'existe pas en Shipping

Le contrôleur `AGrimrockPlayerController::DrawCombatTargetingPreview` utilisait exclusivement `DrawDebugBox`. Le mode de visée et sa validation de portée/cellules ne dépendent pas de ces boîtes : c'est leur présentation qui disparaît en Shipping.

**Correction :** `ULineBatchComponent` dédié, créé/registré au `BeginPlay` sur le joueur local, dessine via `DrawBox` des lignes de scène réelles avec les couleurs, positions et épaisseurs existantes. `ClearBatch` efface l'ancien survol chaque frame, les annulations et l'absence de ciblage ; le composant est détruit dans `EndPlay`. `DrawDebugHelpers.h` est supprimé du contrôleur. La suite Automation `Grimrock.Monsters.MON12.8.6.ShippingTargetingRenderer` prouve que le composant produit et nettoie ses lignes. Le compilateur Win64 Shipping doit valider que le code de jeu compile, mais la preuve d'affichage reste native GPU.

La mécanique des cellules, validation de ciblage, coûts, portée, LOS et actions n'est pas modifiée.

## 2. Flaque d'huile : diagnostic honnête, pas de nouveau correctif hypothétique

Les deux ressources `M_Surface_Oil` et `MI_Surface_Oil` existent et le répertoire est désormais déclaré dans `DirectoriesToAlwaysCook`. L'acteur runtime appelle `OilSurfaceMaterial.LoadSynchronous()`, ne crée pas de `UDecalComponent` si ce pointeur est invalide, puis projette des décalques si les surfaces de gameplay sont présentes. `FloorISM->SetReceivesDecals(true)` est déjà appelé pendant la reconstruction du niveau.

**Les informations disponibles n'établissent pas la cause** : la présence des matériaux cuisinés ne confirme ni la visibilité du DBuffer/Decal ni l'orientation de la projection dans l'image finale. Deux hypothèses au moins subsistent : matériau non résolu au moment de l'action ; matériau bien chargé et composants créés, mais non visibles sous les paramètres de rendu du jeu.

**Instrumentation Shipping explicite sans toucher aux sauvegardes :**

`GridLevelRuntimeActorSurfaces.cpp` écrit seulement sous l'argument opt-in `-GrimrockVisualAudit` dans `Saved/VisualDiagnostics/OilSurface.txt` de son `-UserDir`. Chaque projection d'huile fournit exactement les champs `OilCells`, `MaterialLoaded`, `DecalComponents`, `r.Decals`, `r.DBuffer` et `r.DecalQuality` ; aucune information nominative, sauvegarde, image ni argument libre n'est copiée. Un manque de matériau émet `MaterialLoaded=0`. Des décalques créés avec `r.Decals=0` indiqueraient que la scène ne les affiche pas. Si tous les signaux sont positifs mais la flaque reste invisible, inspecter matériau et réception DBuffer réelle avec RHI, sans modifier aveuglément les effets ou la couleur.

**Le diagnostic est désactivé par défaut** : pas de fichier créé sans `-GrimrockVisualAudit`. Il ne modifie ni les mécaniques ni les assets.

## 3. Validation

1. GitHub : un commit atomique code, test, documentation ; statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur son SHA précis. Le second inclut Editor, Shipping et Automation.
2. Windows local : `git pull --ff-only origin master`, puis `Scripts/ValidatePackage.ps1 -EngineRoot D:\UE_5.5 -Configuration Shipping`. Il produit **une nouvelle archive** ; ne pas réutiliser un ancien exécutable.
3. Créer un dossier temporaire de test privé et lancer **le nouvel** `Windows\GrimrockPrototype.exe` avec `-UserDir=<dossier>` et `-GrimrockVisualAudit`.
4. Tester le survol de cible de zone (jaune/vert/rouge) et lancer une flasque d'huile. Relever `<dossier>\Saved\VisualDiagnostics\OilSurface.txt` si la flaque reste invisible.
5. En cas de `OilCells>0 MaterialLoaded=1 DecalComponents>0 r.Decals=1` avec rendu toujours invisible, c'est un problème **aval** (rendu du decal, matériaux récepteurs ou projection), qui nécessite observation GPU ou nouvelle correction dédiée.

**Limites :** `ULineBatchComponent` est une primitive de lignes dont la documentation déconseille l'usage massif permanent ; ici quelques rectangles ne sont tracés que pendant un ciblage actif. Les tests headless `-NullRHI` ne peuvent pas conclure à la qualité visuelle.
