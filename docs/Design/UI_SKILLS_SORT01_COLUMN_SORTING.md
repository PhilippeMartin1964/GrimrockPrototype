# UI-SKILLS-SORT01 — Skill column sorting

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ / CLOS**

## 1. Objectif

Rendre les trois colonnes principales de la liste Skills interactives :

~~~text
COMPÉTENCE
ATTRIBUT
RANG
~~~

Un clic trie la colonne en ordre croissant. Un second clic sur la même colonne
inverse l'ordre. Cliquer une autre colonne active celle-ci en ordre croissant.

## 2. Contrat

Le tri est strictement une projection UI :

~~~text
FGridSkillsPageView::Skills
    -> copie locale
    -> SortSkillEntries()
    -> création des WBP_RPGSkillEntry
~~~

La projection canonique, les rangs, les Skill assets et les SaveGames ne sont
jamais réordonnés ni modifiés.

État session uniquement :

~~~text
SkillSortColumn
SkillSortDirection
~~~

Valeur initiale :

~~~text
Name + Ascending
~~~

Le comportement visuel actuel A -> Z est donc conservé par défaut.

## 3. Colonnes

### Compétence

Tri sur le nom joueur affiché, sans sensibilité à la casse.

### Attribut

Tri sur le **libellé localisé affiché**, et non sur la valeur numérique de
`ERPGSkillGoverningAttribute`.

### Rang

Tri numérique sur `FGridSkillEntryView::Rank`.

Pour les égalités, le nom joueur puis `SkillId` fournissent un ordre
déterministe.

## 4. Vocabulaire

Nouvelles clés `UI.csv` :

~~~text
Skill.Column.Name       = Compétence
Skill.Column.Attribute  = Attribut
Skill.Column.Rank       = Rang
~~~

Ces trois textes sont strictement statiques. Après matérialisation de `ST_UI`,
les TextBlocks d'en-tête doivent utiliser **Referenced Text -> ST_UI**
directement. Aucun setter C++ n'est prévu pour ces libellés.

Les glyphes ▲ / ▼ sont mécaniques et dynamiques ; ils restent pilotés en C++.

## 5. WBP_GridSkills à matérialiser

Ajouter, au-dessus de la liste scrollable :

~~~text
HB_SkillColumnHeaders
├── Button_SortName
│   └── HB_Name
│       ├── Text_HeaderName
│       └── Text_SortNameIndicator
├── Button_SortAttribute
│   └── HB_Attribute
│       ├── Text_HeaderAttribute
│       └── Text_SortAttributeIndicator
├── Button_SortRank
│   └── HB_Rank
│       ├── Text_HeaderRank
│       └── Text_SortRankIndicator
└── Spacer_Actions
~~~

Références directes :

| TextBlock | ST_UI |
| --- | --- |
| Text_HeaderName | Skill.Column.Name |
| Text_HeaderAttribute | Skill.Column.Attribute |
| Text_HeaderRank | Skill.Column.Rank |

Les six hooks natifs sont désormais `BindWidget` obligatoires : la ligne d'en-têtes
est matérialisée dans `WBP_GridSkills` et validée visuellement en PIE.

## 6. Tests source

~~~text
Grimrock.UI.Skills02.Sort.Name
Grimrock.UI.Skills02.Sort.Attribute
Grimrock.UI.Skills02.Sort.Rank
Grimrock.UI.Skills02.Sort.WidgetContract
~~~

## 7. Validation finale — 16 octobre 2026

Validation locale après matérialisation WBP et resserrement des BindWidget :

~~~text
Grimrock.UI.Skills02
    19 succeeded
    0 warnings
    0 failed

Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression
    1 succeeded
    0 warnings
    0 failed

UI-LOCALIZATION01.6 smoke
    Runtime_Culture_Smoke                  : OK
    source-controlled configs unchanged    : OK
    volatile generated outputs restored    : OK
    final git status --short               : clean
~~~

Rapports :

~~~text
Saved/Automation/TD04/TD04-20261016-162401
Saved/Automation/TD04/TD04-20261016-162453
Saved/Localization/UI-LOCALIZATION01.6/20261016-162509
~~~

La matérialisation `ST_UI` est versionnée dans `4df462a0`. La matérialisation
WBP a été commitée avec les ajustements visuels Skills dans `d283efef`. Le
contrat natif obligatoire est versionné dans `1736ce0d`.

## 8. Résultat final

~~~text
Compétence : A -> Z / Z -> A
Attribut   : A -> Z / Z -> A
Rang       : 0 -> N / N -> 0
~~~

Le tri reste strictement présentation-only, les égalités retombent sur le nom,
les libellés d'en-tête sont des références directes `ST_UI`, les glyphes ▲ / ▼
restent mécaniques et dynamiques, et le working tree reste propre après le smoke.

## 9. Commits

~~~text
75a33546 UI-SKILLS-SORT01.1 add Skill column sorting source
68639c40 UI-SKILLS-SORT01.1A stabilize Skill sort tie order
4df462a0 UI-SKILLS-SORT01.2 materialize Skill sort column vocabulary
d283efef UI-SKILLS02 polish Skill affinity rows
1736ce0d UI-SKILLS-SORT01.3 require materialized Skill sort controls
~~~

UI-SKILLS-SORT01 est **clos**.
