# ARM-CLEAN06.3W — Vol des flèches : mesh canonique sans niveau d'arme

**Prérequis :** ARM-CLEAN06.3V, validation automatique UE5.5.4 + `Grimrock` sur le SHA exact.

Les flèches `Item_Arrows_01` ne sont pas des armes ARM N1–N20. Leur `ItemLevel=0` est reconnu par `UGridItemSubsystem` depuis 06.3V. Désormais, `AGridThrownItemActor::InitializeThrownItem` utilise le **`WorldMesh` unique de `UGridAmmoDefinitionAsset`**, via `FGridCanonicalProjectileVisualReadModel`, pour le projectile d'une flèche tirée.

Les armes cataloguées conservent leur `ProjectileMesh` et leurs règles N1–N20 séparées. Le vol, l'axe de pointe, la rotation et l'échelle des flèches restent temporairement les paramètres historiques de leur définition : le nouveau modèle canonique de munitions ne possède pas de second profil de projectile, et aucune politique inventée ne peut en tenir lieu. Les flèches non enregistrées conservent leur chemin précédent. Un identifiant canonique connu mais à niveau invalide ne reprend jamais un ancien mesh.

Cette étape ne modifie aucune quantité, GUID, niveau, collision, projectile physique, état d'arc, cible, impact, monstre, donnée sauvegardée ou `.uasset`. Elle convertit seulement la source du mesh de vol des munitions au runtime.

`Grimrock.RPG.ARM.CLEAN06_3.UnlevelledAmmoFlightMesh` charge le vrai `DA_ItemSystem`, résout `Item_Arrows_01` N0, vérifie la même référence soft de mesh, l'absence d'actions d'armes inventées, le refus du niveau -1 et le maintien du comportement hors registre.

Le rendu réel de la flèche en vol, dans les monstres et au sol devra être vérifié en PIE, séparément de `-NullRHI`. **UI.csv → ST_UI → WBP → Game/fr/en : inchangé**.
