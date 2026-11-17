# ARM-CLEAN06.4X.26 — Validation de l'édition canonique sans pointeur historique

**17 novembre 2026.** Base : fffa04d9b0a0118d4edb72b1cb77c96b61f2f4e1. C++ Editor-only, test transitoire et documentation. Aucun binaire UE ni sauvegarde modifié.

## Objet

Après X.25, les loose items canoniques pouvaient apparaître dans l'aperçu, mais l'outil de validation demandait encore une ancienne ItemDefinition. Le setter Selected Object ItemLevel dépendait lui aussi du vieux pointeur.

## Modifications

- Nouveau contrôle CanAuthorCanonicalLevel(Registry, ItemId, PhysicalLevel) : ARM N1–N20, non-ARM N0, refus de tout ItemId inconnu, niveau hors plage ou registre manquant.
- SetSelectedObjectItemLevel reconnaît CanonicalItemId des loose items et ItemSpawns, et autorise le changement du niveau directement depuis ItemSystem ; les anciens auteurs non migrés conservent leur logique de niveau hérité et l'absence de sélection n'est plus déréférencée.
- ValidateCurrentLevel classe les items pointerless à partir de leur ID canonique au lieu de signaler systématiquement un ItemDefinition manquant ; toute contradiction avec une ancienne définition, ID inconnu ou niveau physique invalide produit une erreur précise.
- Les règles d'acceptation et contenus initiaux de réceptacles possédant déjà une identité canonique n'engendrent plus de faux avertissement pour un ancien pointeur absent.
- SetSelectedObjectItemDefinitionId reconnaît une identité canonique déjà présente ; la création d'une nouvelle identité avec niveau physique n'est pas ajoutée à l'UI dans ce jalon.

## Tests

Test du module Editor : Grimrock.RPG.ARM.CLEAN06_4X26.PointerlessCanonicalEditorValidation. Monde d'éditeur transitoire, vrai DA_ItemSystem, un loose ARM N20 et un ItemSpawn ammo N0 sans ancien pointeur. Validation sans faux warning, changement effectif vers N1, refus ARM N0 et ammo N1 sans mutation, et diagnostics contradictoire/ID inconnu. Les tests Editor historiques et toute la suite Grimrock restent exigés dans LocalAuto.

## Suites et barrières

06.4 toujours incomplète ; 06.5 bloquée. La palette et le panneau Selected Object exigent encore un cutover d'authoring complet, puis les données binaires réelles devront être migrées de manière native et transactionnelle. X.26 n'a converti aucun niveau, WBP, .uasset ou SaveGame. Ne pas relancer les anciennes migrations ARM-CLEAN06.4W/X5.
