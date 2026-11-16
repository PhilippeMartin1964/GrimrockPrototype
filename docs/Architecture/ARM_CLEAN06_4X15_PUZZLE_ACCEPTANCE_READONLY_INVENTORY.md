# ARM-CLEAN06.4X.15 — Inventaire des règles d'acceptation réelles (lecture seule)

## Justification

Jalons X.10 à X.14 validés sur `master` :
X.10 (`62dbfa08dea9`), X.11 (`56f09ecdaab2`),
X.12 (`4f6ed4c4d6eb`), X.13 (`d330d613d175`),
X.14 (`d9595b630837`). Point de reprise :
`docs/Architecture/ARM_CLEAN06_CURRENT_HANDOFF.md`.

Le retour PIE utilisateur confirme **le ramassage des armes,
les potions/flasques et les combats**. Le panneau de palette et
l'inspecteur Selected Object ont été contrôlés visuellement.
**Les réceptacles et leurs liens d'énigmes ne sont pas encore
confirmés par cette recette.**

## Frontières restantes

Les 95 références matérielles de 06.4W/X.5 **n'incluent pas**
les règles de sélection des réceptacles ou des serrures :
- `FGridReceptacleAcceptedItemConfig::ItemDefinition`
  n'a encore **aucun champ `CanonicalItemId`** ;
- `AGridReceptacleActor::InitializeRuntimeWorldObject`
  résout les règles acceptées par `ItemDefinition->ItemDefinitionId` ;
- `FGridWorldObjectInteractionOverrides::ReceptacleRules`
  peut remplacer entièrement les règles de définition ;
- `FGridLockBehaviorParams::AcceptedKeyItems` et
  `AcceptedKeyIds`, ainsi que les surcharges d'instance
  `AcceptedKeys`, forment un second contrat d'énigmes ;
- `SGridEditorObjectInspectorPanel` permet encore de
  sélectionner les anciens `UGridItemDefinitionAsset` ;
- les diagnostics dans `GridLevelEditorActor_Validation.inl`
  exigent encore des pointeurs pour certaines règles.

## Preuve automatique

Test `Grimrock.RPG.ARM.CLEAN06_4X15.RealPuzzleAcceptanceSourcesReadOnly` :
charge les quatre niveaux natifs et `DA_ItemSystem` (34 familles),
parcourt via l'Asset Registry les vraies définitions
`UGridWorldObjectDefinitionAsset`, puis compte :
règles de définition, surcharges d'instances, clés acceptées,
identités déjà dans le registre, sources absentes/inconnues et
noms de définitions du niveau non couverts. Consigne aussi
les formes `AcceptedKeyIds` déjà explicites.

**Aucune conversion, pas de changement de runtime, d'éditeur ou
de comportement d'énigme.** Les compteurs sont uniquement
publiés dans le journal de test UE local ; ils ne doivent pas
être inventés si l'agent ne les expose pas. Une suite verte
prouve la lecture et le maintien des DataAssets, pas l'absence
de sources historiques ni la validation PIE des réceptacles.

## Suite conditionnelle

Selon les comptages réels, construire un migrateur transactionnel
spécifique pour les sources d'acceptation réellement utilisées,
en distinguant règles par défaut et surcharges d'instance.
Interdire le fallback historique pour une identité persistée,
préserver `bAcceptAnyItem`, capacités, règles de clés, GUID
et liens. Tester insertion refusée/acceptée, retrait, capacité,
objets déjà contenus, `Save/Continue`, puis seulement supprimer
les pointeurs historiques. Aucun `.uasset`, `.umap`,
`.sav` ou Blueprint n'est modifié par X.15.
