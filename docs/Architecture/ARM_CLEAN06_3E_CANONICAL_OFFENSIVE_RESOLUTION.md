# ARM-CLEAN06.3E — Résolution de l'attaque au moment de l'exécution

La construction des actions du HUD a été migrée en 06.3D ;
cette étape migre le **lecteur de profil offensif au moment de
l'attaque**, pour les 22 armes du registre.

Deux chemins du moteur MON12 sont concernés :
`ResolvePlayerOffensiveProfile` (attaque depuis un slot physique
ou sélection automatique) et
`ResolveCombatActionWeaponProfile` (action de classe
utilisant une arme équipée).

Pour un `ItemId` présent dans `DA_ItemSystem`, le moteur appelle
`UGridItemSubsystem::ResolveAttackFromEquippedInstance` avec
l'identité réelle du slot. Il reçoit les valeurs N1–N20,
dés, dégâts, précision, portée et scaling de l'unique
`DA_WeaponCatalogue`. Un résultat négatif **refuse** l'attaque,
sans recours à l'ancien DataAsset ni substitution de N1.

Un profil offensif `FGridOffensiveEquipmentProfile`
est construit **en mémoire uniquement** pour l'exécuteur MON12
actuel. Il ne devient ni un asset, ni une nouvelle source
persistante, ni un champ du `DA_Weapon_*`.
Les trois armes passives ne sont pas transformées en attaques ;
le combat à mains nues garde sa règle indépendante.

Les objets encore hors registre utilisent provisoirement leur
chemin de compatibilité historique. Double attaque, munitions,
projectiles, effets d'impact, sauvegardes et objets des niveaux
ont des chemins anciens résiduels : ce jalon n'annonce
pas la suppression de leur legacy.

Tests : les 19 profils concrets et l'identité des slots
sont couverts par `RealNativeRegistryAndActions` et
`EquippedInstanceAttackIdentity`, complétés par la suite
`Grimrock` Windows. Une **validation PIE ciblée**
sera demandée après les tests sur le commit exact, avant
de supprimer d'autres lecteurs de combat.

Aucun `.uasset`, `.umap`, SaveGame ni fichier hors du
périmètre des 22 armes n'est modifié.
