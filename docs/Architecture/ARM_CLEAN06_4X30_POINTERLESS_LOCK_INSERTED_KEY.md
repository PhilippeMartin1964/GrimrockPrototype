# ARM-CLEAN06.4X.30 — Clés canoniques sans ancien asset : présentation insérée dans une serrure

**Date :** 17 novembre 2026. **Parent :** `145a97b3f217e84a796a756db61f31c6902b8bde`. **C++ et tests automatisés en mémoire**, sans assets Unreal natifs ni SaveGame.

## Dépendance restante

Après X.29, les serrures peuvent accepter uniquement leurs `AcceptedKeyIds` enregistrés, mais `AGridWallLockActor::AttachInsertedKeyVisual` recherchait une ancienne `UGridItemDefinitionAsset` pour l'acteur visuel de la clé insérée. Sans cette définition, `AddContainedItem` pouvait fabriquer une représentation ID-only, sans initialiser de façon complète les composants `LightEmitter` et `VisualEffects` canoniques.

## Correction limitée

Pour une clé enregistrée dans `DA_ItemSystem` et ne possédant plus d'ancienne définition accessible, `AttachInsertedKeyVisual` prépare un `AGridItemActor` attaché à la serrure avant l'insertion logique, puis appelle `InitializeFromCanonicalItem` avec l'`ItemDefinitionId`, le niveau physique, une quantité unitaire et le GUID attribué à la représentation visuelle. Le préfixe `AddContainedItem` de X.22 reconnaît cet acteur déjà initialisé et ne réattache pas de source legacy. L'actor reçoit sa mesh, lumière et Niagara depuis le seul registre canonique. Toute initialisation invalide détruit le préacteur, ne consomme pas la clé et ne débloque pas la serrure.

Les autres chemins ne changent pas : si l'ancien `ItemDefinition` existe encore, sa présentation reste utilisée jusqu'à parité des profils natifs ; les clés réellement hors registre conservent leurs chemins historiques. Les clés empilées conservent le GUID nouveau déjà prévu pour le visuel de l'unité insérée ; une clé unitaire conserve son GUID. `CompleteUnlock`, ses liens, sons, messages, scripts et la politique de consommation ne sont pas modifiés.

## Régression

`Grimrock.RPG.ARM.CLEAN06_4X30.PointerlessCanonicalWallLockInsertedKeyVisual` : monde de jeu transitoire avec vrai GameInstance/ItemSystem, `AGridLevelRuntimeActor`, `AGrimrockPartyPawn` et `AGridWallLockActor`, sans aucun asset de clé historique découvert. Une clé `Key_Copper` N0 déverrouille la serrure par `TryUnlockWithContextItem`. Les vérifications portent sur l'identité, le GUID, l'absence d'`ItemDefinitionAsset`, la projection complète, le mesh, Niagara, lumière, l'absence de simulation physique/collision du visuel attaché. Une clé N1 invalide est refusée avant tout changement logique/visuel.

Le cycle réel PIE des serrures et leurs assets ne sont pas remigrés ; la suite automatique Grimrock reste obligatoire.

## Suites

Aucun champ historique supprimé. Les paramètres persistants des 22 ARM et les références des neuf DataAssets attendent toujours une transaction native et des tests de parité. **ARM-CLEAN06.4 incomplète ; 06.5 BLOQUÉE.**
