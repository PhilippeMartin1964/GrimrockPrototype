# ARMEMENT01.9.3 — Choix du kit de départ dans le wizard

## Portée et autorité

Cette tranche ajoute **le choix explicite d'un kit** au wizard C++ existant,
sans modifier le `.uasset` UMG ni les cinq étapes établies en CC7 :

`Race → Classe → Caractéristiques → Identité → Résumé`.

Le choix intervient sur `Classe`, le récapitulatif est lu sur `Résumé`.
Le contexte `CustomRecruit` demeure inchangé : seuls les héros du `NewGame`
ont une sélection de kit dans cette tranche.

## Contrat C++

- `FRPGCharacterCreationRequest::StartingEquipmentKitId` est le choix explicite.
  `None` préserve le kit par défaut et les anciennes requêtes CC2/CC7.
- `UGridPartyInventoryComponent::CreateInitialCharacter` choisit l'ID explicite
  s'il est défini, sinon le `DefaultStartingEquipmentKitId`. Le validateur
  transactionnel `FRPGStartingEquipmentService::TryBuildKit` refuse les IDs
  étrangers/inconnus sans altérer le groupe.
- `FGridCharacterInventoryState::StartingEquipmentKitId` conserve l'ID
  réellement attribué, dans `PartyInventoryState` sauvegardé.
- Les six assets de classe et leur douze kits (ARMEMENT01.9.2) sont lus via
  une **copie temporaire**. Aucun `DA_Class_*` original n'est réenregistré.
- Le niveau de l'arme est fixé lors de l'attribution (ARMEMENT01.9.1).
- Les règles de quantité, d'inventaire et d'armes à deux mains restent
  inchangées.

## API Blueprint exposée par URPGCharacterCreationWizardWidget

- `GetAvailableStartingEquipmentKits()` → tableau de `FRPGStartingEquipmentKit`
  (ID, nom et tableau d'items).
- `GetSelectedStartingEquipmentKitId()` → `FName`.
- `SelectStartingEquipmentKit(KitId)` → `bool`; refuse un ID absent de la
  classe actuelle.
- `GetSelectedStartingEquipmentKitDescription()` → texte multiligne décrivant
  le kit, ses objets, leurs destinations et niveaux.

La sélection est initialisée au kit par défaut, conservée lorsqu'on navigue
dans les autres étapes du wizard et remise au défaut lorsqu'on change de classe.

## Phase UMG manuelle (à faire dans Unreal après validation C++)

Ouvrir :

`/Game/GrimrockPrototype/Blueprints/UI/CharacterCreation/WBP_CharacterCreationWizard`

1. Dans `Panel_StepClass`, placer un `ComboBoxString` nommé exactement
   **`ComboBox_StartingKit`** et un `TextBlock` nommé
   **`Text_StartingKitDescription`**. Activer « Is Variable ».
2. Dans `Panel_StepSummary`, placer un `TextBlock` nommé exactement
   **`Text_SummaryStartingKit`**, avec `Auto Wrap Text` et une place
   suffisante pour deux lignes d'objets.
3. Compiler puis sauvegarder le widget dans Unreal. Capturer l'étape
   Classe et l'étape Résumé pour validation visuelle.
4. Les trois widgets sont `BindWidgetOptional` : aucun changement visuel ne
   survient tant que ces composants n'ont pas été ajoutés. Leur remplissage et
   leurs événements sont raccordés automatiquement par le parent C++.

Le champ `ComboBox_StartingKit` affiche uniquement le **nom lisible** des
kits (sans suffixe entre crochets). `Text_StartingKitDescription` énumère les
objets, leur emplacement et leur niveau **sans répéter le nom du kit**, puis
indique la `Référence technique : KitId` en fin de texte. Sur la page
`Résumé`, `Text_SummaryStartingKit` conserve le **nom du kit** suivi des
détails, car la liste déroulante n'y figure pas. Le choix est limité au
`NewGameMainHero` et ne modifie jamais les DataAssets.

**Ne pas changer le WidgetSwitcher ni le nombre d'étapes CC7.**
**Ne pas éditer de `.uasset` en dehors de l'éditeur Unreal.**
Les libellés de prévisualisation sont provisoires et devront entrer au
catalogue de localisation avant finalisation de l'interface.

## Tests ciblés

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.WizardSelection"
```

Cinq tests vérifient :
- options de rôdeur, sélection alternative, refus d'ID étranger et reset au
  changement de classe ;
- création avec kit alternatif, propriété SaveGame de `PartyInventoryState`,
  attribution unique et ownership ;
- refus atomique des IDs étrangers/inconnus ;
- compatibilité du kit par défaut et non-régression de `CustomRecruit`.
- libellés joueurs sans `KitId`, absence de répétition dans l'étape Classe,
  référence technique reléguée au détail et nom maintenu dans le Résumé.

L'interface UMG visuelle sera vérifiée par captures Unreal après tests C++.
Aucune campagne globale n'est demandée.
