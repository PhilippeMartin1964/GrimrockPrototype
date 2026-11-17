# ARM-CLEAN06.4X.29 — Serrures murales : AcceptedKeyIds fait autorité

**Date :** 17 novembre 2026. **Parent :** `474e622472fcb15a2f6a85f0ca0730d626a55a61`. **C++ et tests en mémoire uniquement.** Aucun package UE, règle réelle de puzzle, SaveGame ou matériau modifié.

## Motivation

`AGridWallLockActor::InitializeRuntimeWorldObject` réunissait `FGridLockBehaviorParams.AcceptedKeyItems` et `AcceptedKeyIds` par union. Pourtant les deux serrures réelles `DA_Lock_CopperWall` et `DA_Lock_IronWall` possèdent déjà leurs identités canoniques `Key_Copper` et `Key_Iron`. Après le cutover, un vieux pointeur à une autre clé pourrait étendre silencieusement les clés acceptées par l'énigme.

## Changement limité

Le modèle en lecture seule `FGridWallLockAcceptedKeyReadModel::Resolve` donne la priorité absolue à une liste `AcceptedKeyIds` **non vide**. Les noms vides sont omis, les doublons ignorés ; une liste explicitement renseignée mais mal formée ne déclenche **aucun fallback** vers les anciens assets. Si aucune liste d'identités n'existe, les anciennes définitions `AcceptedKeyItems` restent lisibles pour les objets historiques hors cutover. Il n'y a pas de second registre, de migration d'assets ni de nouvelle règle d'interaction.

L'acteur conserve son parcours existant : insertion visuelle, consommation, événement `Activated` et messages ; le changement ne porte que sur la constitution des IDs autorisés.

## Test

`Grimrock.RPG.ARM.CLEAN06_4X29.CanonicalWallLockAcceptedIds` exerce une vraie instance transitoire de `AGridWallLockActor`. Lorsque le comportement contient `AcceptedKeyIds=[Key_Copper]` et un ancien `AcceptedKeyItems=[Key_Iron]`, la serrure n'accepte que le cuivre. Une serrure n'ayant aucun ID explicite conserve l'ancien comportement iron-only. Un tableau explicite `[None]` rejette toutes les clés et n'ouvre pas de fallback permissif. Le modèle de lecture est aussi testé pour la déduplication. La suite complète `Grimrock` et les anciennes régressions lock-and-key sont obligatoires.

## Limites

Les anciennes références des serrures murales dans les packages existants ne sont **pas** retirées. La présentation de la clé insérée passe encore par `AGridReceptacleActor::AddContainedItem` et peut rechercher une ancienne `ItemDefinition` ; c'est une dépendance distincte à traiter avant la conversion de ressources. Aucune sauvegarde v27 ni scène PIE testée par ce jalon.

**06.4 non terminée ; 06.5 bloquée.**
