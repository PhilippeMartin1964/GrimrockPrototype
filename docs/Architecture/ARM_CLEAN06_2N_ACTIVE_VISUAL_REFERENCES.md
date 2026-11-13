# ARM-CLEAN06.2N — Un visuel obligatoire seulement pour les ARM concrets

## Défaut levé dans le registre canonique

Jusqu'ici `UGridItemSystemAsset::Validate` refusait
un `DA_ItemSystem` si un des 82 archétypes de
`DA_WeaponCatalogue` ne possédait pas de
`VisualBinding`, même lorsque cet ARM n'était
utilisé par aucune définition d'arme concrète.

Ce blocage était incompatible avec le rythme de
production : **82 profils numériques N1–N20**
forment la bibliothèque canonique, mais seuls
**22 objets ARM** possèdent actuellement des
DataAssets de production liés au manifeste.
Il n'est pas nécessaire de créer 60 assets factices
ni 60 visuels fictifs pour enregistrer les 22 vrais.

## Contrat corrigé

- Le catalogue combat continue de valider **82 ARM**
  et **1 640 niveaux**, dont 260 profils passifs.
- La présence d'un visuel est exigée pour chaque
  `UGridArmamentDefinitionAsset` effectivement
  enregistré dans `DA_ItemSystem.ItemDefinitions`.
- Les `VisualBindings` autorisés continuent
  d'être validés (ID uniques, `VisualId` connu)
  par `UGridCanonicalWeaponVisualCatalogueAsset`.
- Le contrôle des `AmmoKindId`, des actions
  référencées et des `PresentationId` est conservé
  sans assouplissement.
- L'ajout futur d'une définition vers un ARM
  auparavant inutilisé exige alors immédiatement
  son visuel concret.

Le test `Grimrock.RPG.ARM.CLEAN06_2.Canonical.LiveArmamentVisualReferences`
crée un registre de **82 ARM**, dont seulement deux
ont une définition concrète dans la fixture ; il
confirme que les 80 autres n'exigent pas de visuel,
et qu'enlever le visuel d'un bouclier réellement
enregistré rend le registre invalide.

Cette modification touche le **modèle canonique
inactif**, pas le lecteur de gameplay historique.
Aucun asset, WBP, SaveGame, niveau ou comportement
d'arme existante n'est modifié. Le cutover binaire
reste soumis aux preuves de parité globales et
à la validation UE5.5.4 sur le SHA exact.
