# ARM-CLEAN06.4X.32 — Précontrôle d'insertion des réceptacles et niveaux physiques

**Date :** 17 novembre 2026. **Parent :** `4f1f58913b21301a8dc10edbee25727605a06ee1`.
**Périmètre :** C++ runtime, test Automation et point de reprise ; aucun binaire UE/SaveGame.

## Défaillance ciblée

`AGridReceptacleActor::EvaluateItemAcceptance` vérifiait seulement
`FGridItemInstance::IsValid()` (GUID, ID, quantité, plage générale 0..20)
avant de répondre à `bAcceptAnyItem` ou `AcceptedItemDefinitionIds`.
Un ARM enregistré N0 et un non-ARM N1 pouvaient donc être annoncés
acceptés par `CanAcceptItemInstance` et `CanAcceptCursorItemFromParty`
alors que `TryInsertItemInstanceFromCursor` et `AddContainedItem` les
refusaient plus tard avec le lecteur physique strict.

## Correction

Après les refus déjà existants (instance invalide, insertion désactivée,
réceptacle plein), l'absence d'une règle d'acceptation correspondante reste
prioritaire : `NoMatchingAcceptanceRule` est conservé y compris pour une arme
N0 non listée, conformément au test historique X.16.1. **Seulement si la
règle existe** (ou que `bAcceptAnyItem` est vrai), la présence d'une
identité canonique dans `DA_ItemSystem` déclenche
`FGridReceptacleItemMetadataReadModel::Resolve(..., false)` :
ARM N1–N20 et non-ARM N0 uniquement, sans promotion N0 vers N1.
Un échec devient `InvalidItem` et ne peut pas se replier sur un ancien
pointeur. Les vrais ItemIds hors registre continuent d'utiliser la politique
historique ; les valeurs et les liens authorés ne changent pas.

**Important :** `UGridItemSubsystem::ResolveItem` accepte
intentionnellement certaines projections N1 non-ARM pour les lecteurs
historiques (contrat 06.3V). Ce jalon ne change **pas** cet usage :
seul le précontrôle d'une instance *physique* utilise le lecteur strict.

## Test

`Grimrock.RPG.ARM.CLEAN06_4X32.ReceptacleCursorPhysicalLevelPreflight`
utilise le vrai registre et un acteur réceptacle en monde transitoire :
dague ARM N1/N20, refus N0, clé cuivre N0 et refus N1, ItemId non inscrit,
instance invalide, insertion désactivée et absence de mutation du contenu.
Suite `Grimrock` obligatoire sur le SHA final.

Les règles de puzzle et les 22 profils ARM, les 95 objets de niveau/palette
et le schéma SaveGame v27 restent intacts. Ce test `-NullRHI` ne prouve
pas le rendu PIE/Chaos. **06.4 partielle ; 06.5 BLOQUÉE.**
