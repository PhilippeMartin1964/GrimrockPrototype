# MON20.9.5 — Automation / PIE Regression & Closure

Date : **24 août 2026**  
Statut : **EN VALIDATION — AUTOMATION 24/24 SUCCESS — PIE À FAIRE**

## 1. Objectif

Clore MON20.9 en consolidant la validation Automation déjà verte et en effectuant un dernier smoke test PIE de la frontière réelle SaveGame v8 / Continue.

MON20.9.5 n'ajoute pas de nouvelle abstraction de production et ne modifie aucun `.uasset/.umap`.

## 2. État Automation acquis

La campagne cumulative fournie le 24 août 2026 est entièrement verte :

```text
Grimrock.MON20.9.SkillPersistence        8/8 Success
Grimrock.MON20.9.ActivePoolPersistence   8/8 Success
Grimrock.MON20.9.RestoredConsumers       8/8 Success
---------------------------------------------------
TOTAL                                   24/24 Success
0 Fail
0 Error
```

Cette campagne couvre :

```text
Save snapshot Skill v8
migration v7 -> v8
active + CharacterPool
restore par CharacterId
projection SkillId
projection des seuils
combat action gating
MissingRequirements
Skills page
SelectedCharacterIndex
snapshot vide autoritaire
restore invalide atomique
```

## 3. Audit de la frontière SaveGame réelle

`UGrimrockPartySaveGame::Serialize()` utilise déjà le pipeline final MON20.9.

### Save

```text
PartyInventoryState
    -> CaptureStatusEffectState()
    -> ClassProgression / PendingLevelUps
    -> FRPGSkillPersistence::CapturePartySkills()
    -> CharacterSkillStates
    -> FRPGSaveMigrationService::ValidateCurrentSave()
    -> Super::Serialize()
```

### Load

```text
Super::Serialize()
    -> PrepareLoadedSave() / migration v8
    -> RestoreStatusEffectState()
    -> Restore class progression
    -> FRPGSkillPersistence::RestorePartySkills()
    -> SkillRanks runtime
    -> consommateurs MON20.8
```

Le log de load expose également :

```text
[GridSaveMigration] ... SkillCharacters=<N> Result=Accepted
```

Aucun second état Skill n'est maintenu en runtime.

## 4. Pourquoi aucun nouveau test C++ n'est ajouté ici

Les tests MON20.9.2 à MON20.9.4 couvrent déjà la logique non vide avec des définitions Skill contrôlées et les consommateurs associés.

Le repository ne possède pas encore, dans le répertoire RPG de production audité, d'asset Skill versionné destiné à un scénario joueur de bout en bout. Ajouter un `.uasset` uniquement pour MON20.9.5 élargirait artificiellement le scope et violerait la règle de ne pas créer d'asset sans nécessité.

La clôture suit donc le pattern MON20.8.5 :

```text
Automation logique complète
    + smoke test PIE réel
    + clôture documentaire
```

## 5. Smoke test PIE demandé

### Préparation

Si le fichier de sauvegarde courant contient un état que vous souhaitez conserver, faites une copie de sécurité de :

```text
Saved/SaveGames/GrimrockParty.sav
```

Le test doit être effectué hors combat, la politique MON18.9.1 refusant volontairement les sauvegardes de combat.

### Étape A — session avant sauvegarde

1. Lancer le jeu/PIE avec un groupe valide.
2. Ouvrir le menu Grimrock.
3. Ouvrir la page **Compétences**.
4. Vérifier que la page s'affiche sans erreur.
5. Changer de personnage si au moins deux personnages sont actifs.
6. Noter le personnage sélectionné et l'état visible de la page.
7. Fermer proprement la session hors combat afin de laisser le chemin normal de sauvegarde s'exécuter.

Attendu dans le log de sauvegarde :

```text
aucun GridSkillPersistence SaveCapture Result=Rejected
aucun GridSaveMigration SaveValidation Result=Rejected
```

### Étape B — Continue / chargement

1. Relancer le jeu.
2. Utiliser **Continue** / le chemin normal de reprise de la sauvegarde.
3. Vérifier que le groupe est restauré sans retour forcé au menu principal.
4. Ouvrir de nouveau la page **Compétences**.
5. Vérifier que la page se reconstruit correctement.
6. Changer de personnage et vérifier que la sélection reste fonctionnelle.

Attendu dans l'Output Log :

```text
[GridSaveMigration] Load SourceVersion=8 TargetVersion=8 ... Result=Accepted
```

Le champ :

```text
SkillCharacters=<N>
```

peut légitimement être `0` tant qu'aucun Skill de production n'est entraîné dans la sauvegarde actuelle.

### Étape C — contrôle de régression

Vérifier qu'après Continue :

```text
[ ] le groupe existe toujours
[ ] le personnage sélectionné peut être changé
[ ] la page Compétences s'ouvre
[ ] aucune erreur de migration SaveGame n'apparaît
[ ] aucune erreur GridSkillPersistence n'apparaît
[ ] les autres systèmes visibles du groupe restent utilisables
```

## 6. Critères de clôture MON20.9

MON20.9 pourra être marqué **CLOS** lorsque :

```text
[OK] SkillPersistence        8/8 Automation
[OK] ActivePoolPersistence   8/8 Automation
[OK] RestoredConsumers       8/8 Automation
[OK] TOTAL                  24/24 Automation
[ ]  PIE Save hors combat
[ ]  PIE Continue / load accepté
[ ]  PIE page Compétences après load
[ ]  PIE changement de personnage après load
[ ]  aucun warning/error GridSkillPersistence bloquant
```

## 7. Architecture finale attendue à la clôture

```text
FGridCharacterInventoryState::SkillRanks (Transient)
        ↓ capture
FRPGCharacterSkillSaveState keyed by CharacterId
        ↓
UGrimrockPartySaveGame::CharacterSkillStates
SaveVersion = 8
        ↓ load / migrate / validate
FRPGSkillPersistence::RestorePartySkills()
        ↓
SkillRanks runtime
        ├── Skill checks
        ├── Requirement projection
        ├── Combat action gating
        └── Skills page
```

Les `RequirementIds`, l'état des actions et le read model de la page restent dérivés et ne sont jamais sauvegardés.

## 8. Suite après clôture

Après validation PIE et clôture MON20.9 :

```text
MON20.10 — Balance / Regression / Closure
```

MON20.10 sera la dernière grosse étape de MON20 avant passage à MON21.
