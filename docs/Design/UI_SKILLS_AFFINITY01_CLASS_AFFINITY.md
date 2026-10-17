# UI-SKILLS-AFFINITY01 — Class affinity column

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.1 SOURCE PRÉPARÉE — matérialisation ST_UI / WBP / données requise**

## 1. Objectif

Ajouter à la liste des compétences une colonne compacte qui indique
l’affinité de chaque compétence avec la classe du personnage sélectionné.

~~~text
Recommended -> ★★
Useful      -> ★
Neutral     -> ☆
Discouraged -> ★̸
~~~

La couleur de fond est propriété du Designer : vert pour Recommended/Useful,
neutre pour Neutral et rouge pour Discouraged. Les glyphes conservent une
différence lisible même sans perception de la couleur.

## 2. Autorité des données

`DA_RPGSkillPresentation` reste l’unique autorité des conseils de build :

~~~text
RecommendedClassIds
UsefulClassIds
DiscouragedClassIds
~~~

Ces métadonnées sont uniquement de présentation. Elles ne bloquent jamais
l’utilisation, l’achat de rang ou un test de compétence.

Règle importante : une classe absente des trois tableaux est `Neutral`.
L’absence ne signifie jamais `Discouraged`.

Les trois groupes doivent être disjoints.

## 3. Projection UI

`FGridSkillEntryView::ClassAffinity` est recalculé au refresh de
`UGridSkillsWidget` à partir de `View.ClassId` et de `SkillPresentationAsset`.
Aucune donnée gameplay ou SaveGame n’est modifiée.

La colonne Affinité participe au tri :

~~~text
Ascending  : Recommended -> Useful -> Neutral -> Discouraged
Descending : Discouraged -> Neutral -> Useful -> Recommended
égalité    : nom de compétence
~~~

## 4. Designer à matérialiser

### WBP_GridSkills

Ajouter entre Attribut et Rang :

~~~text
Button_SortAffinity
├── Text_HeaderAffinity       -> ST_UI / Skill.Column.Affinity
└── Text_SortAffinityIndicator
~~~

### WBP_RPGSkillEntry

~~~text
Overlay_SkillAffinity
├── Border_AffinityRecommended   -> fond vert fort
├── Border_AffinityUseful        -> fond vert plus léger
├── Border_AffinityNeutral       -> fond neutre / discret
├── Border_AffinityDiscouraged   -> fond rouge
└── Text_SkillAffinity
~~~

C++ ne choisit que l’état visible et le glyphe. Les couleurs, bordures,
marges et tailles restent sous autorité UMG.

## 5. Vocabulaire

~~~text
Skill.Column.Affinity = Affinité
Skill.Affinity.DiscouragedClasses = Classes déconseillées :
~~~

## 6. Validation 01.1

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Nouveaux tests : Resolution, OverlapGuard, Sort et WidgetContract sous
`Grimrock.UI.Skills02.Affinity`.

Après validation source : matérialiser ST_UI, authorer les `DiscouragedClassIds`
pertinents dans `DA_RPGSkillPresentation`, puis construire les deux colonnes UMG.
