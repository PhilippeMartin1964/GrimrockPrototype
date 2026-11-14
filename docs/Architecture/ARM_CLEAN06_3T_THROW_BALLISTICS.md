# ARM-CLEAN06.3T — Vitesse initiale et arc des armes lancées depuis le profil canonique

**Précondition :** jalon 06.3S validé sur le SHA exact par VALIDATION-AUTO01.

`AGrimrockPartyPawn` utilise désormais une projection pure `FGridCanonicalThrowBallisticsReadModel::Resolve(ItemId, ItemLevel)` pour les **cinq chemins** de lancement d'objets : curseur, MainHand physique, sac physique, attaque équipée et attaque rapide. Pour ARM054, ARM057 et ARM058, la vitesse initiale et l'arc proviennent du `FGridCanonicalProjectilePolicy` N1–N20 de `DA_ItemSystem`. L'appel se fait **avant extraction ou consommation** et refuse un niveau canonique invalide plutôt que de lancer avec `DA_Weapon_*.ThrowSpeed`.

Une arme une-main sans projectile numérique spécialisé garde provisoirement sa politique de lancer physique générique ; les objets hors registre conservent leurs valeurs historiques. Les flèches et leur `ItemLevel = 0` sont inchangés ; elles passent par le circuit de tir à l'arc du `TurnManager` et non par les cinq lancers du Pawn.

La règle Force / poids et le coefficient `GetThrowSpeedScaleForStrength` utilisent encore l'ancienne définition ; leur migration atomique doit synchroniser l'autorisation UI, le curseur du contrôleur et les cinq entrées runtime. Ce jalon ne fait **pas** croire que le lancer physique complet a une autorité unique. Le facteur Force existant est multiplié à la vitesse initiale canonique sans le modifier.

Aucun changement des dégâts, de la consommation, du calcul d'impact, des assets, des sauvegardes ou des textes UI. `Grimrock.RPG.ARM.CLEAN06_3.ThrowBallisticsByActualLevel` compare le vrai registre et une ancienne définition volontairement contradictoire sur trois ARM lancées N1/N20 et N0 invalide. Exiger le succès de `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur ce SHA avant le jalon suivant.

La recette PIE contrôlera le lancer physique, l'attaque d'arme de jet et les flèches récupérables.
