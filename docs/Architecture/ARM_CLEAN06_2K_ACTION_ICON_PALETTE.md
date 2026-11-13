# ARM-CLEAN06.2K — Palette d'icônes d'actions d'armes

Le fichier `Content/GrimrockPrototype/UI/Icons/Combat/
DA_EquippedWeaponActionIcons.uasset` est bien présent dans
GitHub LFS. Les douze textures d'actions `T_Action_*`
existent dans le même dossier. **Leur existence ne prouve
pas qu'elles soient toutes affectées dans le DataAsset** :
seul le chargement du vrai asset par Unreal établit la map.

Le modèle approuvé prévoit de déplacer
`UGridEquippedWeaponActionIconPalette::Icons` vers
`UGridCanonicalWeaponVisualCatalogueAsset::ActionIcons`.
Les deux maps utilisent déjà la même clé
`EGridEquippedWeaponActionIconKind` et des soft references
vers `UTexture2D`. Aucun changement de texture
n'est requis pour ce transfert.

Le helper Editor-only `GridArmClean06ActionIconsProjection`
copie uniquement les entrées réellement authorées, y compris
les valeurs nulles intentionnelles. Il refuse la clé
`None`, une map vide ou un écrasement d'une destination
déjà remplie. Le contenu exact des `SoftObjectPath` est
comparé, sans charger les textures ni sauvegarder de package.

Les deux tests Automation
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionIconPalette`
et `Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActionIconProjectionSafety`
vérifient les chemins réellement authorés, le refus des
catégories invalides et l'absence de modification de
`UPackage`.

**L'icône d'objet reste** `VisualProfiles.ItemIcon`,
**l'icône de commande reste** `ActionIcons[ActionKind]`.
La présence d'une image ne crée aucun combat, mouvement
ou droit d'attaque. L'ancien WBP reste relié à la
palette historique pendant la période de conversion
et ne sera coupé qu'avec les autres lecteurs au cutover
cohérent.

Aucun binaire `.uasset`, niveau, WBP ni sauvegarde
n'est modifié par ce jalon ; la migration réelle reste
conditionnée à la validation globale et à l'outil UE
de conversion Editor-only.
