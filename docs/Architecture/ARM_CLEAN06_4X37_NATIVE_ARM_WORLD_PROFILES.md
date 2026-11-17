# ARM-CLEAN06.4X.37 — Migrateur natif des 22 profils ARM (préparé, NON exécuté)

**Date :** 17 novembre 2026. **Parent :** 1ba63f264892b20e4fc5eb72ce39998a86fb7d7b.
**Source déjà auditée :** ARM_CLEAN06_4X19_FINAL_LEGACY_DEPENDENCY_AUDIT.md, issue #182.
**Autorité :** ARM_CLEAN06_4X20_ARM_WORLD_PRESENTATION_PRECUTOVER.md.

## Périmètre réel de ce jalon C++

Ce n'est pas un nouvel audit X.19 ni une réécriture automatique de contenu.
Le commandlet Editor-only GridArmClean064X37NativeArmWorldProfiles
prépare des **copies transitoires** et vérifie une jointure stricte :

- 22 véritables DA_Weapon historiques (hors Shuriken non-ARM et redirecteur ThrowingKnife) ;
- 22 ItemIds canoniques, 22 ArchetypeIds et 22 VisualIds distincts ;
- les 22 profils visuels EXISTANTS, sans changement d'icônes, mesh,
  transform, finition ou projectile ;
- **8 valeurs** du visuel persistant par ARM, copiées par FProperty
  de type identique : LightEmitter, VisualEffects, activation et
  paramètres du WorldSparkle (matériau, couleur, intensité,
  vitesse, variation) ;
- **2 propriétés** de physique monde par ARM : masse Chaos opt-in
  et tilt initial ; le poids numérique ne se copie PAS ;
- 82 archétypes inchangés hors de ces deux propriétés ciblées.

La régression Grimrock.RPG.ARM.CLEAN06_4X37.NativeArmWorldProfileStaging
charge les 22 vraies sources et les catalogues depuis le projet,
vérifie la projection en mémoire et détecte une contradiction physique
injectée uniquement dans un struct local. Aucune écriture de package
n'est réalisée par le test.

## Modes native Editor-only

**-Preflight** : lecture seule ; construit le plan et compte les
profils actuellement différents. Aucun SavePackage.

**-Apply** : uniquement après une invocation Windows explicite et
un manifeste privé de 24 backups SHA-256 sur un HEAD exact.
Unreal écrit par SavePackage au plus DEUX binaires :
CanonicalStage/DA_CanonicalWeaponVisualCatalogue.uasset et
CanonicalStage/DA_CanonicalWeaponCatalogue.uasset. Les 22 anciens
DA_Weapon ne sont jamais écrits. En cas d'échec, le lanceur
restaure les DEUX catalogues depuis la copie vérifiée.

**-Verify** : exécuté dans un SECOND processus UE, recharge les
vrais catalogues du disque et contrôle les 10 propriétés de
chacun des 22 ARM. Aucun SavePackage.

La procédure n'est **pas** exécutée pendant LocalAuto.
Le script Scripts/ApplyARM_CLEAN06_4X37_ArmWorldProfiles.ps1
débute par un préflight en lecture seule, vérifie master, HEAD,
origin/master, propreté Git, LFS, puis sauvegarde 22 sources
et 2 destinations hors dépôt avec leur SHA-256 seulement si
-Apply est explicitement choisi. Il conserve toutes les sauvegardes
privées et ne commit/push jamais.

Exemple de lecture seule, à n'utiliser qu'après les deux statuts
Windows SUCCESS sur le code X.37 :

    .\Scripts\ApplyARM_CLEAN06_4X37_ArmWorldProfiles.ps1 -RepoRoot D:\Development\GrimrockPrototype -EngineRoot D:\UE_5.5 -StateRoot D:\Development\GrimrockPrototype_Audits -ExpectedHead SHA_COMPLET_X37

Pour la migration effective, une seule intervention native
UE5.5.4/Codex est nécessaire afin de lancer explicitement
le même script avec -Apply, vérifier le rapport privé, puis
préparer un **nouveau jalon atomique X.38** pour les DEUX
binaires LFS. Ne pas réaliser cette écriture sur le simple
commit C++ X.37.

## Interdits et résultats

Ne pas toucher aux neuf packages historiques de X.19,
aux 107 pointeurs legacy, aux 95 occurrences de niveaux/palette,
aux WBP, aux 22 sources DA_Weapon ou aux SaveGames v27.
Après X.38, les tests de parité des vrais binaires, PIE,
Chaos/PIT02, Save/Continue et l'inventaire AssetRegistry
restent obligatoires avant tout cutover. Pas de suppression 06.5.

**Tant que les deux vrais catalogues UE ne sont pas écrits,
rechargés et validés, ARM-CLEAN06.4 est PARTIELLE et
ARM-CLEAN06.5 est BLOQUÉE.**
