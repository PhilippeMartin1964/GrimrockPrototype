# DOC-AUDIT02 — docs/Design / C++ Coherence Rebaseline

Date : **9 octobre 2026**  
État : **AUDIT DOCUMENTAIRE TERMINÉ — corrections courantes appliquées**  
HEAD C++ audité : **6f98a0ef5599f37d3d544529aa5a790d647e8251**

## 1. Périmètre

L'inventaire initial audité contient **521 documents Markdown** sous `docs/Design`. Ce rapport DOC-AUDIT02 devient le **522e** document lors de sa publication.

L'audit applique la politique documentaire déjà décidée par DOC-CLOSURE01 :

1. les documents **courants/canoniques** doivent correspondre au C++ actuel ;
2. les documents de **clôture** restent vrais sur le résultat qu'ils ont clos ;
3. les tickets datés sont des **snapshots historiques** et ne sont pas réécrits
   simplement parce que le projet a évolué ;
4. un ancien fichier nommé `CURRENT`, `FINAL` ou `CLOSURE` reçoit un
   bandeau `HISTORIQUE / SUPERSEDED` lorsqu'il pourrait être pris à tort pour
   l'API actuelle.

## 2. Méthode

La passe a combiné :

- inventaire exhaustif des 521 fichiers ;
- recherche transversale des symboles/contrats supprimés ;
- contrôle des statuts `CURRENT`, `EN ATTENTE`, `À FAIRE`, `CLOS` ;
- comparaison sémantique des références actives avec le C++ de `master` ;
- vérification directe des autorités Save/UI/RPG/WorldObject/Mouse/Audio/Lua ;
- comparaison des jalons ouverts avec la présence réelle de code.

Ce ticket est **documentation-only**. Aucun C++, Blueprint, DataAsset ou map
n'est modifié.

## 3. Ground truth C++ courant

### SaveGame

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 24
exact-match
```

Le Save courant ne contient pas de snapshot Quest.

État personnage :

```text
Durable
    Experience
    SelectedClassProgressionChoiceIds
    Attributes
    Resources
    SkillRanks
    KnownSpellIds
    StatusEffects
    inventory / hotbar / identité authored

Transient / reconstruit
    Level
    DerivedStats
    ClassDefinition / ClassDisplayName
    RaceDisplayName
    Portrait / ClassIcon
```

`LastAcknowledgedLevel` n'existe plus.

### Skills / Talents

```text
Talents
    URPGClassAsset::ProgressionChoices
    -> FRPGClassProgressionService
    -> FRPGClassProgressionTransactionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView

Skills
    FGridCharacterInventoryState::SkillRanks
    -> FRPGSkillService
    -> FRPGSkillPointService
    -> FGridSkillsPageService
```

Supprimés :

```text
FRPGTalentRuntimeService
FRPGSkillRuntimeService
FGridTalentEntryView
FGridSkillsPageView::Talents
GetTalentEntryCount / GetTalentEntry
CollectAutomaticSatisfiedRequirements
```

### UI

```text
WBP_GridPersistentHud
    -> navigation globale
    -> action bar persistante

WBP_GridCombatHud
    -> combat uniquement
    -> initiative / membres / PAM / fin du tour / targeting backend

WBP_GridSkills
    -> surface autonome Skills + Talents

WBP_GrimrockMenu
    -> shell temporaire Spellbook / Journal / Recipes / Codex
```

### Quest

`UGridQuestSubsystem` et `FGridCampaignQuestRuntimeState` existent, mais
aucun état Quest n'est encore stocké dans `UGrimrockPartySaveGame`.
MON21.4 reste donc réellement **EN ATTENTE** ; Journal et Codex restent futurs.

## 4. Références actives contrôlées

Les contrats suivants sont cohérents avec leurs symboles C++ actuels après la
rebaseline :

- `00_PROJECT_OVERVIEW.md`
- `PROJECT_COMPLETION_ROADMAP.md`
- `UI_ARCHITECTURE_CURRENT.md`
- `UI_GRIMROCK_MENU_CURRENT.md`
- `UI_COMBAT_WIDGETS_CURRENT.md` — contrat C++ courant, validation UMG/PIE
  toujours explicitement non revendiquée ;
- `01_GRID_OBJECT_SYSTEM.md`
- `11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md`
- `12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md`
- `10_MOUSE_INTERACTION_SYSTEM.md`
- `GRID_OBJECT_AUDIO_SYSTEM.md`
- `GRID_RELOCATION_DATA.md`
- `GRIMROCK_LOCK_SYSTEM.md`
- `LUA_SCRIPTING_LANGUAGE_REFERENCE.md`
- `RPG_SKILL01_FINAL_CLOSURE.md`
- `RPG_LEVELUX01_FINAL_CLOSURE.md`
- `RPG_ATTR01_FINAL_CLOSURE.md`
- `UI_RPG_DESC01_17_FINAL_CLOSURE.md`
- `UI_RPG_CODE_AUDIT01_DEAD_COMPATIBILITY_PATHS.md`

Les vérifications source ont confirmé notamment :

```text
ResolveLeftMouseInteraction / ResolveCursorItemHoverCursor  présents
IGridInteractableInterface                                  présent
PlayObjectAudioEventDetailed                                présent
bRelocationInitiallyEnabled                                 présent
MovingPartOverrides                                         présent
FGridObjectPaletteEntry::PaletteCategory                    présent
AcceptedKeyItems / bStartsUnlocked                          présents
GrimrockLua / compiler / runtime bridge                     présents
```

## 5. Incohérences trouvées

### Baseline documentaire

`README.md`, `00_PROJECT_OVERVIEW.md` et la roadmap se présentaient encore
comme la baseline du **4 octobre / 9045ef2** alors que le HEAD courant est
`6f98a0ef`.

Correction : ils distinguent désormais :

- **HEAD courant** : 6f98a0ef ;
- **dernière campagne globale + Shipping** : 9045ef2, 1026/1026 ;
- **validations ciblées postérieures** : RPG/UI du 8–9 octobre.

Aucune nouvelle validation globale/Shipping n'est inventée.

### SaveGame

Plusieurs documents TD07 nommés `CURRENT` décrivent encore v19/v20/v22 et
`LastAcknowledgedLevel`. Ils restent valides historiquement mais ne sont plus
une référence du schéma courant.

Correction : bandeaux historiques explicites. L'autorité courante est v24.

### Skills / Talents

Des clôtures MON20/UI-RPG anciennes documentent les façades
`FRPGSkillRuntimeService`, `FRPGTalentRuntimeService` ou la projection plate
`FGridTalentEntryView`.

Correction : ces documents conservent leur contenu d'époque mais sont marqués
superseded pour l'architecture courante. La suppression effective est documentée
par UI-RPG-CODE-AUDIT01.

### UI

Le bandeau de `UI_ARCHITECTURE_TARGET.md` indiquait encore que la navigation
persistante appartenait au Combat HUD. Le C++ courant la porte dans
`UGridPersistentHudWidget`.

Correction appliquée.

`UI_COMBAT_WIDGETS_CURRENT.md` décrivait déjà le contrat UNIFY02 dans son
corps mais conservait UNIFY01 dans le titre. Le titre est aligné sur le contrat
courant sans modifier son état de validation : UMG/PIE reste à valider.

## 6. Documents historiques

Les tickets MON/TD/UI datés ne sont **pas** convertis artificiellement en
documentation courante. Une phrase comme « Save v7 » ou une API supprimée reste
légitime dans un document qui caractérise précisément le jalon où elle existait.

DOC-AUDIT02 n'ajoute un bandeau qu'aux fichiers dont le nom/statut pourrait
raisonnablement tromper un lecteur sur l'état actuel.

## 7. État de validation

Conclusion :

```text
C++ courant vs références actives docs/Design     COHÉRENT après corrections
Save current schema                               v24 exact-match
Quest persistence                                 non implémentée
Journal/Codex                                     non implémentés
Talent flat compatibility                         supprimée
Skill runtime compatibility facade                supprimée
Combat UNIFY02                                    contrat C++ courant
Combat UNIFY02 UMG/PIE                            validation non revendiquée
Last full global + Shipping                       4 octobre / 9045ef2
Current HEAD full global + Shipping               NON REVALIDÉ
```

Aucune modification de production n'est requise par cet audit. Les écarts
identifiés étaient documentaires.
