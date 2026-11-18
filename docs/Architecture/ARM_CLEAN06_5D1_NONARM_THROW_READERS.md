# ARM-CLEAN06.5D.1 — Lancers non-ARM sans ancien DataAsset

Parent `46d7dcd8113685aca7629543acffde62046b57c6` : deux statuts Windows SUCCESS. Les 12 champs GenericThrow avaient été transférés et audités en 06.4N puis matérialisés dans les définitions non-ARM. Leur projection runtime était absente : le préflight rejetait tous les non-ARM connus, et le spawn et le visuel utilisaient encore les anciennes valeurs ou des valeurs par défaut.

- `FGridResolvedItem` projette GenericThrow et la politique de main non-ARM exacte, sans nouvelle propriété authorée.
- Force/poids, préflight et spawn reconnaissent un N0 non-ARM, avec permission OneHanded explicite. Pas d'autorisation issue de bCombatRecoverable, d'un tag de catégorie ou d'une vieille copie.
- Les munitions suivent toujours leur entrée spécifique de lancement à distance. L'axe, pose, scale, rotation, durée et offset de leur vol et des objets non-ARM proviennent de GenericThrow.
- Le projectile non-ARM réinitialise sa présentation depuis la définition canonique et ne garde pas le pointeur historique, même si le caller en passe un.
- Tests du root réel : 26 définitions non-ARM, comparaison réfléchie de GenericThrow, main explicite, N1 refusé, spawn des objets à une main éligibles avec vieille définition contradictoire et vérification du mesh/pointeur.

Aucun changement de package natif ni de save. Les ARM sans politique de projectile spécialisé utilisent encore leur ancien ThrowSpeed/Arc et paramètres de vol : ne pas les retirer avant preuve de migration. Les lecteurs historiques de fixtures hors registre restent pendant la transition. PIE et collisions réelles restent à vérifier.