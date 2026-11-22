# ARM-CLEAN06.5 — Retrait des derniers profils d'industrialisation et tests Runtime

Le lot convertit huit sources de tests du module Runtime qui dépendaient de
DA_WeaponCatalogue ou DA_WeaponVisualCatalogue. Le seul registre réellement installé,
DA_ItemSystem, reste l'autorité des nombres, niveaux, mains, classes, visuels et icônes.

Les suites existantes conservent leurs noms :
- CLEAN03 : 82 archétypes et 1640 résolutions N1-N20 ;
- ClassAffinity et ARMEMENT01.9 : six préférences de classe, affinités, erreurs ;
- HandActionParity : 22 armes N1-N20, attaques MainHand et OffHand ;
- ARMEMENT01.6 : Dagger, ShortBow et RoundShield, icônes, mesh et PBR,
  présentation tooltip, actor monde et preview de niveau.

Le test Editor ARM-CLEAN06.5D3 utilise également directement le ItemSystem final
pour les 22 armes, 19 lancers génériques et 3 projectiles spécialisés ;
le plan transitoire GridEditorArmHandThrowPlan ainsi que les paires
ProjectilePolicyProjection/ProjectileVisualProjection sont retirés. Il n'existe
plus de projection qui exige les DataAssets historiques de ces armes.

Aucun package Unreal ou fichier SaveGame n'est modifié.
Les deux Readables référencés par les sauvegardes v27 sont toujours protégés.
La suppression des 53 candidats reste suspendue à une validation native sur
le SHA final, aux sept saves privées, au PIE, Cook et Shipping.
