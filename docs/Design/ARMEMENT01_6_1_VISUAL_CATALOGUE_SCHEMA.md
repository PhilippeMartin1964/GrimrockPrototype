# ARMEMENT01.6.1 — Schéma du catalogue visuel de l'armement

**Date :** 20 octobre 2026.  
**Statut :** implémentation C++ soumise à compilation et validation TD04.2 sur UE5.5.4.  
**Contrat directeur :** docs/Design/ARMEMENT01_6_CONTRAT_PRESENTATION_VISUELLE.md.  
**Référence de non-régression avant ce ticket :** 1 471 tests Grimrock, 0 avertissement, 0 échec.

## Périmètre exact

Ce ticket ajoute le **schéma** des silhouettes réutilisables et ses invariants.
Aucune icône, aucun StaticMesh, aucun asset binaire UE5, aucun widget et aucun
résolveur runtime ne sont créés dans ARMEMENT01.6.1. Le chargement, les
fallbacks d'apparence et leur raccordement à l'inventaire sont réservés à
ARMEMENT01.6.2 puis aux tickets suivants.

### Classe UPrimaryDataAsset

Chemin C++ :
- Public/Runtime/Combat/GridWeaponVisualCatalogueAsset.h
- Private/Runtime/Combat/GridWeaponVisualCatalogueAsset.cpp

Classe : UGridWeaponVisualCatalogueAsset, destinée à l'asset futur :

    /Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue

Un seul asset contient deux tableaux :

| Tableau | Entrée | Rôle |
|---|---|---|
| ArchetypeBindings | FGridWeaponVisualArchetypeBinding | ArchetypeId canonique ARM001..ARM082 → VisualId |
| VisualProfiles | FGridWeaponVisualProfile | VisualId → FamilyTag, BaseIcon, WorldMesh, EquippedMesh optionnel, transforms et ornement optionnel |

Deux ArchetypeId différents peuvent référencer un VisualId commun.
Une même clé ArchetypeId et un même VisualId ne peuvent être déclarés
plus d'une fois dans leurs collections respectives.

### Validation structurelle

IsValidCatalogue vérifie :
- au moins une entrée dans chacun des deux tableaux ;
- ArchetypeId exactement au format ARM001..ARM082 ;
- absence de doublons dans les bindings et les profils ;
- aucune référence à un VisualId absent ;
- VisualId et FamilyTag non vides ;
- au moins une ressource BaseIcon ou WorldMesh par silhouette ;
- si OrnamentMesh est renseigné, WorldMesh doit exister ;
- transforms finis et échelles strictement positives.

Une **couverture partielle est valide** pendant la phase pilote : le
catalogue de combat possède 82 archétypes, mais le catalogue visuel
n'a pas besoin d'en mapper 82 dès sa première version. La couverture
complète sera auditée en ARMEMENT01.6.6.

La vérification est purement **structurelle**, sans LoadSynchronous.
Une SoftObjectPath non vide peut désigner un asset non encore créé ;
la vérification de disponibilité effective et les fallbacks visuels
sont à traiter dans ARMEMENT01.6.2/6.3.

### Modèle transitoire futur

FGridResolvedWeaponVisual et EGridWeaponVisualResolutionError préparent
l'interface du résolveur. Tous les champs de FGridResolvedWeaponVisual
sont marqués **Transient**, dont le niveau d'item affichable lorsqu'une
instance réelle sera fournie. Il ne s'agit **pas** d'un nouvel état de
sauvegarde, d'une nouvelle source de dégâts ni d'une valeur de rareté.

### Activation explicite par définition

UGridItemDefinitionAsset reçoit :
- bUseWeaponVisualCatalogue = false par défaut ;
- WeaponVisualCatalogue, soft reference conditionnée par le booléen.

IsValidDefinition vérifie l'ID canonique et la référence uniquement
lorsque l'opt-in est actif. La validation de contenu d'un DataAsset
visuel ne se confond pas avec celle des dés de DA_WeaponCatalogue.
Aucun ancien item n'est migré ni automatiquement activé.

Les propriétés préexistantes Icon, WorldMesh et EquippedMesh restent
inchangées. Aucun override de matériau principal n'est introduit,
conformément à MATERIAL-OWNERSHIP01.

## Tests et procédure de validation

Cinq tests C++ automatisés nouveaux, filtre :

    Grimrock.RPG.ARMEMENT01.6

Noms :
1. VisualCatalogueSchema — mapping partagé et catalogue pilote valide ;
2. UniqueBindings — doublons et références pendantes ;
3. VisualProfileValidation — structure, silhouettes et transforms ;
4. ItemVisualOptIn — nouveau opt-in et protection Legacy ;
5. VisualOnlyNoGameplayAuthority — absence de dés, rareté, forge et
   niveau permanent dans la présentation.

Aucune ressource graphique non créée n'est chargée par ces tests :
les chemins de ressources sont des identifiants fictifs utilisés pour
la validation structurelle, **pas des assets réels à inclure au jeu**.

Validation locale :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.6"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Attendu **si compilation et exécution réussissent** :
- 5 / 5 nouveaux tests ;
- 1 476 / 1 476 tests de la campagne globale ;
- 0 warning d'automation, 0 échec, exit code 0.

Le warning habituel du compilateur Visual Studio n'est pas un warning
d'automation ; aucune réussite n'est présumée sans log de validation.

## Ticket suivant

**ARMEMENT01.6.2 :** FGridWeaponVisualResolver, résolution par
ArchetypeId → VisualId → FGridWeaponVisualProfile, fallback explicite
sur les champs visuels Legacy, gestion des références manquantes,
aucune mutation de combat/instance et pas de chargement par Tick.
