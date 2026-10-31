# TRAP01.5 — Placer des pièges de donjon depuis le Grid Editor

Date : 31.10.2026. État : **code C++ et test Editor publiés ; validation locale UE 5.5.4 en attente**.

## Problème traité

Les jalons TRAP01.1 à TRAP01.4.1 définissent, détectent, déclenchent,
désamorcent et récompensent les pièges, mais aucun piège prêt à peindre
n'était présent dans `DA_ObjectPalette_Default`.

TRAP01.5 ajoute **3 définitions de démarrage provisionnées par l'éditeur**
dans la catégorie `Hazards`. Aucun asset de niveau secondaire ou nouvel
acteur autonome de piège n'est créé : les trois sont des `Trigger` standards
stockés dans `UGridLevelAsset::WorldObjectInstances`, avec un GUID par placement.

## Définitions fournies

| Palette Entry / DefinitionId | Placement | Détection DC | Désamorçage DC | Dégâts | XP groupe si désamorcé |
|---|---|---:|---:|---|---:|
| `Trap_Floor_Spikes_01` | Floor | 10 | 10 | Physical Piercing 6–12 | 20 |
| `Trap_Wall_Darts_01` | Wall | 14 | 15 | Physical Piercing 8–14 | 30 |
| `Trap_Ceiling_Fire_01` | Ceiling | 18 | 20 | Fire 12–20 | 40 |

Tous les pièges sont `bIsDungeonTrap=true`,
`bInitiallyHidden=true`, `bCanBeDisarmed=true`,
`RequiredToolTag=Tool.Disarm`, `bOneShot=true`,
`bAffectsEntireParty=true`, `ActivationPolicy=OnEnterCell`.
Un piège détecté **reste dangereux** tant qu'il n'est pas désamorcé.

Le piège mural `OnEnterCell` est placé sur un mur de la cellule
qui le déclenche. Le déclenchement sur frontière `OnCrossBoundary`
n'est pas implémenté et ne doit pas être prétendu fonctionnel.

## Provisionnement par l'éditeur

Le panneau `SGridEditorToolPalettePanel::BuildPaletteSection` vérifie les
3 entrées ; lorsque l'une manque, il appelle
`AGridLevelEditorActor::EnsureDungeonTrapDefinitions`.
La fonction est également accessible depuis l'acteur éditeur comme
fonction `Editor|Palette` (appel C++ / Blueprint), avec `OutError`.

Lors du **premier** appel avec la palette de production assignée :
1. création des DataAssets `DA_Trap_Floor_Spikes_01`,
   `DA_Trap_Wall_Darts_01` et `DA_Trap_Ceiling_Fire_01` dans
   `/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/` ;
2. attribution de `SupportedType=Trigger`, `PlacementSurface`,
   paramètres TRAP01, `RuntimeActorClass=AGridGenericObjectActor` ;
3. ajout de 3 entrées sous `Hazards` dans `DA_ObjectPalette_Default` ;
4. synchronisation des définitions avec le runtime de prévisualisation ;
5. sauvegarde des nouveaux assets et de la palette dans Content/.

**Important :** les assets binaires créés sur votre PC sont des fichiers
`.uasset` **locaux**. Le commit C++ ne les contient pas. Après ouverture
de la palette ou test automatisé, vérifiez `git status` et ajoutez ces
4 assets à un commit local séparé si vous souhaitez une installation
reproductible pour les futurs créateurs de niveaux.

Un asset existant n'est jamais réinitialisé : même si son maillage,
sa difficulté ou ses dégâts ont été personnalisés, le provisionneur
le laisse intact. En cas de contrat incompatible, il refuse de le
réparer silencieusement. L'opération est idempotente.

### Meshing provisoire

Le système cherche un mesh final par nom :
- `SM_Trap_FloorDisarmable_01`
- `SM_Trap_WallDarts_01`
- `SM_Trap_CeilingFlame_01`

Quand il manque, l'éditeur utilise le cube UE natif
`/Engine/BasicShapes/Cube`, avec une mise à l'échelle adaptée.
Ce cube sert **uniquement à débloquer la recette fonctionnelle**.
Le preview éditeur montre le piège même lorsqu'il est caché en jeu.
Dans le runtime, le cube est invisible avant détection ou activation.
Le matériau rouge `DungeonTrap.RevealedOverlayMaterial` reste optionnel
et doit être assigné dans le DataAsset pour un effet rouge transparent.

## Procédure manuelle de placement

1. Dans UE 5.5.4, ouvrir `L_Dungeon` et l'outil **Grid Editor**.
2. Vérifier que l'acteur possède `DA_ObjectPalette_Default`.
3. Choisir **Paint Object**, puis la catégorie **Hazards**.
4. Pour le test immédiat, choisir `Trap_Floor_Spikes_01`.
5. Cliquer sur une cellule de sol, **à distance du spawn** du groupe,
   de préférence au bout d'un couloir dégagé.
6. Pour les variantes : choisir `Trap_Wall_Darts_01` et sélectionner
   le bord (N/E/S/W) de la cellule ; pour
   `Trap_Ceiling_Fire_01` sélectionner la cellule sans bord.
7. Sauvegarder le **LevelAsset / DungeonAsset** et les quatre DataAssets
   nouvellement générés. Les instances ont chacune leur GUID stable.
8. En PIE : avancer vers le piège. Si le meilleur observateur réussit
   son test, le flux affiche le piège et le cube apparaît. Si le test
   échoue, marcher dessus le déclenche : dégâts de type individuel
   et révélation.
9. Pour le désamorçage, il faut également un objet
   `Item_DisarmTools_01` effectif, la compétence `Skill_Traps`
   apprise et le tag `Tool.Disarm`. Sans ce kit, tester uniquement
   la détection et le déclenchement.
10. Pour le test de notification XP verte, réussir un désamorçage
    (ou vaincre un monstre). Le service partagé MON15 émet l'XP.

L'inspecteur d'objet indique maintenant le type de piège, les deux
difficultés, le gain XP maximal de désamorçage et les dégâts. La
personnalisation des valeurs s'effectue dans les **DataAssets**, pas
par copie de ces paramètres sur l'instance ; la création d'overrides
de piège par instance sera un jalon distinct.

## Tests automatisés UE

`Grimrock.Editor.TRAP01.5.StarterPaletteAndPlacement` vérifie :
- le provisionnement de la vraie palette et l'idempotence ;
- les définitions Floor/Wall/Ceiling, dégâts et invisibilité ;
- l'appel standard `ApplyPaletteEntry` puis `PlaceSelectedObject` ;
- les trois instances dans un seul LevelAsset, GUID valides,
  coordonnées et orientation de bord correctes ;
- les trois previews avec leur mesh dans l'éditeur.

Le test crée les DataAssets de production **si absents** pour débloquer
l'expérience PIE. Vérifiez les nouveaux fichiers dans `Content/`.

Commande (compilation incluse) :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.TRAP01.5"
```

Puis les régressions `Grimrock.TRAP01.4`,
`Grimrock.TRAP01.3` et `Grimrock.TRAP01.2`
en utilisant `-SkipBuild`.

## Limites volontaires

Les trois pièges **OnEnterCell** sont jouables pour la détection,
les dégâts et le désamorçage avec outils. OnCrossBoundary,
OnInteract, commande distante de neutralisation, animations 3D,
VFX d'activation et effets de statut restent des jalons suivants.
Le mesh provisoire n'est pas l'objet final de la direction artistique.

## Compatibilité du validateur Trigger

Le validateur historique `UGridWorldObjectDefinitionAsset::ValidateDefinition`
imposait toujours `PlacementSurface=Floor` aux `Trigger`. TRAP01.5
autorise explicitement Wall/Ceiling **uniquement lorsque**
`DungeonTrap.bIsDungeonTrap=true`, tout en conservant la restriction
Floor-only pour les triggers ordinaires. Le test Editor vérifie les
nouveaux profils sur les trois surfaces et le refus du Trigger classique
placé sur un mur.
