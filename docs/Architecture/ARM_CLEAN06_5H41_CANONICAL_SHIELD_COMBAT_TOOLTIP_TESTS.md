# ARM-CLEAN06.5H.41 — Traces de combat ARM075 et tooltips canoniques

## Migration

Les trois tests `Grimrock.RPG.ARMEMENT01.10.3.3` ne chargent plus `DA_Weapon_RoundShield_01`. Un `UWorld` temporaire avec `UGameInstance` et `UGridItemSubsystem` fournit le bouclier ARM075 N1 et son identité `Weapon_RoundShield_01`, son nom affiché et ses 5 points d'armure physique.

Les trois inventaires de test proviennent désormais de `AGrimrockPartyPawn`. Le bouclier est matérialisé avec `ApplyItemDefinitionToInstance` puis équipé en OffHand, sans `RegisterItemDefinition` historique. Les assertions de source du combat, de nom dans le journal, de statut « épuisé » et de texte du tooltip utilisent `FGridResolvedItem`.

## Contrats intacts

Les assertions détaillées de localisation ST_UI, traces d'absorption, affichage des valeurs avant/après, perte de l'armure avant HP, épuisement sur le deuxième coup, message historique immuable, multiples sources de protection (équipement et classe) et différences de textes restent en place.

Les autres tests du fichier, indépendants du bouclier, restent inchangés. Aucun code gameplay, `.uasset`, `.umap`, `.sav` ou HUD n'est modifié.

## Validation

Avant H.42, confirmer `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, avec Editor, Shipping et `Grimrock.RPG.ARMEMENT01.10.3.3`. Les 53 packages historiques H.23 sont conservés; les sept sauvegardes natives restent à vérifier avant retrait.
