# ARM-CLEAN06.5H.72 — Munitions : supprimer le repli d'arme ancienne en production

## Périmètre converti

La politique `FGridRangedAmmunitionPolicy::ResolveForAttack` consultait encore `UGridItemDefinitionAsset::RequiredAmmunitionItemDefinitionId` pour les armes **inconnues** du registre, puis pouvait joindre une ancienne arme à une munition N0 canonique. Ce chemin restait compilé en Shipping, bien que les armes connues résolvent déjà leurs besoins depuis `UGridItemSubsystem`.

Désormais toute attaque **à distance** d'un `ItemId` non enregistré échoue fermée : `bValid=false`, `AmmunitionItemId=NAME_None`. La caractérisation d'une définition historique n'est possible que sous `WITH_DEV_AUTOMATION_TESTS`, sans registre, pendant `GIsAutomationTesting`, avec correspondance exacte d'identité. Les quatre surcharges prenant directement `const UGridItemDefinitionAsset&` ne sont plus déclarées ni compilées en Shipping ; leurs consommateurs actuels sont exclusivement des tests historiques.

## Non-régressions

Le préflight canonique de l'arc `Weapon_ShortBow_01` N1 et de `Item_Arrows_01` N0 conserve l'`AmmoKindId` autoritaire, le clamp ordinaire 400–4800 cm/s, la vitesse spécialisée prioritaire, le refus des profils incohérents, le débit 2→1→0, les GUID, le mesh et le projectile récupérable. Une attaque non distante n'exige toujours pas de munitions.

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalRangedAmmo.Projection` remplace l'ancienne attente qui autorisait une arme inconnue dans un registre configuré par un refus explicite, et maintient un test distinct de la fixture Automation sans registre.

Aucun changement de WBP, texte joueur, localisation, Niagara, mesh, `.uasset`, `.umap` ou `.sav`. `UI.csv → ST_UI → WBP → Game/fr/en` : inchangé. Les 53 packages historiques H.23 et les sept sauvegardes privées restent protégés. Aucun test `-NullRHI` ne prouve un rendu GPU.

H.73 n'est autorisé qu'après `UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS` sur le commit H.72 exact (Editor, Shipping, suite Automation `Grimrock`).
