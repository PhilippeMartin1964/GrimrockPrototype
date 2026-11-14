# ARM-CLEAN06.3AF — Texte d'équipement de départ depuis ItemSystem

## Portée

Le widget C++ `URPGCharacterCreationWizardWidget`, parent du
`/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard`,
présentait encore `Entry.ItemDefinition->DisplayName` au lieu du nom du
catalogue canonique, même après 06.3Y qui unifiait les métadonnées des
**instances réellement attribuées**.

`FGridStartingKitDisplayReadModel` est une **projection en lecture seule** des
`FRPGStartingEquipmentEntry` ; elle réutilise
`FGridStartingKitStaticMetadataReadModel` et le **même**
`UGridItemSubsystem` configuré sur le `GameInstance` du frontend.

- ARM enregistré N1/N20 : libellé depuis `DA_ItemSystem`, niveau réel
  de l'entrée sans profil N1 local.
- ARM enregistré avec niveau d'édition 0 : hérite **une seule fois** du
  `DefaultWeaponItemLevel` historique, comme la création effective des kits.
  Il ne transforme **pas** systématiquement 0 en 1.
- Flèches enregistrées, et tout item canonique non-ARM : niveau réel **0**.
  Le type canonique l'emporte sur le `bUseWeaponCatalogue` transitoire.
- Entrée enregistrée invalide : aucun ancien nom/poids ni niveau inventé
  ; la ligne erronée n'est pas présentée comme une attribution valide.
- Item hors registre : affichage historique provisoirement inchangé.

Les clés de lignes existantes
`CharacterCreation.StartingKit.ItemRow` et
`CharacterCreation.StartingKit.ItemRowWithLevel` conservent exactement
leurs formats et leurs paramètres `Quantity`, `Item`, `Slot`, `Level`.
Il n'y a **aucune modification du Designer, du niveau, des assets ou de SaveGame**.

## Preuve

Test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitDisplayByActualLevel`
sur **le vrai** `DA_ItemSystem`, ARM014/ARM031/ARM075/ARM081
N1/N20, niveau hérité N4, ARM N0/default invalide, flèches N0 et
non-ARM N1 interdit dans un kit, item inconnu, source non mutée.
Les tests de `Grimrock.RPG.ARMEMENT01.9.StartingKits.*` et
`CanonicalStartingKitStaticMetadata` restent actifs.

## Workflow UI obligatoire

`docs/UI_WBP_LOCALIZATION_WORKFLOW.md` relu avant authoring.
- Source `Localization/Source/UI.csv` : **inchangée** (aucune nouvelle clé).
- `ST_UI.uasset` : **inchangé** (aucune matérialisation requise).
- `WBP_CharacterCreationWizard.uasset` : **inchangé** ; le TextBlock
  dynamique continue de recevoir son `FText` depuis C++.
- `Game/fr/en` : **aucune nouvelle chaîne**, validation du rendu culturel
  en PIE indépendante des tests `-NullRHI`.

**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
Les validations `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`
seront vérifiées sur le SHA exact après push. La qualité visuelle
des textes et les cultures fr/en ne sont pas prouvées par les
tests automatiques sans rendu GPU.
