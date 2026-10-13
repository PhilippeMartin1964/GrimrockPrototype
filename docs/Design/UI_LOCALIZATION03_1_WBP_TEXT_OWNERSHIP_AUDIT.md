# UI-LOCALIZATION03.1 — Audit serialized player-facing WBP text ownership

Date : **13 octobre 2026**  
Projet : **Unreal Engine 5.5.4 dungeon crawler prototype**  
Ticket : **UI-LOCALIZATION03.1**  
État : **AUDIT UNIQUEMENT — aucun WBP, Data Asset, C++ runtime ou catalogue de localisation modifié**  
Baseline auditée : **8304e12be63d7aa16b9dc915ab69fdecb520df31** (`UI-LOCALIZATION02.DOC`)

## 1. Objet

Inventorier les textes joueur encore sérialisés sous :

~~~text
Content/GrimrockPrototype/Blueprints/UI/*
~~~

et déterminer leur **autorité cible** sans modifier les assets.

Ce ticket ne migre rien. Il ne doit pas :

- éditer un `.uasset` ;
- ajouter de nouvelles clés à `UI.csv` ;
- modifier le runtime C++ ;
- modifier `DA_RPGTalentDetailTextSet` ;
- déduire qu'un texte doit aller dans `ST_UI` uniquement parce qu'il est
  player-facing.

Le principe conservé est :

~~~text
ST_UI
    = vocabulaire UI générique partagé

Data Assets
    = contenu RPG / gameplay / narratif authoré

C++ / read models
    = valeurs dynamiques, composition, mécanique

WBP
    = présentation et copy réellement propre à l'écran
~~~

## 2. Méthode

La source principale de l'audit est le `Game.manifest` matérialisé et validé
à la fermeture de UI-LOCALIZATION02.

Ce choix permet d'inspecter les `FText` réellement collectés dans les WBP
sans tenter de lire ou de modifier les `.uasset` binaires à l'aveugle.

Les occurrences sous `/Game/GrimrockPrototype/Blueprints/UI/` ont ensuite
été recoupées avec :

- `Localization/Source/UI.csv` ;
- les widgets C++ qui réécrivent les champs dynamiques ;
- l'autorité existante `DA_RPGTalentDetailTextSet` ;
- les règles d'ownership fermées par UI-LOCALIZATION01/02.

## 3. Mesure globale

Le manifest contient :

~~~text
716 entrées collectées au total

221 occurrences provenant de Blueprints/UI
175 textes source distincts
29 packages WBP concernés
~~~

Répartition structurelle :

~~~text
203 occurrences WidgetTree
  6 occurrences Script Bytecode
 12 occurrences Default__ / données sérialisées du WBP
---
221
~~~

Répartition d'ownership retenue par cet audit :

| Classe | Occurrences | Décision |
| --- | ---: | --- |
| Valeur / placeholder runtime sérialisé | **62** | vider la valeur Designer lorsque le runtime possède déjà l'autorité |
| Texte actif dans Blueprint Script Bytecode | **6** | sortir le wording/logique du Blueprint dans un ticket borné |
| Doublon exact d'une clé `ST_UI` existante | **14** | réutiliser l'autorité existante |
| Vocabulaire générique Talent dans WBP | **13** | utiliser `DA_RPGTalentDetailTextSet`, pas une nouvelle autorité |
| Contenu authoré dans les defaults du WBP | **12** | déplacer vers l'autorité de contenu existante appropriée |
| Glyphes / notation mécaniques | **12** | conserver hors migration linguistique |
| Autres textes statiques de présentation | **102** | rester WBP par défaut ; migrer seulement si partage réel démontré |
| **Total** | **221** | |

Cette classification est volontairement conservatrice : elle cherche à
supprimer les **doubles autorités démontrées**, pas à vider tous les WBP de
leurs textes.

## 4. Valeurs runtime sérialisées — 62 occurrences

Ces textes ne sont pas des autorités de wording. Ils servent de valeurs
Designer d'exemple alors que le runtime les remplace.

Les familles confirmées sont :

~~~text
WBP_GridCombatHud
    Text_EndTurnDisabledReason
    Text_MobilityActionPoints

WBP_CharacterSheet
    Text_Character*
    Text_AttributePoints
    TextBlock                  (" - ")

WBP_LoadGameSlotRow
    Text_Status
    Text_DisplayName
    Text_SlotName

WBP_CCStep_Attributes
    Text_*ClassValue
    Text_*RaceBonus
    Text_*TotalValue
    Text_*Modifier
    Text_AttributePointsRemaining

WBP_CharacterCreationWizard
    Text_IdentityClass
    Text_IdentityRace
    Text_IdentityGender

WBP_RPGSkillEntry
    Text_SkillName
    Text_SkillAttribute
    Text_SkillRank
    Text_SkillTrainingPolicy
    Text_SkillDescription

WBP_RPGTalentBranch
    Text_BranchName
    Text_BranchProgress

WBP_RPGTalentDetail
    Text_DetailName

WBP_RPGTalentNode
    Text_TalentTier
    Text_TalentLevel
    Text_TalentState

WBP_GridSkills
    Text_CharacterName
    Text_ClassLevel
    Text_SkillPoints
    Text_TalentPoints

WBP_ItemTooltipStatLine
    Text_StatDelta             ("(Δ +1)")
~~~

Exemples collectés qui ne doivent pas devenir du contenu de traduction :

~~~text
0
10
-
PAM 2 / 2
Rang 0 / 5
Classe — Niveau 1
Points de compétence : 4 — Rang max : 2
Points de talent : 0
Sauvegarde TEST
Slot TEST
(Δ +1)
~~~

Le code de production confirme déjà plusieurs de ces autorités runtime :

- `UGridCombatHudWidget` projette `View.Mobility.DisplayText` et
  `View.EndTurnDisabledReason` ;
- `URPGCharacterCreationAttributesStepWidget` recalcule les valeurs
  d'attributs et les points restants ;
- `UGridSkillEntryWidget` vide puis projette les champs Skill ;
- `UGridTalentBranchWidget` et `UGridTalentNodeWidget` vident leurs champs
  avant initialisation ;
- `UGridSkillsWidget` projette les données de personnage et progression.

### Décision 03.1

**Ne pas créer de clés de localisation pour ces valeurs Designer.**

Ticket de migration recommandé : vider uniquement les valeurs sérialisées
confirmées comme runtime-controlled, sans toucher aux bindings ni à la logique.

## 5. Blueprint Script Bytecode — 6 occurrences

Le Gather expose six textes player-facing présents dans du graphe Blueprint :

| WBP | Texte collecté | Décision cible |
| --- | --- | --- |
| `WBP_ItemTooltip` | `({Qty}x)` | format mécanique/read-model C++ ; pas de nouvelle prose WBP |
| `WBP_ItemTooltip` | `Lumière : allumée` | wording UI à sortir du graphe ; `ST_UI` candidat |
| `WBP_ItemTooltip` | `Lumière : éteinte` | wording UI à sortir du graphe ; `ST_UI` candidat |
| `WBP_ItemTooltipStatLine` | `(Δ {Delta})` | format mécanique/read-model C++ |
| `WBP_ItemTooltipComparisonRow` | `vide` | vocabulaire UI générique candidat `ST_UI` |
| `WBP_LoadGameMenu` | `Impossible de charger cette sauvegarde.` | feedback UI candidat `ST_UI` |

Ces six entrées sont plus importantes qu'un simple label statique : elles
montrent du wording joueur dans la logique Blueprint.

### Décision 03.1

Les sortir du Blueprint dans un ticket distinct. Ne pas modifier les graphes
UMG dans cet audit.

## 6. Doublons exacts avec ST_UI — 14 occurrences statiques

Ces occurrences ont déjà une autorité explicite dans `UI.csv`.

| WBP / champ | Texte sérialisé | Clé ST_UI existante |
| --- | --- | --- |
| CharacterSheet / `Label_Arcane` | Arcane | `Spellbook.School.Arcane` |
| CharacterSheet / `Label_Fire` | Feu | `Spellbook.School.Fire` |
| CharacterSheet / `Label_Ice` | Glace | `Spellbook.School.Frost` |
| CCStep Attributes / Charisma | Charisme | `CharacterCreation.Attribute.Charisma` |
| CCStep Attributes / Constitution | Constitution | `CharacterCreation.Attribute.Constitution` |
| CCStep Attributes / Dexterity | Dextérité | `CharacterCreation.Attribute.Dexterity` |
| CCStep Attributes / Strength | Force | `CharacterCreation.Attribute.Strength` |
| CCStep Attributes / Intelligence | Intelligence | `CharacterCreation.Attribute.Intelligence` |
| CCStep Attributes / Wisdom | Sagesse | `CharacterCreation.Attribute.Wisdom` |
| CCStep Attributes / Class header | Classe | `CharacterCreation.Wizard.Step.Class` |
| CCStep Attributes / help | Les valeurs de classe peuvent être ajustées… | `CharacterCreation.Attribute.Help` |
| Character Creation Wizard / current choices | Choix actuels | `CharacterCreation.Identity.CurrentChoicesTitle` |
| Character Creation Wizard / portrait caption | Portrait final | `CharacterCreation.Portrait.FinalCaption` |
| Character Creation Wizard / summary title | Résumé | `CharacterCreation.Wizard.Step.Summary` |

Deux autres sources sérialisées ont aussi un texte identique à `ST_UI`, mais
sont **runtime-controlled** et appartiennent donc au nettoyage des placeholders,
pas à un binding statique vers la String Table :

~~~text
WBP_GridCombatHud
    "Déplacement en cours"
    -> Combat.EndTurn.PartyMotion

WBP_LoadGameSlotRow
    "Disponible"
    -> SaveLoad.Slot.Available
~~~

### Décision 03.1

Réutiliser l'autorité déjà existante au lieu de conserver deux identités
traduisibles pour la même surface.

## 7. Vocabulaire Talent — 13 occurrences statiques

Les widgets suivants contiennent encore des labels génériques Talent sérialisés :

~~~text
WBP_RPGTalentDetail
    ACQUISITION
    ACQUÉRIR
    ANNULER
    CONFIRMER
    EFFETS
    PRINCIPE
    STATUT
    TYPE
    UTILISATION
    VARIANTES

WBP_RPGTalentVariantBlock
    CHOISIR
    EFFETS
    UTILISATION
~~~

Le projet possède déjà une autorité explicite :

~~~text
DA_RPGTalentDetailTextSet
    -> vocabulaire UI générique Talent
~~~

`CHOISIR` est même déjà présent comme source dans ce TextSet.

### Décision 03.1

**Ne pas ajouter ces textes à ST_UI par réflexe.**

Le futur ticket doit d'abord réutiliser `DA_RPGTalentDetailTextSet` ou
confirmer qu'un label particulier est intentionnellement purement décoratif
dans le WBP.

Cette décision évite une troisième autorité :

~~~text
WBP
vs ST_UI
vs DA_RPGTalentDetailTextSet
~~~

## 8. Contenu authoré dans CharacterCreationWizard — 12 occurrences

Le CDO du WBP contient les `DisplayName` des 12 illustrations race/genre :

~~~text
Humain mâle.
Humain femelle.
Nain mâle.
Nain femelle.
Elfe mâle.
Elf femelle.
Gnome mâle.
Gnome femelle.
Halfelin mâle.
Halfelin femelle.
Demi-orque mâle.
Demi-orque femelle.
~~~

Ces textes sont du **contenu de personnage/portrait**, pas du vocabulaire
d'interface générique.

### Décision 03.1

Ils ne doivent pas migrer vers `ST_UI`.

Un futur ticket doit les rattacher à l'autorité de contenu existante
race/portrait la plus proche. Aucune nouvelle couche Data Asset ne doit être
créée avant d'avoir confirmé que les assets de race/portrait actuels ne peuvent
pas porter cette information.

L'audit note aussi l'incohérence orthographique existante :

~~~text
"Elfe mâle."
"Elf femelle."
~~~

Elle n'est pas corrigée dans 03.1.

## 9. Glyphes et notation mécanique — 12 occurrences

Les occurrences suivantes ne constituent pas du wording linguistique :

~~~text
WBP_GridPersistentHud
    ESC
    G
    H
    I
    J
    K
    M

WBP_GridMap
    ↑
    ↓

WBP_RPGSkillEntry
    +
    -

WBP_GridSkills
    - / +
~~~

### Décision 03.1

Conserver hors migration de localisation.

Une éventuelle future gestion dynamique des raccourcis clavier est un sujet
Input/UI distinct, pas une dette de traduction.

## 10. Textes statiques WBP restants — 102 occurrences

Après retrait des doubles autorités démontrées ci-dessus, 102 occurrences
statiques restent des textes de présentation WBP.

L'audit **ne recommande pas de les migrer en bloc**.

Des écrans comme les suivants peuvent légitimement posséder du copy local :

~~~text
WBP_CreditsMenu
WBP_LicenseMenu
WBP_OptionsMenu
WBP_GridCodex
WBP_GridJournal
WBP_GridRecipes
WBP_GridSpellbook
~~~

Exemples qui peuvent rester WBP tant qu'ils sont propres à cet écran :

~~~text
Prototype développé sous Unreal Engine 5.5.4.
Prototype privé de développement...
Options audio/video à venir.
Aucun sort connu
Livre de sorts
Aucune compétence disponible.
Sélectionnez une sauvegarde à charger.
~~~

Le fait qu'un texte soit localisable ne signifie pas que son autorité doit
être centralisée.

## 11. Nouveaux candidats ST_UI réellement partagés

Quelques chaînes restantes sont partagées entre plusieurs surfaces et méritent
un ticket de convergence ciblé :

| Texte | Occurrences | Surfaces | Décision |
| --- | ---: | --- | --- |
| Retour | 4 | Credits, Load, License, Options | forte candidate `ST_UI` |
| Charger partie | 2 | MainMenu + LoadGameMenu | candidate `ST_UI` |
| Options | 2 | MainMenu + OptionsMenu | candidate `ST_UI` |
| Licence | 2 | MainMenu + LicenseMenu | candidate `ST_UI` |
| Equipement | 2 | CharacterSheet + InventoryBag | candidate `ST_UI` |
| Mana | 2 | CharacterSheet + CC Attributes | candidate `ST_UI` |

Ne pas dédupliquer uniquement par égalité de chaîne.

Contre-exemples :

~~~text
"Niveau"
    Map
    Talent Node
    -> même français, sémantique différente

"STATUT"
    GridSkills
    Talent Detail
    -> le Talent Detail appartient au Talent TextSet
~~~

L'ownership sémantique prime sur le texte source identique.

## 12. Matrice des 29 packages WBP concernés

| Package | Occurrences | Composition observée | Décision principale |
| --- | ---: | --- | --- |
| Combat/WBP_GridCombatActionPanel | 2 | 2 static | revoir vocabulaire stat PV/Mana |
| Combat/WBP_GridCombatHud | 3 | 2 runtime + 1 static | vider runtime ; garder/migrer action Fin du tour séparément |
| InGameMenu/WBP_GridCodex | 1 | 1 static | WBP acceptable |
| InGameMenu/WBP_GridJournal | 1 | 1 static | WBP acceptable |
| InGameMenu/WBP_GridMap | 5 | 3 static + 2 mécaniques | conserver glyphes ; copy écran à revoir seulement si partagé |
| InGameMenu/WBP_GridRecipes | 1 | 1 static | WBP acceptable |
| InGameMenu/WBP_GridSpellbook | 2 | 2 static | WBP acceptable |
| Inventory/WBP_CharacterSheet | 37 | 13 runtime + 24 static | priorité élevée : vider runtime + réutiliser ST_UI existant |
| Inventory/WBP_GridPersistentHud | 7 | 7 mécaniques | aucune migration localisation |
| Inventory/WBP_InventoryBag | 8 | 8 static | revoir filtres partagés ; ne pas migrer en bloc |
| Inventory/WBP_ItemReadPanel | 1 | 1 static | `Fermer` candidat vocabulaire commun |
| Inventory/WBP_ItemTooltip | 6 | 3 bytecode + 3 static | priorité élevée : sortir le wording du graphe |
| Inventory/WBP_ItemTooltipComparisonRow | 2 | 1 static + 1 bytecode | sortir `vide` du graphe |
| Inventory/WBP_ItemTooltipStatLine | 2 | 1 runtime + 1 bytecode | format mécanique C++ |
| MainMenu/WBP_CreditsMenu | 4 | 4 static | contenu local WBP ; `Retour` partagé |
| MainMenu/WBP_LicenseMenu | 4 | 4 static | contenu local WBP ; `Retour` partagé |
| MainMenu/WBP_LoadGameMenu | 5 | 4 static + 1 bytecode | sortir erreur du graphe ; factoriser vocabulaire partagé |
| MainMenu/WBP_LoadGameSlotRow | 3 | 3 runtime | vider les trois valeurs Designer |
| MainMenu/WBP_MainMenu | 9 | 9 static | factoriser actions réellement partagées |
| MainMenu/WBP_OptionsMenu | 4 | 4 static | contenu local WBP ; `Retour` / `Options` partagés |
| RPG/WBP_CCStep_Attributes | 43 | 25 runtime + 18 static | priorité élevée : vider runtime + réutiliser ST_UI |
| RPG/WBP_CharacterCreationWizard | 29 | 3 runtime + 14 static + 12 contenu defaults | priorité élevée : ST_UI existant + externaliser contenu portrait |
| RPG/WBP_RPGStoryCompanionRecruitment | 2 | 2 static | WBP acceptable tant que non partagé |
| Skills/RPG/WBP_RPGSkillEntry | 7 | 5 runtime + 2 mécaniques | vider runtime ; conserver +/- |
| Skills/RPG/WBP_RPGTalentBranch | 2 | 2 runtime | vider les placeholders |
| Skills/RPG/WBP_RPGTalentDetail | 11 | 1 runtime + 10 static Talent | vider runtime + réutiliser Talent TextSet |
| Skills/RPG/WBP_RPGTalentNode | 3 | 3 runtime | vider placeholders |
| Skills/RPG/WBP_RPGTalentVariantBlock | 3 | 3 static Talent | réutiliser Talent TextSet |
| Skills/WBP_GridSkills | 14 | 4 runtime + 9 static + 1 mécanique | vider runtime ; revue ciblée du vocabulaire |

Les nombres de cette matrice totalisent exactement les **221 occurrences**
inventoriées.

## 13. Ordre de migration recommandé

### UI-LOCALIZATION03.2A — Runtime WBP placeholder cleanup

Objet :

- vider les **62 valeurs runtime** confirmées ;
- ne modifier ni bindings ni logique ;
- travailler via Unreal Editor / Python Editor avec liste explicite de champs ;
- regather et vérifier que ces valeurs disparaissent du manifest.

C'est le meilleur premier ticket : il réduit fortement le bruit de traduction
sans déplacer d'ownership linguistique.

### UI-LOCALIZATION03.2B — Blueprint bytecode text boundary

Objet :

- traiter les **6 textes Script Bytecode** ;
- déplacer les formats mécaniques vers le read-model C++ ;
- utiliser une autorité localisable existante ou `ST_UI` pour le wording UI ;
- supprimer les textes joueur construits dans les graphes concernés.

### UI-LOCALIZATION03.2C — Reuse existing text authorities

Objet :

- remplacer les **14 doublons statiques ST_UI** par leur autorité existante ;
- faire converger les **13 labels Talent** vers
  `DA_RPGTalentDetailTextSet` ;
- ne créer aucune nouvelle famille d'autorité.

### UI-LOCALIZATION03.2D — Shared WBP vocabulary

Objet :

- traiter seulement les chaînes réellement partagées comme `Retour`,
  `Charger partie`, `Options`, `Licence`, `Equipement`, `Mana` ;
- ajouter des clés `ST_UI` uniquement lorsqu'un partage sémantique est
  démontré ;
- laisser les copy d'écran spécifiques dans leurs WBP.

### UI-LOCALIZATION03.2E — Character illustration authored text

Objet :

- sortir les 12 `AvailableRaceIllustrations.DisplayName` du CDO du WBP ;
- réutiliser l'autorité de contenu race/portrait existante si possible ;
- corriger à cette occasion les incohérences de wording, après décision
  d'authoring.

## 14. Garde-fous futurs

Après les migrations, un ticket de garde-fou pourra vérifier au minimum :

- absence des placeholders runtime connus dans le manifest WBP ;
- absence de player-facing `[Script Bytecode]` pour les six surfaces
  identifiées ;
- absence des doublons ST_UI explicitement migrés ;
- maintien des glyphes mécaniques autorisés ;
- maintien des copy WBP intentionnellement locales.

Le garde-fou ne doit pas imposer :

~~~text
"zéro FText dans les WBP"
~~~

car ce n'est pas l'architecture retenue.

## 15. Conclusion

UI-LOCALIZATION03.1 établit qu'il n'existe pas un problème unique
« les WBP contiennent encore du texte », mais cinq cas différents :

~~~text
1. placeholders runtime à vider
2. wording actif dans des graphes Blueprint à sortir
3. doubles autorités déjà démontrées à supprimer
4. contenu authoré placé au mauvais endroit
5. copy de présentation qui peut légitimement rester dans le WBP
~~~

La prochaine étape recommandée est :

~~~text
UI-LOCALIZATION03.2A — Runtime WBP placeholder cleanup
~~~

Elle doit rester strictement bornée aux **62 occurrences runtime confirmées**.
