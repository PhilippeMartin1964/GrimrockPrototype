# UI-RPG06.1 — Audit Skills et catalogue de production

Date : **7 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Jalon parent : **UI-RPG06 — Unification Compétences + Talents UX**  
Statut : **SOURCE IMPLÉMENTÉE — BUILD / AUTOMATION / MATÉRIALISATION LOCAUX À VALIDER**

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

### UI-RPG06.2 — Matérialisation + page COMPÉTENCES

1. build local ;
2. Automation `Grimrock.UI.RPG06.Skills` ;
3. exécution locale du commandlet d'authoring ;
4. contrôle des 25 DataAssets ;
5. construction manuelle de la page dans `WBP_GridSkills` ;
6. affichage du nom, attribut directeur, rang/max et règle entraînée/non entraînée ;
7. description uniquement lorsqu'elle existe réellement.

Aucun achat de rang n'est ajouté dans cette tranche.

### UI-RPG06.3 — Finition et régression

- interactions de sélection/détail si elles apportent une valeur réelle ;
- cohérence visuelle avec TALENTS ;
- nettoyage des projections de compatibilité devenues réellement inutiles ;
- Automation et validation PIE de l'écran complet.

## 12. Validation locale requise

Ne pas considérer UI-RPG06.1 validé avant fourniture du log local.

Filtre dédié :

```text
Grimrock.UI.RPG06.Skills
```

Régression minimale recommandée après build :

```text
Grimrock.MON20.8.SkillsPage
Grimrock.UI.RPG04
Grimrock.UI.RPG05
```
