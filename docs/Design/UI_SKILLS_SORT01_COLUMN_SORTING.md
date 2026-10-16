# UI-SKILLS-SORT01 — Skill column sorting

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.1 SOURCE IMPLÉMENTÉE — matérialisation WBP manuelle requise**

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

Les six hooks natifs sont temporairement `BindWidgetOptional` jusqu'à cette
matérialisation.

## 6. Tests source

~~~text
Grimrock.UI.Skills02.Sort.Name
Grimrock.UI.Skills02.Sort.Attribute
Grimrock.UI.Skills02.Sort.Rank
Grimrock.UI.Skills02.Sort.WidgetContract
~~~

## 7. Validation

Après récupération du commit source :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Ensuite seulement : matérialiser les trois clés dans `ST_UI`, construire la
ligne d'en-têtes dans `WBP_GridSkills`, rendre les six hooks obligatoires,
rejouer Skills02 et le smoke, puis fermer UI-SKILLS-SORT01.
