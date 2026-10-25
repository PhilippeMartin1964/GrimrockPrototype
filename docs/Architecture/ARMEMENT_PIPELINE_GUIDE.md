# ARMEMENT-INDUS02 — Guide opérationnel du pipeline d'armement

**Statut : première version publiée en code source. Compilation UE5.5.4, rapports locaux et tests TD04.2 À EXÉCUTER SUR LE POSTE DE DÉVELOPPEMENT.**
Références : docs/Architecture/ARMEMENT_PIPELINE_CONTRACT.md (contrat cible), docs/Design/ARMEMENT_INDUS01_AUDIT.md (état de référence).

## 1. Objectif

Ne plus ajouter de script ou de commandlet C++ pour chaque arme normale.
Le seul point d'entrée public devient **Scripts/WeaponPipeline.ps1** et un seul
commandlet Editor **GridWeaponPipeline** gère les sources réellement déclarées dans
Config/Armement/WeaponProductionManifest.csv.

Ne remplacer aucune définition déjà produite. Les outils ARMEMENT01 historiques
restent disponibles le temps de démontrer l'équivalence et ne sont pas supprimés.

## 2. Manifeste : données, pas valeurs de combat

Le manifeste contient 16 colonnes fixes : ArchetypeId, ProductionState, BuildKind,
ItemDefinitionId, ItemPath, AttackId, VisualId, VisualFamily, IconPath, MeshPath,
MaterialPath, BaseColorPath, NormalPath, OrmPath, ClassId, EquipmentSlots.

- **Active** : les huit archétypes produits avec le modèle standard et des références explicites :
  ARM001, ARM002, ARM014, ARM015, ARM025, ARM031, ARM045, ARM075.
- **Planned** : ARM003 Stylet, avec conventions de chemins *proposées*.
  Ses fichiers graphiques ne sont PAS présumés existants, ni générés par le pipeline.
- **Legacy non encore migré** : les DataAssets Shuriken et ThrowingKnife. Ils sont
  détectés dans l'inventaire runtime si bUseWeaponCatalogue est activé, mais leurs
  identifiants et conventions spécifiques ne sont pas inventés. **Pas de migration
  automatique et pas de génération par le handler standard.**
- BuildKind autorisés : MeleeOneHand, MeleeTwoHand, Ranged, ShieldPassive.
  Les armes de lancer personnalisées restent hors autorisation d'écriture.
- Les ressources sont des **chemins UE complets exacts**, y compris les exceptions
  de noms des pilotes. Ne pas appliquer une transformation automatique Stem -> mesh.
- Aucune colonne de dés, dommages, poids, portée ou PA : toujours lire
  DA_WeaponCatalogue (N1 pour la définition, N1–N20 pour les instances).

## 3. Modèle de sécurité

Modes **Audit, Plan, Validate, Build sans -Apply** : lecture seule pour tout
Content/, code et données versionnés. Rapports CSV + JSON + log créés exclusivement
sous Saved/ARMEMENT-INDUS. Les hashes SHA256 de DA_WeaponCatalogue,
DA_WeaponVisualCatalogue et DA_ObjectPalette_Default sont contrôlés avant/après.

Build -Apply exige :
- exactement **un** ArchetypeId et une étape **Visual ou Item** ;
- un profil existant et valide dans le catalogue, art réelle validée (icône,
  mesh, MI sur slot, parent MI, BC sRGB ON, N/ORM sRGB OFF) ;
- aucune collision d'identité, de chemin ou de mapping ; aucune substitution de
  visuel et aucun écrasement d'un DataAsset existant ;
- Stage Visual : sauvegarde au maximum du seul DA_WeaponVisualCatalogue ;
- Stage Item : sauvegarde au maximum d'un seul nouveau DataAsset ;
- deuxième passage identique : PRESENT_SAME, aucune réécriture.

La séparation des deux écritures est intentionnelle : tant qu'une vraie transaction
multi-package sûre n'est pas démontrée, on ne présente pas un couple de SavePackage
comme atomique. Le script bloque tout changement Git supplémentaire non prévu.

**Non couvert par cette première implémentation :** création/édition automatique
de DA_ObjectPalette_Default, placement dans DA_GridLevel_00, sauvegarde UE des
matériaux et meshes, art 3D, mécanique spéciale, descriptions narratives,
infobulle offensive N1–N20. Leur production reste sous leurs contrats existants,
ou sera abordée dans les jalons ultérieurs sans toucher au runtime par surprise.

## 4. Premier contrôle local obligatoire — sans écriture

Fermer Unreal Editor, se placer sur master et vérifier que le dépôt est sain.

    cd D:\Development\GrimrockPrototype
    git branch --show-current
    git status --short
    git pull --ff-only origin master

Puis :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Audit -All

**Attendre le verdict réel**, ne pas affirmer le succès UE avant lecture du log.
Rapports :
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.csv
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.json
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.log

L'audit signale par arme COMPLETE, ART_MISSING, VISUAL_MISSING,
ITEM_MISSING ou CONFLICT, puis LEGACY_UNMANAGED dans le log le cas échéant.
Un Planned ARM003 manquant est un état attendu ; il n'est pas une arme jouable.

Ensuite lancer la validation des contrats communs :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All

Tests ciblés : Grimrock.RPG.ARMEMENT.Industrial (3 tests initiaux).
**Ne pas publier d'assets générés tant que ces tests n'ont pas réussi.**
Les tests vérifient le manifeste, l'autorité des profils N1 et les objets
déjà produits, sans PIE réel.

En cas d'échec, transmettre uniquement le SUMMARY, les RESULT bloquants et
le bilan des tests TD04.2 ; les longs logs n'ont pas à être recollés si les
lignes pertinentes suffisent. Corriger ce ticket dans un commit atomique
avant toute utilisation d'Apply.

## 5. Scénario d'une nouvelle arme — APRÈS validation du pipeline

Ajouter d'abord sa ligne unique au manifeste, créer en UE5 ses vrais
art assets selon les chemins explicitement déclarés, puis utiliser :

    .\Scripts\WeaponPipeline.ps1 -Action Plan -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003

Uniquement si la sortie indique Art=READY sans CONFLICT :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Visual -Apply

Relancer le préflight et contrôler le nouveau visuel :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003

Après enregistrement réel du visuel et confirmation de l'état ITEM_MISSING :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Item -Apply

Chaque Apply déclenche la suite de tests génériques. Aucun -Apply n'est
autorisé pour plusieurs armes. Un état CONFLICT ne peut jamais être
« réparé » en remplaçant arbitrairement l'asset existant.

Compléter ensuite la palette et le placement avec le procédé Unreal
existant, et la recette PIE réelle : mesh/matériau, pickup unique,
infobulle, MainHand/OffHand, dépôt/récupération, combat PA/dégâts/portée,
sauvegarde/rechargement.

## 6. Critères de décision

### Validation locale

- Build Development Editor OK ; 3 tests Industrial sans avertissement ni échec.
- Audit des huit lignes Active ; chaque CONFLICT/ART_MISSING documenté,
  jamais remplacé en silence.
- ARM003 Planned n'est pas interprété comme objet produit.
- Un audit sans Apply ne modifie aucun .uasset, .umap, WBP ni catalogue.
- Les anciens objets de lancer ne sont jamais réécrits par un handler
  généralisé ne sachant pas les gérer.

### Publication

Un ticket = un commit atomique sur master. Jamais de force push,
jamais de branche/PR, jamais de git add . pour masquer un conflit.
Ne pas publier de données Unreal produites à la place d'un rapport de
validation local. Les tests automatiques ne remplacent pas le contrôle PIE.

## 7. Calendrier de consolidation

1. INDUS02 (actuel) : commande générique, manifeste et contrats pour le
   pipeline de lecture et d'authoring Visual/Item, **tests locaux en attente**.
2. Poursuite INDUS02 : audit des écarts éventuels sur les huit assets réels,
   extension contrôlée de la palette/placements si nécessaire, baseline stable.
3. INDUS03 : enrichissement narratif du classeur (schéma compatible et versionné).
4. INDUS04 : infobulle d'inventaire fondée sur le ItemLevel réel.
5. INDUS05 : archiver les scripts/commandlets spécifiques après preuves
   d'équivalence ; ne pas supprimer l'historique ni les outils historiques
   avant d'avoir couvert les exceptions.

Ce guide remplace les recettes ad hoc pour une nouvelle arme de types supportés.
Les documents ARMEMENT01 anciens restent des archives techniques.
