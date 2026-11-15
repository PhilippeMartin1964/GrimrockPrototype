# ARM-CLEAN06.4Q — Piles d'objets non-ARM et véritable registre canonique

## Objectif et provenance

Le jalon 06.4P `29955df612e1478b94f30a706bac009df3c8d531` est validé avec trois contrôles Windows SUCCESS et le rapport [#149](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/149). Il reste **6 occurrences actives** des deux champs anciens `bStackable` et `MaxStackSize` (domaine `NONARM_STACK` de 06.4K), sur 11 ItemId non-ARM ayant 40 références monde. Ces 6 occurrences ne sont pas 6 identités.

Référence normative : `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`. Le contrat 06.3AS exige que toutes les opérations d'inventaire et de curseur interrogent `FGridCanonicalInventoryStackReadModel` : **un ID enregistré ne retombe jamais sur la capacité de l'ancien DataAsset**, même si celle-ci diffère. N0 est un niveau valide pour les objets hors ARM ; N21 n'est jamais accepté.

## Propriétaire de données — sans double autorité

L'ancien modèle porte deux champs, `bStackable` et `MaxStackSize`. Le résultat effectif est **une capacité unique**, `bStackable ? Max(1, MaxStackSize) : 1`. Le nouveau modèle n'a pas besoin d'un second booléen de pile : **capacité > 1** suffit.

La classe `UGridGenericStackableDefinitionAsset : UGridNonArmamentDefinitionAsset` ajoute un unique `MaxStack` (min. 1). `UGridItemSubsystem::ResolveItem` le résout dans `FGridResolvedItem.MaxStack`, et `UGridItemSystemAsset::Validate` refuse une capacité invalide. Elle est destinée aux composants et objets génériques, **pas** aux armes.

Les spécialisations existantes conservent leur propriétaire unique :
- `UGridAmmoDefinitionAsset.MaxStack` pour les munitions ;
- `UGridConsumableDefinitionAsset.MaxStack` pour les consommables ;
- `UGridGenericStackableDefinitionAsset.MaxStack` pour les piles génériques ;
- les items sans capacité authorée restent à **1**.

La classe générique temporaire utilisée par le commandlet pour l'ensemble des 11 sources est seulement une **sonde de capacité** : elle ne décide pas du type définitif des 11 objets. Les familles réelles seront vérifiées sur chaque identité avant l'écriture Editor, en conservant tous les autres comportements métier et les `ItemActionBinding`. En particulier, ne pas transformer une potion en « composant générique » sous prétexte qu'elle peut s'empiler.

## Audit UE5.5.4 sur les sources réelles

Le commit exact `feat(ARM-CLEAN06.4Q): validate generic nonarm stacks through canonical registry` déclenche `Scripts/AuditARM_CLEAN06_4Q_NonArmStackRegistryParity.ps1` après la compilation et l'Automation `Grimrock`.

Le commandlet Editor-only `GridArmClean064QNonArmStackParity` :

1. vérifie les reçus privés de **06.4K** et **06.4P**, les SHA exacts, la liste fermée des champs actifs et le refus de migration ;
2. rehash **84 sources binaires originales** et **84 copies privées immuables** de 06.4G ;
3. recharge le vrai registre (23 définitions) et les 11 véritables anciennes définitions absentes du registre, identifiées exclusivement par `ItemId` et chemin de source privé ;
4. recontrôle les **50 propriétés actives** des sources (valeurs réfléchies égales à 06.4K) ; recense les **6 propriétés de pile actives** et examine les **22 valeurs bool/capacité** en incluant les valeurs par défaut ;
5. construit un registre `UGridItemSystemAsset` **transitoire**, composé de 23 définitions existantes + 11 copies génériques en mémoire, avec les quatre catalogues réels inchangés ; le fait valider par `UGridItemSubsystem::RegisterItemSystem` ;
6. contrôle par le véritable `ResolveItem` la capacité et le poids unitaire pour **N0, N1 et N20**, le refus de **N21** : **44 sondes** ;
7. vérifie **33 politiques de quantité** (1, 2 et la capacité par item), sans modifier un inventaire utilisateur ni fabriquer de SaveGame ;
8. vérifie qu'aucun package source ne devient Dirty, que le worktree est propre et qu'aucune écriture `.uasset`, `.umap` ou `.sav` n'est faite ;
9. scelle les identités et capacités privées dans `stack-parity-private.json` et `stack-gate-private.json` ; publie uniquement les comptes agrégés sur GitHub.

**SUCCESS requis sur le SHA final :** `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4Q`. Le succès signifie *parité en mémoire du registre et des politiques*, pas conversion de vrais DataAssets ou Save/Continue.

## Correctif de fixture Windows (15 novembre 2026)

Le premier audit 06.4Q atteignait sa fin et retournait `result 0`, mais Unreal le classait en échec car la fixture créait `UGridItemSubsystem` directement sous `GetTransientPackage()`. Cette classe déclare `Within=GameInstance`; Unreal émettait un **handled ensure** à `UObjectGlobals.cpp:3362` (création sous un Outer interdit). La compilation et tous les compteurs métier de l'audit étaient déjà corrects.

Le correctif crée d'abord un **`UGameInstance` transitoire**, puis `UGridItemSubsystem` avec celui-ci pour `Outer`. Aucun `UGameInstance::Init`, aucun monde réel, aucune session et aucun SaveGame ne sont requis ou manipulés. Un test de `Registry->GetOuter()` sécurise cette contrainte avant la validation effective du registre.

**Ne pas déclarer Q terminé sans un nouveau trio de statuts Windows SUCCESS sur le SHA corrigé**, y compris la disparition de l'ensure dans le contrôle spécialisé.

## Limites — vers la clôture de 06.4

- **11 occurrences actives du domaine actions** demeurent non approuvées. Le rapport 06.4O contient 1 action d'équipement valide, 3 QuickItems valides, 1 présentation d'attaque valide et 54 champs imbriqués non défaut : ce matériel exige une traduction sémantique complète, **pas** une conversion automatique en un ActionId simplifié.
- Il faut décider explicitement de la famille finale de chaque ItemId ; préserver `ItemLevel`, `Quantity`, GUID, états mutables, armure consommée, lumière et sérialisation ; puis convertir les 84 binaires **seulement** avec Editor, sauvegarde immuable, manifest et rollback.
- **Plan resserré :** 06.4R = actions non-ARM, rapprochement des données/effets et liaisons avec les vrais catalogues ; ensuite une opération de migration Editor unique, avec dry-run, conversion et contrôles de parité. Le nombre exact de commits de cette dernière opération dépend de la faisabilité et de la sécurité révélées par les contrôles.
- 06.5 = suppression vérifiée du legacy après conversion ; 06.6 = PIE transversal, Save/Continue et Cook/Shipping.
- `migration_authorized=false`, aucune écriture binaire ni SaveGame autorisée dans le jalon 06.4Q. Le PIE validé par le propriétaire en 06.4L est une baseline **pré-migration** uniquement.
