# UI-SKILLS-AFFINITY01 — Class affinity column

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.2D CONTRAT UMG RESSERRÉ — validation finale requise**

## 1. Objectif

Ajouter à la liste des compétences une colonne compacte qui indique
l’affinité de chaque compétence avec la classe du personnage sélectionné.

La sémantique reste à quatre états, mais **aucun caractère Unicode n’est rendu
par C++**. Les symboles étoile sont des textures/images Designer afin d’éviter
tout fallback de police ou rendu emoji.

~~~text
Recommended -> 2 × image étoile pleine
Useful      -> 1 × image étoile pleine
Neutral     -> 1 × image étoile contour
Discouraged -> 1 × image étoile barrée
~~~

La couleur de fond reste propriété du Designer : vert pour Recommended/Useful,
neutre pour Neutral et rouge pour Discouraged.

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
├── Border_AffinityRecommended   -> fond vert fort + 2 images étoile pleine
├── Border_AffinityUseful        -> fond vert léger + 1 image étoile pleine
├── Border_AffinityNeutral       -> fond neutre + 1 image étoile contour
└── Border_AffinityDiscouraged   -> fond rouge + 1 image étoile barrée
~~~

C++ choisit uniquement le `Border_Affinity*` visible. Les textures, couleurs,
bordures, marges et tailles restent sous autorité UMG.

`Text_SkillAffinity` et `ResolveAffinityGlyph()` ont été supprimés du contrat
natif en 01.2C. Un éventuel `Text_SkillAffinity` résiduel dans le WBP peut être
supprimé du Designer : il n’a plus aucune fonction runtime.

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


## 7. UI-SKILLS-AFFINITY01.2C — retrait du rendu Unicode

Le chemin texte mécanique a été supprimé :

~~~text
Text_SkillAffinity        -> supprimé du C++
ResolveAffinityGlyph()    -> supprimé
Unicode étoile            -> plus aucun rendu runtime C++
~~~

Les quatre `Border_Affinity*` restent les seuls hooks natifs de la cellule
d’affinité. Après matérialisation/versioning de `WBP_RPGSkillEntry`, ils sont
désormais des `BindWidget` obligatoires. `Button_SortAffinity` et
`Text_SortAffinityIndicator` sont également obligatoires dans `WBP_GridSkills`.

Validation attendue :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~


## 8. UI-SKILLS-AFFINITY01.2D — contrat matérialisé

Les six hooks natifs d’affinité sont maintenant obligatoires :

~~~text
WBP_GridSkills
  Button_SortAffinity
  Text_SortAffinityIndicator

WBP_RPGSkillEntry
  Border_AffinityRecommended
  Border_AffinityUseful
  Border_AffinityNeutral
  Border_AffinityDiscouraged
~~~

Les images internes restent volontairement non liées au C++ : textures, nombre
d’étoiles, couleurs et mise en page restent sous autorité UMG.
