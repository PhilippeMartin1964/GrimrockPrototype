# ARM-CLEAN06.3R — Vol d'armes lancées : visuels canoniques par niveau

**Référence de départ :** ARM-CLEAN06.3Q, `901b9a4`, compilation UE5.5.4 et suite Automation `Grimrock` vertes. Validation PIE des pickups / masses / réceptacles confirmée par le propriétaire.

## Découpage sans perte

`AGridThrownItemActor::InitializeThrownItem` résout, pour une *arme cataloguée* qui possède réellement une présentation de projectile, le `ProjectileMesh` et le `FGridCanonicalProjectileVisual` du seul `DA_ItemSystem`. `FGridCanonicalProjectileVisualReadModel` ne prend que `FGridItemInstance.ItemDefinitionId` et le niveau réel `ItemLevel` : aucun N1 synthétique. `VisualId`, rotation, échelle, mode Stable/Tumble/Spin, axe de pointe, vitesse de rotation et vitesse de bascule sont projetés depuis `WeaponVisualCatalogue` en lecture seule.

Le mouvement, les dégâts, le ciblage, le rebond, la gravité, la collision et la consommation d'un objet ne sont **pas** migrés ici. Le code ne reconstitue aucun profil de projectile depuis l'ancienne définition lorsqu'un ARM enregistré a un niveau invalide (mesh vide). Une arme cataloguée qui ne possède pas de présentation spéciale de projectile et les objets non enregistrés restent dans leur comportement de vol historique : les profils balistiques seront migrés séparément.

**Important :** les munitions du tir à l'arc sont encore créées dans `GridTurnManagerPlayerActions.cpp` à `ItemLevel = 0`. Leur trajectoire et leur mesh existants sont explicitement conservés **dans 06.3R**, car convertir implicitement ces instances en N1 endommagerait la continuité des flèches récupérables et des sauvegardes. Un jalon spécifique devra migrer la fabrication et l'identité des munitions, puis le vol de ces projectiles. Il ne faut pas considérer la migration complète des projectiles comme terminée.

Aucune modification des `.uasset`, `SaveGame`, `WBP` ni de la feuille de personnage. La validation automatique ne remplace pas un contrôle PIE GPU des flèches et armes lancées.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalProjectileVisualByActualLevel` utilise le vrai `DA_ItemSystem`, les ARM054/ARM057/ARM058 aux niveaux N1 et N20, l'absence de repli visuel au niveau N0 et le maintien du lecteur historique pour les identités absentes. `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` doivent être `success` sur le SHA exact du jalon.
