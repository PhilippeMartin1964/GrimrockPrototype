# ARMEMENT01.9.3 — Choix du kit de départ dans le wizard

> **Statut au 22.10.2026 — IMPLÉMENTATION ACTIVE, CONTENU HISTORIQUE.** Le wizard et la sauvegarde utilisent encore les 12 kits ARMEMENT01.9.2 ; ce document reste la référence du **fonctionnement C++/UMG**. Les 12 noms et les assertions « deux kits » ne décrivent **pas** les 18 futures recettes. La migration et la nouvelle localisation sont différées à l'activation ; voir [ARMEMENT01.9.6.1](ARMEMENT01_9_6_1_CLEANUP_CONSOLIDATION.md). Ne pas supprimer les chaînes ou identifiants anciens.

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


## Correctif ARMEMENT01.9.3 — Localisation des kits

**Autorité :** `Localization/Source/UI.csv` est le seul fichier d'authoring
textuel. Le runtime lit `/Game/Localization/StringTables/ST_UI` via
`FUILocalizationText`. Le `.uasset` ne doit jamais être édité à la main.

Le groupe de clés `CharacterCreation.StartingKit.*` contient 44 entrées :
12 noms de kits, 20 noms d'emplacements, 12 formats/libellés/erreurs.
Les kits intégrés conservent leurs identifiants immuables (`KitId`), mais
leurs `DisplayName` proviennent de `ST_UI`. Les objets conservent leur
`UGridItemDefinitionAsset::DisplayName` authoré : **ne pas dupliquer leurs
noms dans la String Table UI**.

- Étape Classe : la liste affiche uniquement le nom localisé ; les objets
  sont présentés avec `ItemRow`, `ItemRowWithLevel`, les slots sont
  localisés (notamment `MainHand → Main principale`) et la référence
  technique est un format localisable.
- Étape Résumé : `SummaryFormat` inclut le nom du kit et le détail,
  sans dépendre de l'existence du ComboBox.
- `Text_StartingKitTitle` est **statique** : lier directement sa propriété
  `Text` à la clé `CharacterCreation.StartingKit.Title` dans `ST_UI`
  depuis le Designer (Referenced Text / String Table / Key). Ne pas ajouter
  de `SetText` C++ qui écrase la référence.
- Les erreurs affichables à la création utilisent `Unavailable`,
  `InvalidKit`, `ConflictingDefinition` ; le détail des échecs du service
  reste dans les logs C++. Les chaînes diagnostiques `FString OutError`
  des services ne sont **pas** des libellés d'interface et ne sont pas
  injectées directement dans les widgets.

### Matérialiser ST_UI avant les tests

Le commit C++/CSV ne peut pas inclure la régénération locale du `.uasset`
Unreal. Après `git pull --ff-only origin master` et après avoir fermé
Unreal Editor :

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation

& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
```

Le premier import doit afficher `Changed=true`. Le deuxième, lancé
à l'identique, doit afficher `Changed=false` (idempotence).

Puis valider uniquement :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.Localization"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.WizardSelection"
```

Résultats attendus : **2/2** et **5/5**. Ne pas les déclarer validés
avant les logs locaux. Une fois réussis, vérifier que seul
`Content/Localization/StringTables/ST_UI.uasset` est modifié et le
committer via Git LFS dans **un commit séparé sur master**.

Ne pas lancer `RunLocalizationSmoke.ps1` pour cette migration étroite :
le script smoke complet modifie d'autres artefacts de localisation.


## ARMEMENT01.9.3 — Correctif des retours à la ligne CSV/ST_UI

Diagnostic du journal local du 22 octobre 2026 :
- \`Grimrock.RPG.ARMEMENT01.9.Localization\` : 2/2 OK ;
- \`Grimrock.RPG.ARMEMENT01.9.WizardSelection\` : un échec de
  \`PlayerFacingKitLabels\` sur le préfixe du Résumé.

Cause : \`FCsvParser\` conserve \`\\n\` comme deux caractères dans la valeur
CSV ; l'importeur historique copiait cette séquence littéralement dans
\`ST_UI\`. Or le contrat de LOCALIZATION01.3 requiert des échappements
de style C pour les textes multilignes. Le rendu attend un saut de ligne
réel entre le nom du kit et ses objets.

Correction dans \`FUILocalizationCsvAuthoring::ParseAndValidateCsv\` :
- décodage strict des échappements \`\\n\`, \`\\r\`, \`\\t\`, \`\\\\\` dans la
  colonne \`SourceString\`, avant validation et projection ;
- refus explicite des séquences inconnues et des antislashs finaux ;
- mêmes invariants de projection et d'idempotence ;
- aucun changement des clés ou du vocabulaire de \`UI.csv\`.

Deux tests de régression renforcés : analyse CSV/projection via
\`Game.UI.Localization.LOCALIZATION01.CsvValidationContract\` et
\`Grimrock.RPG.ARMEMENT01.9.Localization.RuntimeStringTableProjection\`.
Le test \`WizardSelection.PlayerFacingKitLabels\` reste strict sur la
présence d'un **véritable** saut de ligne. Il n'est pas assoupli.

**Important :** après avoir mis à jour et compilé le C++, régénérer
\`Content/Localization/StringTables/ST_UI.uasset\` avec le commandlet
\`UIStringTableAuthoring\`. Les deux formats \`SummaryFormat\` et
\`DetailsFormat\`, ainsi que \`CharacterCreation.Summary.Full\` déjà
présent, contiendront alors des retours à la ligne effectifs.
Le binaire modifié est à valider et à committer via Git LFS **séparément**.

Validation ciblée après régénération :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Game.UI.Localization.LOCALIZATION01.CsvValidationContract"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.Localization"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.WizardSelection"
~~~

Ne pas considérer ces tests validés avant réception des résultats UE5.5.4.
