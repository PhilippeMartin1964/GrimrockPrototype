# ARMEMENT-INDUS02 — Guide opérationnel du pipeline d'armement

**Statut : INDUS02.1 validé localement le 25 octobre 2026 (8/8 armes reconnues, 3/3 tests Industrial, 0 conflit, aucune écriture). INDUS02.2 ajoute Palette, proposition idempotente et tests ; compilation/validation locale INDUS02.2 EN ATTENTE.**
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
- exactement **un** ArchetypeId et une étape **Visual, Item ou Palette** ;
- un profil existant et valide dans le catalogue, art réelle validée (icône,
  mesh, MI sur slot, parent MI, BC sRGB ON, N/ORM sRGB OFF) ;
- aucune collision d'identité, de chemin ou de mapping ; aucune substitution de
  visuel et aucun écrasement d'un DataAsset existant ;
- Stage Visual : sauvegarde au maximum du seul DA_WeaponVisualCatalogue ;
- Stage Item : sauvegarde au maximum d'un seul nouveau DataAsset ;
- Stage Palette : sauvegarde au maximum du seul DA_ObjectPalette_Default, après une définition validée et un contrôle de collision d'EntryId et d'alias ;
- deuxième passage identique : PRESENT_SAME, aucune réécriture.

La séparation des **trois** écritures est intentionnelle : tant qu'une vraie transaction
multi-package sûre n'est pas démontrée, on ne présente pas un ensemble de SavePackage
comme atomique. Le script bloque tout changement Git supplémentaire non prévu.

**Non couvert par INDUS02.2 :** placement dans DA_GridLevel_00, sauvegarde UE des
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
ITEM_MISSING, PALETTE_MISSING ou CONFLICT, puis LEGACY_UNMANAGED dans le log le cas échéant. La nouvelle colonne Palette distingue MISSING, PRESENT_SAME et CONFLICT.
Un Planned ARM003 manquant est un état attendu ; il n'est pas une arme jouable.

Ensuite lancer la validation des contrats communs :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All

Tests ciblés : Grimrock.RPG.ARMEMENT.Industrial (**4 tests**, dont palette en lecture seule, idempotence et conflits simulés en mémoire).
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

Après Item, si le rapport indique PALETTE_MISSING, contrôler le plan puis
enregistrer **une seule nouvelle entrée**, sans modifier les entrées déjà
présentes :

    .\Scripts\WeaponPipeline.ps1 -Action Plan -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Palette -Apply

Les pilotes historiques ARM001/ARM045/ARM075 peuvent conserver leur
catégorie préexistante Items, mais toute nouvelle entrée est catégorisée
Weapon. Les doublons d'identifiant ou les entrées pointant vers un autre item
sont refusés. L'étape n'effectue **aucun placement dans le donjon**.

Compléter ensuite le placement dans l'éditeur Unreal avec le procédé
existant, et la recette PIE réelle : mesh/matériau, pickup unique,
infobulle, MainHand/OffHand, dépôt/récupération, combat PA/dégâts/portée,
sauvegarde/rechargement.

## 6. Critères de décision

### Validation locale

- Build Development Editor OK ; **4 tests Industrial** sans avertissement ni échec.
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


### Garde-fous spécifiques à la palette (INDUS02.2)

La palette est partagée par toutes les catégories d'objets. Le contrôleur
prend une **copie en mémoire** des entrées, y ajoute au maximum une entrée
\`EntryId=ItemDefinitionId, PaletteCategory=Weapon, DefaultItemDefinition=Item\`
et rejette les candidats invalides via \`ValidatePalette\` avant \`SavePackage\`.
Il **ne modifie jamais** les entrées préexistantes, même pour réparer une
catégorie : ce serait un autre ticket. Une catégorie \`Items\` préexistante
n'est admise que pour ARM001, ARM045 et ARM075, conformément à leurs
tests pilotes historiques. Les entrées déjà identiques produisent
\`PRESENT_SAME\` sans sauvegarde.

**Ne pas lancer -Apply avant réussite des quatre tests et du contrôle des
champs Palette dans les huit lignes actives.** Exemple de premier test local :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All
    git status --short

Résultat attendu : huit COMPLETE ; un Planned ARM003 ART_MISSING ;
zéro CONFLICT ; zéro SavedPackages ; **4/4** tests Industrial réussis ;
aucune modification Git. En cas d'écart, communiquer uniquement RESULT
non COMPLETE, SUMMARY et Automation summary. L'action Validate
**ne tente aucune écriture sur la palette**.

Pour les futures modifications binaires, inspecter la palette sous UE5
avant le commit ; un commit de code ne constitue pas une validation de
contenu.

## 7. Calendrier de consolidation

1. INDUS02.1 : commande générique Visual/Item et validation des huit armes **validées localement**.
2. INDUS02.2 : création idempotente d'entrées dans la palette et quatre tests génériques, **validation locale en attente**. Aucun placement automatique dans un niveau.
3. INDUS03 : enrichissement narratif du classeur (schéma compatible et versionné).
4. INDUS04 : infobulle d'inventaire fondée sur le ItemLevel réel.
5. INDUS05 : archiver les scripts/commandlets spécifiques après preuves
   d'équivalence ; ne pas supprimer l'historique ni les outils historiques
   avant d'avoir couvert les exceptions.

Ce guide remplace les recettes ad hoc pour une nouvelle arme de types supportés.
Les documents ARMEMENT01 anciens restent des archives techniques.
