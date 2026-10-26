# ARMEMENT-INDUS04.3B.2 - Comparaison des degats physiques bruts

## Decision

La comparaison des degats minimum et maximum (extremes bruts de NdM+K, avant les attributs du personnage) est permise entre armes du **meme type principal de degats**, meme si leurs sous-types physiques different : tranchants, perforants ou contondants.

Le sous-type reste affiche dans `WeaponDamageType` et ne produit pas de delta numerique : la resistance de l'adversaire peut donner un autre resultat reel. Les types principaux differents (par exemple physique contre feu) conservent des valeurs cote a cote **sans** delta de degats, pour eviter de suggerer un gain universel.

Les des `WeaponDice` sont du texte non soustractible. La precision, le cout en PA, la portee et les deltas globaux avant/apres ne changent pas. Aucun calcul de combat ni DataAsset de production n'est modifie.

## Cas de recette N1

Avec la Dague N1 (tranchants, min. 1, max. 4) en main principale, survoler le Pic de guerre N1 (perforants, min. 4, max. 7) :

- Degats min. : 4 contre 1, delta **+3** benefique.
- Degats max. : 7 contre 4, delta **+3** benefique.
- Types de degats : perforants contre tranchants, affiches sans delta.
- La simulation de main secondaire reste independante : retrait du bouclier et perte d'armure sont conserves.

Le test `Grimrock.UI.WeaponTooltip04.PhysicalSubtypeRawDamage` lit les vrais `.uasset` sans les modifier et verifie ces deltas ainsi que l'absence de delta sur les textes. `ActionPointDeltaSemantics` continue a verifier que physique contre feu ne cree pas de delta de degats.

## Validation locale avant cloture

Le commit n'est pas une preuve de compilation Unreal. Depuis `D:\Development\GrimrockPrototype` :

```powershell
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.WeaponTooltip04"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentTooltip043B"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentPreview043A"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.Item01"
git status --short
```

Attendu : **9/9 + 4/4 + 4/4 + 5/5 = 22/22**, sans echec de test ; controler en PIE Pic de guerre/Dague et Grande Hache/Dague/Bouclier.

Ne pas ecraser ni reversionner `Content/Localization/StringTables/ST_UI.uasset` : ce fichier local a ete genere par le smoke precedent et attend un commit binaire distinct. Aucun changement UMG ou de hauteur du tooltip ici. Les descriptions narratives du tableur seront prises en compte au jalon dedie.
