# UI-RPG06.1 — Audit Skills et catalogue de production

Date : **7 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Jalon parent : **UI-RPG06 — Unification Compétences + Talents UX**  
Statut : **UI-RPG06.1 VALIDÉ — Automation 3/3 ; UI-RPG06.2A catalogue de production matérialisé (25 assets) et poussé**

## 1. Objectif

Commencer UI-RPG06 par les données réellement disponibles avant toute construction UMG.

La page `COMPÉTENCES` doit rester une projection du domaine Skills existant :

```text
URPGSkillAsset
        +
FGridCharacterInventoryState::SkillRanks
        ↓
FGridSkillsPageService
        ↓
FGridSkillEntryView[]
        ↓
UGridSkillsWidget
        ↓
WBP_GridSkills
```

Aucune seconde autorité de Skill, aucun registre parallèle et aucune règle de progression ne sont créés par l'interface.

## 2. Audit du HEAD de départ

HEAD audité :

```text
c4832024f6f9ce38c30ad2ef9559a6b1afe18dd8
UI-RPG05 materialize progression notification toast
```

Le code possède déjà un read model Skills réel.

`FGridSkillEntryView` exposait :

```text
SkillId
DisplayName
Description
GoverningAttribute
Rank
MaxRank
bTrained
```

Le rang autoritaire est :

```text
FGridCharacterInventoryState::SkillRanks
```

Le rang zéro reste représenté par l'absence d'entrée.

`FRPGSkillService`, `FRPGSkillRuntimeService`, les Skill Checks, la projection de RequirementIds et `FGridSkillsPageService` consomment déjà cette autorité.

## 3. Données qui n'existent pas encore

Le modèle courant ne contient pas :

- d'XP propre à une compétence ;
- de progression fractionnaire vers le prochain rang ;
- d'origine persistée d'un rang ;
- de solde autoritaire de Skill Points ;
- de transaction d'achat de rang.

La spécification `RPG_Class_Progression_1_20_v0_1.md` définit une future économie de Skill Points, mais précise elle-même que cette économie reste à implémenter.

UI-RPG06.1 n'en déduit donc aucun champ runtime et n'ajoute aucun bouton d'achat.

## 4. Valeur effective d'un Skill Check

Une valeur unique du type « compétence = 12 » ne serait pas autoritaire.

Un Skill Check réel dépend de :

```text
Rank
+ modificateur de caractéristique
+ modificateurs de Talents personnels éventuellement contextuels
+ éventuel modificateur de groupe
+ d20
```

Certains modificateurs dépendent notamment d'un contexte distance ou d'une catégorie de monstre.

La page Compétences peut afficher le rang brut et l'attribut directeur ; elle ne doit pas présenter un bonus contextuel comme une valeur permanente.

## 5. Dette bloquante découverte

Le dépôt ne contenait aucun `URPGSkillAsset` de production dans `Content/`.

Conséquence :

```text
FGridSkillsPageService::ResolveCanonicalSkillDefinitions()
    -> scan RPGSkill dans /Game
    -> aucun catalogue de production
    -> View.Skills vide en production
```

Construire le Designer avant de fermer ce trou aurait produit une page correcte techniquement mais sans données de production.

## 6. Catalogue canonique UI-RPG06.1

Le catalogue est défini côté Editor à partir de la table autoritaire de `RPG_Class_Progression_1_20_v0_1.md`.

Il contient exactement 25 compétences, avec :

```text
SkillId
DisplayName
GoverningAttribute
bAllowUntrainedChecks
MaxRank = 5
```

Les six compétences exigeant un entraînement sont :

```text
Skill_Lockpicking
Skill_Traps
Skill_Alchemy
Skill_Arcana
Skill_Runes
Skill_Religion
```

Les autres autorisent les checks au rang zéro.

## 7. Authoring Editor-only

Nouveaux contrats :

```text
FRPGSkillCatalogAuthoring
URPGSkillCatalogAuthoringCommandlet
```

Destination de matérialisation :

```text
/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/
```

Nommage :

```text
Skill_HeavyWeapons -> DA_Skill_HeavyWeapons
...
```

L'authoring configure uniquement les champs réellement définis par le catalogue :

```text
SkillId
DisplayName
GoverningAttribute
MaxRank
bAllowUntrainedChecks
```

Il préserve volontairement :

```text
Description
RequirementGrants
```

Ainsi, un futur enrichissement métier de ces champs ne sera pas détruit par un rerun du commandlet.

## 8. Read model enrichi sans nouvelle autorité

`FGridSkillEntryView` expose désormais également :

```text
bAllowUntrainedChecks
```

`FGridSkillsPageService` copie cette information depuis `URPGSkillAsset`.

Cette donnée permettra à l'UMG d'expliquer proprement :

```text
Rang 0 — test autorisé
```

ou :

```text
Rang 0 — entraînement requis
```

sans recalcul métier dans Blueprint.

## 9. Tests Automation ajoutés

Nouveau filtre :

```text
Grimrock.UI.RPG06.Skills
```

Tests :

```text
Grimrock.UI.RPG06.Skills.CanonicalCatalog
Grimrock.UI.RPG06.Skills.AuthoringPreservesExtensions
Grimrock.UI.RPG06.Skills.ReadModelTrainingPolicy
```

Ils vérifient :

- 25 SkillIds uniques ;
- attributs directeurs explicites ;
- exactement six compétences entraînées obligatoirement ;
- plusieurs entrées représentatives du contrat ;
- conservation de `Description` et `RequirementGrants` ;
- identité `RPGSkill:<SkillId>` ;
- projection read-only de la politique de check au rang zéro ;
- absence d'invention de rang.

## 10. Aucun asset binaire dans UI-RPG06.1

Cette tranche source ne modifie aucun `.uasset`.

La matérialisation des 25 DataAssets doit être exécutée localement sous UE5.5.4, puis contrôlée avant tout commit binaire.

## 11. Suite UI-RPG06

### UI-RPG06.2A — Catalogue de production — VALIDÉ

- build/Automation UI-RPG06.1 : **3/3, 0 warning, 0 échec** ;
- commandlet exécuté localement ;
- **25** `DA_Skill_*` créés ;
- second passage `Grimrock.UI.RPG06.Skills` : **3/3, 0 warning, 0 échec** ;
- commit production : `c3338d8941b81f9e7eafdad7bfb11fa48934d960`.

### UI-RPG06.2B — Renderer COMPÉTENCES

- widget de ligne réutilisable `UGridSkillEntryWidget` / `WBP_RPGSkillEntry` ;
- liste dynamique pilotée uniquement par `FGridSkillsPageView::Skills` ;
- nom, attribut directeur, rang/max et politique entraînée/non entraînée ;
- description uniquement lorsqu'elle existe réellement ;
- aucune économie de Skill Points, aucun achat de rang.

La matérialisation UMG reste manuelle et est décrite dans
`UI_RPG06_2B_SKILLS_PAGE_RENDERER.md`.

### UI-RPG06.3 — Finition et régression

- rendre les nouveaux bindings UMG obligatoires après matérialisation validée ;
- interactions de sélection/détail uniquement si elles apportent une valeur réelle ;
- cohérence visuelle avec TALENTS ;
- nettoyage des projections de compatibilité réellement inutiles ;
- correction des statuts documentaires UI-RPG03/04/05 devenus obsolètes ;
- Automation et validation PIE de l'écran complet.

## 12. Validation locale requise

Filtre dédié après UI-RPG06.2B :

```text
Grimrock.UI.RPG06.Skills
```

Régression minimale recommandée :

```text
Grimrock.MON20.8.SkillsPage
Grimrock.UI.RPG04
Grimrock.UI.RPG05
```
