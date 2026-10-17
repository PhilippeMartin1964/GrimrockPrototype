# UI-SKILLS-CODE-AUDIT02.1 — Discouraged Skill Detail Projection

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **02.1A SOURCE PROJECTION IMPLÉMENTÉE — matérialisation UMG requise avant clôture**

## 1. Défaut corrigé

`FGridSkillDetailView::DiscouragedClasses` était correctement alimenté par
`FGridSkillsPageService`, mais `UGridSkillDetailWidget` ne projetait ni ne
rendait cette collection.

La chaîne s'arrêtait donc ici :

~~~text
DA_RPGSkillPresentation.DiscouragedClassIds
    -> FGridSkillsPageService
    -> FGridSkillDetailView::DiscouragedClasses
    -> [perdu avant WBP_RPGSkillDetail]
~~~

Aucune modification de la matrice d'affinité ou de son autorité n'est requise.

## 2. Correction native 02.1A

`UGridSkillDetailWidget` expose désormais :

~~~text
ResolvedDiscouragedClassesText
VB_DetailDiscouragedClasses
Text_DetailDiscouragedClasses
~~~

Le texte est résolu avec le même chemin que Recommended / Useful :

~~~text
DiscouragedClasses
    -> JoinClassNames(...)
    -> ResolvedDiscouragedClassesText
    -> Text_DetailDiscouragedClasses
~~~

`VB_DetailAffinities` reste visible lorsqu'au moins l'un des trois groupes
Recommended / Useful / Discouraged contient une valeur.

Les deux nouveaux hooks UMG restent temporairement `BindWidgetOptional` dans
02.1A. Cette optionalité est uniquement une frontière de matérialisation : elle
ne doit pas devenir le contrat final.

## 3. Tests renforcés

`Grimrock.UI.Skills02.Pipeline.DetailProjection` vérifie maintenant que
`DiscouragedClassIds` est résolu par le service vers un
`FGridSkillClassAffinityView` correct.

`Grimrock.UI.Skills02.DetailWidget.Formatting` vérifie que le widget produit
le texte player-facing correspondant.

`Grimrock.UI.Skills02.DetailWidget.WidgetContract` exige les deux hooks
transitoires et vérifie qu'ils sont optionnels uniquement avant matérialisation.

Le contrat de localisation exige explicitement la clé déjà existante :

~~~text
Skill.Affinity.DiscouragedClasses
~~~

Aucune nouvelle clé ST_UI n'est créée.

## 4. Matérialisation manuelle requise — 02.1B

Ne pas modifier le `.uasset` par script.

Dans `WBP_RPGSkillDetail`, sous la section d'affinités, ajouter sur le même
modèle que Useful :

~~~text
VB_DetailDiscouragedClasses
├── Text_LabelDiscouragedClasses
└── Text_DetailDiscouragedClasses
~~~

Contrat :

~~~text
Text_LabelDiscouragedClasses
    -> texte statique direct ST_UI
    -> Skill.Affinity.DiscouragedClasses

Text_DetailDiscouragedClasses
    -> Is Variable = YES
    -> valeur dynamique C++
~~~

Le style, les marges, la police et la couleur restent propriété du Designer.

Après matérialisation et validation, 02.1B doit transformer :

~~~text
VB_DetailDiscouragedClasses   BindWidgetOptional -> BindWidget
Text_DetailDiscouragedClasses BindWidgetOptional -> BindWidget
~~~

et le test de contrat doit exiger leur caractère obligatoire.

## 5. Non-objectifs

02.1 ne modifie pas :

~~~text
RecommendedClassIds
UsefulClassIds
DiscouragedClassIds
ResolveClassAffinity(...)
SkillAffinityAudit
SkillAffinityAuthoring
tri Affinity
économie / allocation / undo Skill
~~~

## 6. Validation source 02.1A

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis régression localisation ciblée :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.Localization.LOCALIZATION01.LocalizationTargetContract"
~~~

Ne pas déclarer 02.1 clos avant le résultat local et la matérialisation UMG
manuelle.
