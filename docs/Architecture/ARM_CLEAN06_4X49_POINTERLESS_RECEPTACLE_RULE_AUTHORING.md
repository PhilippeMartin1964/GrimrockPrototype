# ARM-CLEAN06.4X.49 — Authoring des règles de réceptacles : ItemId canonique sans ancien UObject

**Parent :** `e2f4db137dc21b3e27797bd8abe5470ac19eab28` ; X.48 validé par `SilentTask` et `LocalAuto`.
**Périmètre :** C++ Editor-only, test `X.162` et documentation.
**Pas de migration native .uasset ni de SaveGame dans ce lot.**

## Pourquoi

Les deux règles partagées de réceptacles déjà migrées vers
`CanonicalItemId` continuent à conserver dans les packages
`DA_Receptacle_TorchHolder` et `DA_Guardian` leurs anciens
`ItemDefinition`. Ce résidu sera retiré séparément sous
transaction UE sauvegardée. En revanche, la fonction
`FGridEditorCanonicalReceptacleRuleAuthoring::TryAssign`
recopiait encore systématiquement un ancien pointeur lorsqu'un
utilisateur sélectionnait une définition enregistrée dans le
système canonique. Le toggle `TryCopyDefaults` en réintroduisait
un aussi dans les overrides des placements.

Le runtime `FGridReceptacleAcceptedCanonicalReadModel` sait déjà
résoudre une règle par `CanonicalItemId` seul, valide la présence
de l'ID dans `UGridItemSubsystem` et refuse la contradiction
ID/pointeur. Il conserve le comportement historique pour les
IDs non enregistrés.

## Nouveau contrat d'édition

- `TryAssign` sur une définition **enregistrée** enregistre
  `CanonicalItemId` et **`ItemDefinition=nullptr`** ;
  les changements d'ID puis la remise à zéro ne laissent aucun
  pointeur antérieur
- sélection d'un ID réellement **hors-registre** :
  conserve le `ItemDefinition` historique, sans fabriquer de
  `CanonicalItemId`
- `TryCopyDefaults` exige le registre configuré ; pour une
  entrée avec ID existant ou une ancienne définition connue,
  **copie uniquement l'ID**, en conservant l'ordre, les valeurs
  d'acceptation et la règle d'origine sur le DataAsset partagé ;
  les règles hors registre restent sous source historique
- un ID canonique contradictoire avec sa source historique est
  **refusé atomiquement**, sans copie partielle
- l'inspecteur Slate `SGridEditorObjectInspectorPanel` affiche
  désormais `CanonicalItemId` même si le champ picker de l'ancien
  UObject est vide. Les noms hors registre sont signalés « Legacy »
  avec le picker historique toujours disponible

La suite `Grimrock.RPG.ARM.CLEAN06_4X162.CanonicalAcceptanceAuthoring`
est étendue : nouveau picker pointerless, remplacement de source,
copie pointerless des defaults, conservation de leur définition
originale, refus des divergences et des anciens IDs non enregistrés.
Aucun WBP ni texte de jeu n'a été modifié ; cette étiquette « Legacy »
est **exclusivement dans l'éditeur Slate**, pas dans une UI joueur.

## Suite protégée

L'audit X.48 a confirmé le **solde exact de 63 références
historiques** après X.46B, dont les **deux règles partagées**
des réceptacles. X.49 ne retire **aucune** de ces références
natives : il interdit seulement leur réécriture dans les nouveaux
overrides. Avant tout nettoyage natif, maintenir les preuves
d'acceptation de `Item_Torch` et `Gem_Blue`, les flags
`bAcceptAnyItem`, les liens et la recette PIE. La conversion des
deux définitions partagées nécessitera un lot et une validation
UE natifs séparés. Les 18 contenus initiaux, 33 entrées de palette,
3 overrides dormants, 2 clés de serrures et cinq loots restent
dans leur état actuel.

**Statut : 06.4 PARTIELLE ; 06.5 BLOQUÉE.**
