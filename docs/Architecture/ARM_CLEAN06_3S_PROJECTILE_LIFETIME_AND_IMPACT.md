# ARM-CLEAN06.3S — Durée de vol et point de chute canoniques

**Base :** 06.3R (visuel de projectile d'arme ARM par niveau, pas de migration d'arc).

Les armes lancées disposant réellement d'une présentation `ProjectileAppearance` dans `DA_ItemSystem` utilisent maintenant `FGridCanonicalProjectilePolicy::FlightLifetime` et `ImpactDropOffset` pour leur acteur projectile. Ces deux paramètres appartiennent à la progression numérique N1–N20 du seul `DA_WeaponCatalogue`, non aux copies `DA_Weapon_*.ThrowLifeSeconds` / `ThrowImpactDropOffset`. Le modèle transitoire `FGridCanonicalProjectileVisualReadView` associe désormais la présentation au `FlightPolicy` résolu pour le **même** ItemId et ItemLevel.

L'ancienne méthode reste en place pour les objets non enregistrés, les armes n'ayant pas de projectile authoré et les munitions de tir à l'arc tant que leur `ItemLevel=0` n'est pas migré explicitement. Aucun nouveau propriétaire d'état, aucune modification SaveGame et aucune correction forcée N1 ne sont introduits.

Les vitesses, arcs, règles de Force, collisions, impacts et dommages ne sont **pas** modifiés dans ce jalon. Le test automatisé `Grimrock.RPG.ARM.CLEAN06_3.CanonicalProjectileVisualByActualLevel` est étendu aux deux paramètres du vol sur ARM054, ARM057 et ARM058 N1/N20 ainsi qu'au refus N0. Les tests `Grimrock` complets et le build UE5.5.4 sont requis via VALIDATION-AUTO01 sur le SHA publié.

**Reste PIE :** forme, orientation, durée de vol et chute des armes lancées, puis préservation des flèches. Aucun `.uasset` ni fichier UI n'est touché.
