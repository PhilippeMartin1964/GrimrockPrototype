# ARM-CLEAN06.4X.16.11 — PIT02 sans pointeur legacy pour les objets non-ARM canoniques

Parent Git : `afecaacf7dd0f0f6d81da358d0cd624aea3bc4a2` — double statut `UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS`.

## Correction C++

`GridLevelRuntimeActorWorldItems.cpp` :
- **À la sortie :** `TryRouteWorldItemThroughOpenPit` distingue une identité canonique connue et sa résolution à un niveau physique valide. Si l'identité existe mais que `ResolveItem` échoue (notamment ARM au niveau N0), refuse le transfert, même si l'appelant présente un ancien `UGridItemDefinitionAsset`. Pas de fallback.
- **Dans la file SaveGame :** tout objet non-ARM canonique (`DA_ItemSystem`) garde GUID/ItemId/ItemLevel/quantité/pose/lumière/lecture, mais la propriété `FGridPendingWorldItemState.ItemDefinitionAsset` est explicitement nulle. Pour ARM dont la présentation n'a pas encore basculé, et pour un ItemId non enregistré, le traitement historique est conservé.
- **À l'entrée :** `ApplyPendingInboundItemsForCurrentLevel` réévalue le registre avant de sélectionner la définition. Pour un non-ARM canonique, ignore même un pointeur legacy présent dans un ancien snapshot courant. Les ID connus mais invalides à leur niveau restent en file et ne matérialisent aucun acteur. Réutilise le dépôt déjà canonique qui configure mesh, lumière et Niagara.

**Aucune UPROPERTY SaveGame supprimée.** Schéma toujours v27 exact-match, sans compatibilité arrière inventée, ni conversion des données de l'utilisateur.

## Automatisation

`Grimrock.RPG.ARM.CLEAN06_4X1611.CanonicalPitSaveContinueAndRejectInvalid` ajoute un scénario PIT02 sur deux vrais niveaux éphémères et le vrai registre :
- torche N0 allumée transférée malgré une définition historique contradictoire ; vérification du pointeur nul dans la file ;
- ARM connu avec niveau N0 invalide et pointeur historique fourni : transfert refusé sans perte du contenu valide ;
- SaveGameToMemory puis LoadGameFromMemory (aucun .sav ouvert), déplacement vers le niveau inférieur, une seule torche du même GUID, lumière + Niagara canoniques, file consommée sans duplication.

Régression `Grimrock.Pit.PIT02` existante conserve sa voie legacy (Item_PIT02_Stone sans ItemId canonique).

## Conditions de sortie et suite

Compilations et suite d'automatisation Windows obligatoires sur le **SHA final** de ce ticket ; SilentTask et LocalAuto doivent être SUCCESS. Ce test -NullRHI n'est pas un test GPU/PIE.

Après ce jalon, rester prudent avec les armes ARM N1–N20, les états de projectile `ProjectileDefinitionAsset` et la conservation provisoire des anciens champs de contenu. Préparer la bascule de leur présentation et le contrôle Save/Continue sur copies avant toute suppression 06.5. La recette visuelle PIT02, notamment torche allumée et son effet au retour du niveau inférieur, requiert PIE.
