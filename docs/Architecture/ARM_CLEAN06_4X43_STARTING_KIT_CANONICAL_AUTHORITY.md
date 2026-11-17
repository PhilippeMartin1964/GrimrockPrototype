# ARM-CLEAN06.4X.43 — Kits de départ : l'ItemId explicite domine toute ancienne définition

**Parent :** `d3adc0e99adb604c479569fdbf6dba233b863d93`. Lot cohérent : service métier, test sur
`DA_ItemSystem` réel, documentation. Aucun package natif ni SaveGame.

## Défaut

Dans `FRPGStartingEquipmentService::TryBuildKit`, une entrée
`FRPGStartingEquipmentEntry` portant un `CanonicalItemId`
explicite n'utilisait les règles canoniques de **MaxStack**,
émetteur initial et emplacement de main que lorsque
`ItemDefinition == nullptr`. Les presets comportant simultanément
l'ID enregistré **et** l'ancienne définition correspondante
pouvaient encore autoriser/refuser un stack ou une torche
selon les champs obsolètes, alors que niveau et poids étaient
déjà résolus depuis le registre.

## Correction

Dès qu'une entrée porte `CanonicalItemId` enregistré, l'autorité
fonctionnelle devient `FGridResolvedItem` au `ItemLevel`
physique explicite, indépendamment de la présence d'un ancien
pointeur. `MaxStack`, quantité, lumière par défaut et
`FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical`
suivent alors le registre. L'ancien pointeur est examiné
uniquement pour rejeter un **ItemId différent**, et reste
provisoirement recensé dans `OutDefinitions` afin de ne pas
modifier la phase de réhydratation des anciennes données.

Les entrées sans `CanonicalItemId` explicite conservent
provisoirement leur authoring et résolution historique.
Une erreur de niveau, stack, main ou identité rejette
toujours **toute** la transaction, sans octroi partiel.

## Régression

Nouveau test
`Grimrock.RPG.ARM.CLEAN06_4X43.CanonicalStarterIgnoresOldItemMetadata` :
classe/kit transitoires, *trois IDs existant réellement* dans
`DA_ItemSystem` mais anciennes définitions volontairement
contradictoires : épée N20 et éclairage inversé ; flèches
N0 ×3 déclarées non stackables par l'ancien asset ; torche
N0 OffHand avec ancienne main désactivée et ancienne lumière
désactivée. Le kit doit être accordé selon les règles canoniques,
avec ID/niveau/quantité/équipement/lumière inchangés.

Aucune des six classes de départ réelles n'est éditée dans
ce jalon ; la conversion des packages de kits et les
SaveGame v27 restent hors périmètre. La suite `Grimrock`
Windows doit réussir sur le SHA final avant tout autre lot.

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
