# EFFECTS-ORG01.2D — Relocaliser le Niagara de la torche

**Objectif :** réunir les effets propriétaires à Grimrock dans `Effects/`,
sans déplacer les textures et matériaux généraux avant analyse séparée.

## Migration exacte

| Type | Ancien | Nouveau |
|---|---|---|
| Système Niagara | `/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch` | `/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch` |

Dépendance directe à resauvegarder :
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch`.
Dépendances du Niagara qui **restent dans Art** :
`MI_Distortion_1`, `MI_Flame_8`. Les maîtres et textures
sous `Art/Materials` et `Art/Textures` restent à leur place.

## Contrat de sécurité

- L'Asset Registry UE5.5.4 a confirmé via issue #50 :
  `DA_Item_Torch → NS_Flame_8_Torch → MI_Flame_8 + MI_Distortion_1`.
- La migration est effectuée **via IAssetTools::RenameAssets**
  dans le worktree Windows dédié après validation intégrale de la
  suite Grimrock, et non via git mv ou Explorateur Windows.
- L'outil exige un seul Niagara source, aucun paquet cible,
  un seul référent `DA_Item_Torch` et les deux matériaux attendus.
- Exactement trois changements .uasset attendus : suppression de
  l'ancien Niagara, création du nouveau Niagara et resauvegarde de
  `DA_Item_Torch`. Aucun autre fichier ne peut être poussé.
- L'outil corrige uniquement les redirectors propres à cette
  migration, sauvegarde les deux packages et refuse tout résidu à
  l'ancien emplacement.
- Aucun asset acheté FireFX, aucune carte `L_Dungeon`, aucun
  autre objet du donjon ne doit être modifié.
- Le commit de préparation `EFFECTS-ORG01.2D-PREP:` déclenche
  le commandlet via `UEValidationAgent.ps1` **une seule fois**.
  Le commit d'assets produit porte le préfixe
  `EFFECTS-ORG01.2D-ASSET:` et ne déclenche pas l'outil de nouveau.
- Une fois le commit d'assets validé par l'agent UE5.5.4,
  les chemins de tests temporaires (ancien ou nouveau) sont
  remplacés par le chemin canonique uniquement. Le script,
  commandlet et hook ponctuels sont ensuite supprimés.

## Tests

- `Grimrock.Editor.EffectsOrg01.Inventory`
- `Grimrock.Editor.EffectsOrg01.DependencyContract`
- `Grimrock.Items.VFX01_3.*` et `Grimrock.Items.VFX01_4B.*`
- Compilation et suite complète `Grimrock` dans UE5.5.4.
- Vérification visuelle PIE de la torche allumée/éteinte
  encore nécessaire (un test -NullRHI ne démontre pas le rendu).

## Portée

Ce jalon déplace **uniquement** le système Niagara.
Les autres matériaux (en particulier ceux de l'huile, couplés
aux scripts de réparation et aux tests de production) feront
l'objet de migrations séparées. Les profils VFX FireFX
restent des assets privés locaux non versionnés.
