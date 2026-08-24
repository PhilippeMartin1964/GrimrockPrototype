# MON20.10.1 — Balance / Regression Audit

Date : **24 août 2026**  
Statut : **AUDIT TERMINÉ — HARDENING CIBLÉ REQUIS AVANT CLOTURE MON20**

## 1. Objectif

Auditer l'ensemble de MON20 avant sa clôture finale, sans introduire de nouveau système parallèle ni retuner arbitrairement des valeurs déjà contractualisées.

MON20.10 doit couvrir :

```text
Recruitment
Story Companion
Custom Recruit
Skills
Talents
Requirement projection
Skills UI
Persistence v8
Regression PIE
```

La clôture doit distinguer :

- les paramètres structurels déjà décidés ;
- les futurs réglages de contenu qui nécessitent de vrais assets de jeu ;
- les régressions runtime réellement observées.

## 2. Baseline de balance structurelle

### 2.1 Taille du groupe

Le maximum de **6 personnages actifs** est un contrat explicite de MON20.1 et reste cohérent avec l'autorité actuelle :

```text
FGridPartyInventoryState::MaxActiveCharacters = 6
FRPGPartyRecruitmentService::TryRecruitFromPool()
    -> PartyFull lorsque ActiveCharacters.Num() >= MaxActiveCharacters
```

Décision MON20.10 : **ne pas modifier cette valeur**.

Le fait que Legend of Grimrock utilise traditionnellement quatre personnages n'est pas une raison suffisante pour casser le contrat déjà validé du projet.

### 2.2 Recrutement

Les invariants existants restent les bons garde-fous :

```text
CharacterId valide et unique
nom 1..24 caractères
RaceId / ClassId définis
Level >= 1
Experience >= 0
attributs RPG initialisés
hotbar 10 slots normalisée
groupe plein -> rejet atomique
```

Aucun coût en or ni limite économique supplémentaire n'est introduit dans MON20.10. Ce serait un nouveau système de gameplay, pas un équilibrage de clôture.

### 2.3 Skills

Le modèle actuel utilise :

```text
URPGSkillAsset::MaxRank = 5 par défaut
rang sparse : absent == 0
rang valide : 1..MaxRank
Skill Check = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

Les seuils `FRPGSkillRequirementGrant::MinimumRank` doivent rester dans `1..MaxRank`.

Décision MON20.10 : conserver ces bornes. Le repository audité ne possède pas encore un catalogue de Skills de production suffisamment riche pour justifier un retuning compétence par compétence.

### 2.4 Talents

MON20.7 a volontairement réutilisé la progression MON15 :

```text
FRPGClassProgressionChoiceDefinition
MinimumLevel >= 1
PointCost >= 1
PrerequisiteChoiceIds[]
GrantedRequirementIds[]
```

et :

```text
FRPGClassProgressionLevelGrant::ChoicePointsGranted >= 0
```

Décision MON20.10 : aucune seconde économie Talent et aucun nouveau `TalentPoints`.

Le tuning détaillé des coûts et arbres reste data-driven dans les assets de classe.

### 2.5 RequirementIds / actions

Les RequirementIds restent des capacités binaires dérivées :

```text
Class progression
+ Talents
+ Skill rank / thresholds
+ ItemTags
    -> SatisfiedRequirements
    -> FGridCombatActionCatalog
    -> MissingRequirements
```

Aucun RequirementId dérivé ne doit être sauvegardé.

## 3. Régression globale MON20 attendue

Avant MON20.10, les familles validées représentent :

```text
MON20.2   6
MON20.3   6
MON20.4  18
MON20.5  23
MON20.6  24
MON20.7  24
MON20.8  24
MON20.9  24
-----------
TOTAL   149 tests
```

La campagne de référence de clôture sera donc :

```text
Grimrock.MON20
```

Baseline avant nouveaux tests MON20.10 :

```text
149 / 149 Success
0 Fail
0 Error
```

Si MON20.10 ajoute un test de hardening, le total final devra être augmenté explicitement dans la documentation de validation.

## 4. Audit du dernier PIE MON20.9.5

Le Continue réel sur :

```text
Slot=GrimrockParty
SourceVersion=8
TargetVersion=8
Result=Accepted
CharacterCount=2
```

est sain et a permis de clore MON20.9.

Trois diagnostics distincts ont toutefois été observés.

### 4.1 `GrimrockParty_2` rejeté

```text
SlotProbe Slot=GrimrockParty_2
Result=Rejected
Reason=IncompatibleSave
```

Cause : snapshot secondaire ancien/incohérent ne contenant pas les états de progression requis.

Classification : **donnée locale obsolète correctement rejetée**.

Action MON20.10 : aucune relaxation de validation. Le fail-closed est le comportement correct. Le slot peut être supprimé localement lorsqu'il n'est plus utile.

### 4.2 `CustomRecruiter_Service has no RuntimeActorClass`

L'archetype MON20.5.6 est volontairement data-only :

```text
Gameplay Type        = CustomRecruiter
Runtime Actor Class  = None
Runtime Interactable = false
```

Classification : **warning de diagnostic bruité, pas défaut fonctionnel**.

Action MON20.10 : ne pas ajouter un Actor inutile uniquement pour faire disparaître le warning. Une éventuelle passe de log hygiene pourra reconnaître explicitement les targets data-only.

### 4.3 Monster restore rejeté par `PartyOccupiesCell`

Le dernier Continue a produit :

```text
[GridMonsterSpawn] Skipped ... Reason=PartyOccupiesCell
[GridMonsterState] MissingActor ...
```

L'état final du niveau rapportait également deux monstres morts.

Ce cas entre en conflit avec le contrat MON17.8.6 :

```text
monstre mort restauré
    -> aucune occupation
    -> aucune collision
    -> mesh caché
    -> Actor runtime conservé
```

Or `AGridLevelRuntimeActor::AddMonsterSpawnActor()` applique actuellement les conflits de cellule **avant** `RestoreRuntimeMonsterState()`, sans distinguer un `RestoreState` mort.

Conséquence : un monstre mort persistant peut ne pas être recréé si sa cellule est désormais occupée par le groupe ou un monstre vivant, alors qu'il ne devrait occuper aucune cellule après restore.

Classification : **régression runtime réelle et bloquante pour une clôture globale propre**.

## 5. Hardening requis

### MON20.10.2 — Dead Monster Restore Occupancy Hardening

Objectif : garantir qu'un `FGridRuntimeMonsterState` restauré mort ne soit pas rejeté par les gardes d'occupation destinés aux monstres vivants.

Contrat :

```text
RestoreState mort
    -> autorisé même si PartyOccupiesCell
    -> autorisé même si cellule occupée par un monstre vivant
    -> RestoreRuntimeMonsterState()
    -> Dead
    -> aucune occupation
    -> collision désactivée
    -> mesh caché
    -> Actor runtime conservé
```

À l'inverse :

```text
RestoreState vivant
    -> garde PartyOccupiesCell conservée
    -> garde MonsterOccupancyConflict conservée
```

Un test Automation dédié devra verrouiller cette distinction.

## 6. Découpage MON20.10

```text
MON20.10.1 — Balance / Regression Audit                    TERMINÉ
MON20.10.2 — Dead Monster Restore Occupancy Hardening      PROCHAIN
MON20.10.3 — Log Hygiene / Known Diagnostics               À FAIRE
MON20.10.4 — Full MON20 Automation Regression              À FAIRE
MON20.10.5 — Final PIE / MON20 Closure                     À FAIRE
```

MON20.10.3 ne doit modifier le code que si le bruit de log peut être supprimé sans masquer de vrais défauts. Le warning data-only CustomRecruiter peut aussi rester documenté si une correction générique serait trop large.

## 7. Critères de clôture MON20

MON20 ne pourra être marqué **CLOS** qu'après :

```text
[OK] contrats de balance structurelle audités
[ ] dead monster restore occupancy corrigé et testé
[ ] campagne Grimrock.MON20 entièrement verte
[ ] régressions monster persistence ciblées vertes
[ ] PIE Continue sans MissingActor dû à un monstre mort restauré
[ ] Recruitment / CustomRecruit / Skills page encore fonctionnels
[ ] documentation de synthèse et roadmap synchronisées
```

## 8. Hors scope

MON20.10 ne doit pas :

- créer de nouvelles classes jouables ;
- créer un catalogue complet de Skills de production ;
- modifier arbitrairement les classes MON15 ;
- créer une nouvelle économie Talent ;
- ramener le groupe à quatre membres sans décision de design séparée ;
- corriger des systèmes sans rapport avec les régressions réellement observées.

## 9. Suite immédiate

```text
MON20.10.2 — Dead Monster Restore Occupancy Hardening
```
