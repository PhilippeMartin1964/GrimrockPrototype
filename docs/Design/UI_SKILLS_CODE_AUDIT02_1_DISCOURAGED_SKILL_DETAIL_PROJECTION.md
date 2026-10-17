# UI-SKILLS-CODE-AUDIT02.1 — Discouraged Skill Detail Projection

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ / CLOS**

## 1. Défaut corrigé

`FGridSkillDetailView::DiscouragedClasses` était correctement alimenté par
`FGridSkillsPageService`, mais `UGridSkillDetailWidget` ne projetait ni ne
rendait cette collection.

La chaîne était interrompue avant le détail UMG :

~~~text
DA_RPGSkillPresentation.DiscouragedClassIds
    -> FGridSkillsPageService
    -> FGridSkillDetailView::DiscouragedClasses
    -> [perdu avant WBP_RPGSkillDetail]
~~~

Aucune modification de la matrice d'affinité ou de son autorité n'était
nécessaire.

## 2. UI-SKILLS-CODE-AUDIT02.1A — projection source

`UGridSkillDetailWidget` expose désormais :

~~~text
ResolvedDiscouragedClassesText
VB_DetailDiscouragedClasses
Text_DetailDiscouragedClasses
~~~

La projection suit le même chemin que Recommended / Useful :

~~~text
DiscouragedClasses
    -> JoinClassNames(...)
    -> ResolvedDiscouragedClassesText
    -> Text_DetailDiscouragedClasses
~~~

`VB_DetailAffinities` reste visible lorsqu'au moins l'un des trois groupes
Recommended / Useful / Discouraged contient une valeur.

02.1A utilisait temporairement `BindWidgetOptional` pour les deux nouveaux
hooks, uniquement afin de laisser la matérialisation Designer se faire sans
rendre le WBP existant invalide.

Validation locale reçue :

~~~text
Grimrock.UI.Skills02
  25/25 PASS
  0 warning
  0 échec
  rapport : Saved/Automation/TD04/TD04-20261017-144051

Game.UI.Localization.LOCALIZATION02.FullGameGatherContract
  1/1 PASS
  0 warning
  0 échec
  rapport : Saved/Automation/TD04/TD04-20261017-144112
~~~

## 3. UI-SKILLS-CODE-AUDIT02.1B — matérialisation UMG

`WBP_RPGSkillDetail` a été matérialisé manuellement dans Unreal Editor et
versionné sans modification automatique du `.uasset`.

Structure ajoutée sous la section d'affinités :

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
    -> valeur dynamique C++
~~~

Le rendu visuel a été vérifié dans le runtime avec une Skill affichant :

~~~text
Classes recommandées : Voleur, Rôdeur
Classes utiles : Alchimiste
Classes déconseillées : Guerrier, Prêtre
~~~

Commit de matérialisation :

~~~text
d77d06dd UI-SKILLS-CODE-AUDIT02.1B materialize discouraged Skill detail row
~~~

## 4. UI-SKILLS-CODE-AUDIT02.1C — contrat durci

Après matérialisation, les deux hooks sont désormais obligatoires :

~~~text
VB_DetailDiscouragedClasses   -> BindWidget
Text_DetailDiscouragedClasses -> BindWidget
~~~

`Grimrock.UI.Skills02.DetailWidget.WidgetContract` les traite comme les autres
bindings structurels du panneau détail et vérifie qu'ils ne sont plus
`BindWidgetOptional`.

Le C++ ne possède toujours ni style, ni couleur, ni marges, ni texte statique du
label. Ces responsabilités restent dans UMG / ST_UI.

## 5. Tests couverts

`Grimrock.UI.Skills02.Pipeline.DetailProjection` vérifie la résolution de
`DiscouragedClassIds` vers `FGridSkillClassAffinityView`.

`Grimrock.UI.Skills02.DetailWidget.Formatting` vérifie la projection texte
player-facing.

`Grimrock.UI.Skills02.DetailWidget.RejectAndClear` vérifie le nettoyage du
texte résolu.

`Grimrock.UI.Skills02.DetailWidget.WidgetContract` verrouille maintenant les
deux bindings matérialisés comme obligatoires.

Le contrat de localisation exige explicitement la clé existante :

~~~text
Skill.Affinity.DiscouragedClasses
~~~

## 6. Non-objectifs

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

## 7. Validation finale 02.1C

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.Localization.LOCALIZATION02.FullGameGatherContract"
~~~

Validation finale confirmée. Le contrat `LOCALIZATION02.FullGameGatherContract`
est l'autorité correcte pour la clé `Skill.Affinity.DiscouragedClasses`.

État final : **UI-SKILLS-CODE-AUDIT02.1 VALIDÉ / CLOS**.
