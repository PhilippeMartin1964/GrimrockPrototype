# UI-PARTYHUD01 — Rail persistant des portraits du groupe

Date : **16 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Contrat : **ADOPTÉ**  
État courant : **UI-PARTYHUD01.1/.2/.3/.4/.5 VALIDÉS / CLOS — UI-PARTYHUD01.6 IMPLÉMENTÉ / VALIDATION REQUISE**

## 1. Objectif général

UI-PARTYHUD01 ajoute sur le côté gauche de l'écran un rail vertical permanent contenant les portraits des personnages actifs du groupe.

Ce rail est un sélecteur de personnage global. Un clic sur un portrait change l'unique personnage sélectionné du runtime. Les surfaces dépendant d'un personnage — inventaire, feuille de personnage, compétences/talents, spellbook, barre d'actions et futures surfaces character-scoped — utilisent toutes cette même sélection.

Le rail appartient au Persistent HUD. Il n'appartient ni à l'inventaire, ni à la feuille de personnage, ni au Skills, ni au Combat HUD.

## 2. Autorité de sélection

L'autorité existante reste unique :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState::SelectedCharacterIndex
```

Aucun `ActiveCharacter`, `CurrentPortraitCharacter` ou index parallèle n'est introduit.

Le `CharacterId` stable identifie un personnage. Le `CharacterIndex` n'est qu'un index runtime résolu à partir de cet identifiant.

## 3. Contenu visuel cible d'un portrait

Chaque portrait contient uniquement :

```text
Portrait
Barre de vie
Barre de mana si le personnage possède un pool de mana
Barre d'armure physique
Barre d'armure magique
Cadre/indication décorative de sélection
```

La correction du contrat adoptée le 16 octobre 2026 précise que **la barre de mana est affichée uniquement lorsque MaximumMana > 0**.

Aucun nom, classe, niveau, poids, équipement ou liste de statuts n'est imposé dans le portrait compact.

## 4. Tooltip

Un tooltip sera affiché au survol du portrait, mais son contenu métier reste volontairement hors scope de UI-PARTYHUD01.1. Le contenu sera défini dans un sous-jalon ultérieur afin de ne pas transformer le portrait compact en fiche de personnage.

## 5. Placement et ownership UI

Cible :

```text
WBP_GridPersistentHud
├── Panel_PartyPortraits
├── Global Navigation
└── Persistent Action Bar
```

Le root du Persistent HUD reste `SelfHitTestInvisible`. Seuls les portraits et les contrôles interactifs doivent intercepter la souris.

Aucun panneau transparent plein écran ne doit bloquer l'interaction avec le donjon.

## 6. UI-PARTYHUD01.1 — Read Model

UI-PARTYHUD01.1 introduit :

```cpp
FGridPartyPortraitView
```

Ce read model est une projection en lecture seule. Il ne devient jamais une autorité gameplay.

Champs exposés :

```text
CharacterId
CharacterIndex
Portrait
bIsSelected

CurrentHealth
MaximumHealth
HealthRatio

CurrentMana
MaximumMana
bHasMana
ManaRatio

CurrentPhysicalArmor
ReferencePhysicalArmor
PhysicalArmorRatio

CurrentMagicalArmor
ReferenceMagicalArmor
MagicalArmorRatio
```

## 7. Vie

Autorités existantes :

```text
CurrentHealth = Summary.Resources.CurrentHealth
MaximumHealth = Summary.DerivedStats.MaxHealth
```

Ratio :

```text
HealthRatio = clamp(CurrentHealth / MaximumHealth, 0..1)
```

Le maximum est protégé à au moins 1.

## 8. Mana

Autorités existantes :

```text
CurrentMana = Summary.Resources.CurrentMana
MaximumMana = Summary.DerivedStats.MaxMana
```

Règle d'affichage :

```text
bHasMana = MaximumMana > 0
```

Si `MaximumMana == 0` :

```text
bHasMana = false
ManaRatio = 0
```

UMG devra masquer la barre de mana dans ce cas.

## 9. Armure physique

Le contrat RPG03.3 reste l'autorité.

```text
CurrentPhysicalArmor =
    GetCharacterSummary().Resources.CurrentPhysicalArmor

ReferencePhysicalArmor =
    Class.BasePhysicalArmor
    + EquipmentStatBonus.ArmorBonus
```

Ratio :

```text
PhysicalArmorRatio =
    ReferencePhysicalArmor > 0
        ? clamp(CurrentPhysicalArmor / ReferencePhysicalArmor, 0..1)
        : 0
```

Le read model ne fabrique pas de maximum sauvegardé supplémentaire.

## 10. Armure magique

Le contrat RPG03.3 reste également inchangé :

```text
CurrentMagicalArmor =
    GetCharacterSummary().Resources.CurrentMagicalArmor

ReferenceMagicalArmor =
    Class.BaseMagicalArmor
```

Ratio :

```text
MagicalArmorRatio =
    ReferenceMagicalArmor > 0
        ? clamp(CurrentMagicalArmor / ReferenceMagicalArmor, 0..1)
        : 0
```

Une référence à zéro produit une barre vide, jamais une barre artificiellement pleine.

## 11. Résolution de la classe

Le read model utilise d'abord le cache runtime `Character.ClassDefinition`.

Si ce cache ne correspond pas au `ClassId`, la résolution canonique existante est utilisée :

```text
FRPGAuthoringIdentityResolver::ResolveClassById()
```

Aucune nouvelle table de classes n'est créée.

## 12. Ordre visuel du groupe

UI-PARTYHUD01.1 introduit dans `UGridPartyInventoryComponent` :

```cpp
TArray<FGuid> PartyPortraitOrder;
```

Cette donnée est **transient dans UI-PARTYHUD01.1**.

La persistance Save / Load reste réservée au sous-jalon :

```text
UI-PARTYHUD01.5 — Save / Load
```

Le futur Save stockera les `CharacterId`, jamais les indices runtime.

## 13. Invariant critique : ActiveCharacters n'est jamais réordonné

Un drag futur des portraits ne doit jamais permuter :

```text
PartyInventoryState.ActiveCharacters
PartyInventoryState.ActiveEquipment
SelectedCharacterIndex
états de tour
états de combat
compétences
talents
spellbook
inventaires
```

Seul `PartyPortraitOrder` change.

Cette séparation est obligatoire car de nombreux systèmes utilisent encore les indices de `ActiveCharacters`.

## 14. Sanitation de PartyPortraitOrder

`SanitizePartyPortraitOrder()` garantit :

1. suppression des GUID invalides ;
2. suppression des GUID qui ne correspondent plus à un personnage actif ;
3. suppression des doublons ;
4. conservation de l'ordre valide choisi par le joueur ;
5. ajout à la fin de tout personnage actif absent.

Exemple :

```text
ActiveCharacters : A B C
PartyPortraitOrder brut : C INVALID C A X

résultat :
C A B
```

## 15. Changement du roster

`NotifyPartyInventoryChanged()` sanitize désormais l'ordre avant notification.

Conséquences :

- une nouvelle recrue est ajoutée à la fin ;
- un personnage retiré disparaît de l'ordre ;
- un changement de sélection ne modifie pas l'ordre ;
- une notification normale ne peut pas créer de doublon.

## 16. Résolution CharacterId -> index

API :

```cpp
int32 ResolveActiveCharacterIndexById(FGuid CharacterId) const;
```

Elle parcourt `ActiveCharacters` et renvoie l'index runtime actuel.

Un GUID invalide ou absent renvoie :

```text
INDEX_NONE
```

## 17. Construction du rail

API :

```cpp
bool GetPartyPortraitViews(TArray<FGridPartyPortraitView>& OutViews);
```

Séquence :

```text
SanitizePartyPortraitOrder
    -> pour chaque CharacterId
        -> ResolveActiveCharacterIndexById
        -> GetCharacterSummary
        -> projection portrait / sélection / ressources / ratios
```

La fonction retourne vrai lorsque chaque entrée de l'ordre a pu être projetée.

## 18. Réorganisation

API C++ préparée pour le futur drag :

```cpp
bool ReorderPartyPortrait(
    FGuid SourceCharacterId,
    FGuid TargetCharacterId,
    bool bInsertAfter);
```

Sémantique :

```text
A B C D

Reorder(D, A, false)
    -> D A B C

Reorder(A, C, true)
    -> B C A D
```

La transaction modifie uniquement `PartyPortraitOrder`, puis notifie l'UI.

## 19. Clic vs drag

La réalisation UMG ultérieure devra respecter :

```text
MouseDown
    -> potentiel drag

MouseUp sans drag
    -> sélection

drag valide
    -> réorganisation
    -> pas de clic parasite
```

Ce comportement n'est pas implémenté dans UI-PARTYHUD01.1.

## 20. Séparation avec le drag d'inventaire

Le futur drag de portraits utilisera une opération distincte de :

```text
UGridInventoryDragDropOperation
```

Il ne transportera aucun `CursorItem`.

Le rail persistant ne devient pas une cible de transfert d'objet dans le contrat actuel.

## 21. Widget futur

UI-PARTYHUD01.2 créera un widget dédié, et ne détournera pas le `UGridPartyMemberWidget` existant.

Cible :

```text
UGridPartyPortraitHudWidget
WBP_PartyPortraitHud
```

Motif : `UGridPartyMemberWidget` possède déjà des responsabilités inventaire, drop d'items, surcharge et status effects.

## 22. Surfaces synchronisées par SelectedCharacterIndex

Après clic sur un portrait, les surfaces character-scoped devront lire le même personnage :

```text
Character Sheet
Inventory
Skills / Talents
Spellbook
Persistent Action Bar
futures surfaces character-scoped
```

Map, Journal global et Help/Codex global ne deviennent pas artificiellement propres à un personnage.

## 23. Combat

Le personnage sélectionné et le combattant dont c'est le tour restent deux notions distinctes.

Le Turn Manager peut continuer à appeler `SetSelectedCharacterIndex()` lorsqu'un tour de personnage devient actif. UI-PARTYHUD01 ne crée aucune sélection de combat parallèle.

## 24. Persistance

État actuel de UI-PARTYHUD01.1 :

```text
PartyPortraitOrder = Transient
```

UI-PARTYHUD01.5 décidera du bump SaveGame nécessaire et ajoutera le round-trip durable.

Aucun changement de schéma SaveGame n'est effectué dans le présent sous-jalon.

## 25. Tests Automation ajoutés

Filtre :

```text
Grimrock.UI.PartyHud01
```

Tests UI-PARTYHUD01.1 :

```text
Grimrock.UI.PartyHud01.ViewProjection
Grimrock.UI.PartyHud01.OrderSanitization
Grimrock.UI.PartyHud01.ReorderPreservesRuntime
Grimrock.UI.PartyHud01.RosterChangeSanitization
```

Ils vérifient notamment :

- vie ;
- mana conditionnelle ;
- références d'armure ;
- ratios ;
- sanitation ;
- ajout d'une recrue ;
- retrait d'un membre ;
- réorganisation visuelle ;
- préservation stricte de `ActiveCharacters` ;
- préservation de `SelectedCharacterIndex`.

## 26. Validation locale attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"
```

Le sous-jalon ne sera déclaré **VALIDÉ** qu'après réception du log local UE5.5.4.

## 27. Hors scope de UI-PARTYHUD01.1

Non réalisés ici :

- WBP du portrait ;
- placement du rail dans `WBP_GridPersistentHud` ;
- clic ;
- drag souris ;
- indicateur d'insertion ;
- tooltip final ;
- persistance SaveGame ;
- validation PIE visuelle.

## 28. Découpage adopté

```text
UI-PARTYHUD01.1  Read Model & Order Authority
UI-PARTYHUD01.2  Persistent Portrait Rail
UI-PARTYHUD01.3  Character Selection
UI-PARTYHUD01.4  Portrait Drag Reorder
UI-PARTYHUD01.5  Save / Load
UI-PARTYHUD01.6  Tooltip
UI-PARTYHUD01.7  UMG / PIE Final
```

## 29. Invariants définitifs

1. `SelectedCharacterIndex` reste l'unique autorité de sélection.
2. `CharacterId` est l'identité stable.
3. `CharacterIndex` n'est jamais persisté comme identité du portrait.
4. Le read model est en lecture seule.
5. UMG ne calcule aucune règle RPG.
6. `ActiveCharacters` n'est jamais réordonné par le HUD.
7. `ActiveEquipment` n'est jamais réordonné par le HUD.
8. La mana est affichée uniquement si `MaximumMana > 0`.
9. Une référence d'armure nulle donne un ratio nul.
10. L'ordre runtime est sanitizé à chaque changement du groupe.
11. La persistance de l'ordre est différée explicitement à UI-PARTYHUD01.5.
12. Aucun `.uasset` n'est modifié à l'aveugle.


## 30. Validation UE5.5.4 — 16 octobre 2026

Validation locale exécutée via le harness TD04.2 :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20261016-173540
```

Logs :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20261016-173540\Automation.log
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20261016-173540\Automation.console.log
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20261016-173540\Automation.summary.txt
```

Conclusion :

```text
[OK] Automation filter validated.
TD04.2 validation completed successfully.
```

**UI-PARTYHUD01.1 est donc CLOS et VALIDÉ.**

Le prochain sous-jalon autorisé est :

```text
UI-PARTYHUD01.2 — Persistent Portrait Rail
```


## 31. UI-PARTYHUD01.2 — Persistent Portrait Rail

### 31.1 Implémentation C++

UI-PARTYHUD01.2 ajoute le presenter dédié :

```text
UGridPartyPortraitHudWidget
```

Fichiers :

```text
Source/GrimrockPrototype/Public/UI/GridPartyPortraitHudWidget.h
Source/GrimrockPrototype/Private/UI/GridPartyPortraitHudWidget.cpp
```

Le widget consomme exclusivement :

```text
FGridPartyPortraitView
```

Il ne lit ni ne modifie directement `FGridCharacterInventoryState`.

### 31.2 Bindings UMG du portrait

Le futur `WBP_PartyPortraitHud` doit avoir comme parent C++ :

```text
UGridPartyPortraitHudWidget
```

Bindings disponibles :

```text
Image_Portrait
ProgressBar_Health
ProgressBar_Mana
ProgressBar_PhysicalArmor
ProgressBar_MagicalArmor
Border_Selected
```

Tous sont `BindWidgetOptional` afin que la compilation C++ et les Automation tests restent indépendants de la matérialisation du Blueprint.

Le contrat visuel reste strict :

```text
portrait
vie
mana uniquement si MaximumMana > 0
armure physique
armure magique
cadre décoratif de sélection
```

Le presenter ne contient ni poids, ni statut, ni nom, ni niveau, ni équipement.

### 31.3 Règles de visibilité

```text
Health            -> toujours visible
Mana              -> visible seulement si bHasMana
Physical Armor    -> toujours visible
Magical Armor     -> toujours visible
Border_Selected   -> visible seulement si bIsSelected
```

Une barre d'armure dont la référence vaut zéro reste visible avec un pourcentage nul.

Le root du presenter est `SelfHitTestInvisible`. Les éléments purement décoratifs sont `HitTestInvisible`.

Cette règle préserve les interactions avec le monde et permettra à UI-PARTYHUD01.3 d'introduire un enfant cliquable sans rendre le fond du HUD bloquant.

### 31.4 Ownership dans le Persistent HUD

`UGridPersistentHudWidget` possède désormais :

```text
PartyPortraitWidgetClass
Panel_PartyPortraits
PartyPortraitWidgets
```

`Panel_PartyPortraits` est contractuellement un `VerticalBox`.

La collection runtime `PartyPortraitWidgets` suit exactement l'ordre fourni par :

```text
UGridPartyInventoryComponent::GetPartyPortraitViews()
```

Elle ne possède aucune autorité de gameplay.

### 31.5 Rafraîchissement événementiel

Le Persistent HUD s'abonne à :

```text
UGridPartyInventoryComponent::OnPartyInventoryChanged
```

À chaque notification :

```text
HandlePartyInventoryChanged
    -> RefreshPartyPortraits
    -> GetPartyPortraitViews
    -> mise à jour ou reconstruction du rail
```

Aucun polling RPG n'est ajouté à `NativeTick`.

Le `NativeTick` déjà présent reste réservé au dimensionnement de la barre d'actions.

### 31.6 Conservation des widgets

Lorsque le nombre, l'ordre et les `CharacterId` n'ont pas changé, les presenters existants sont rafraîchis sur place.

Le rail est reconstruit uniquement si :

```text
nombre différent
ordre différent
CharacterId différent
parent invalide
widget invalide
```

Le coût reste borné au maximum courant de six personnages.

### 31.7 Matérialisation UMG requise

Aucun `.uasset` n'est modifié automatiquement.

Dans Unreal Editor, créer le widget dans le même domaine UI que le Persistent HUD et les widgets d'inventaire :

```text
/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_PartyPortraitHud
Parent Class = UGridPartyPortraitHudWidget
```

Ce chemin est cohérent avec le dépôt courant, où `WBP_GridPersistentHud`, `WBP_CharacterSheet`, `WBP_InventoryBag` et `WBP_PartyMember` résident déjà sous `Blueprints/UI/Inventory`.

Puis ajouter dans `WBP_GridPersistentHud`, directement sous `CanvasPanel_Root` :

```text
SizeBox_PartyPortraitRail
└── Panel_PartyPortraits [VerticalBox]
```

Réglages obligatoires de départ :

```text
SizeBox_PartyPortraitRail
    Width Override  = 120
    Height Override = 850
    Canvas Anchors  = Top Left
    Position        = 20,20

Panel_PartyPortraits
    Type       = VerticalBox
    Is Variable = true
    Visibility = Not Hit-Testable (Self Only)
```

Le `SizeBox_PartyPortraitRail` fournit l'emprise de layout du rail. Il ne doit pas être omis de la procédure UMG.

Et renseigner sur les Class Defaults de `WBP_GridPersistentHud` :

```text
PartyPortraitWidgetClass = WBP_PartyPortraitHud
```

Le layout exact, les couleurs, les dimensions et les brushes sont des responsabilités UMG.

### 31.8 Hors scope de UI-PARTYHUD01.2

Toujours différés :

```text
clic de sélection                 -> UI-PARTYHUD01.3
drag / insertion                  -> UI-PARTYHUD01.4
persistance de l'ordre            -> UI-PARTYHUD01.5
contenu final du tooltip          -> UI-PARTYHUD01.6
validation visuelle PIE complète  -> UI-PARTYHUD01.7
```

### 31.9 Automation

Deux tests supplémentaires sont ajoutés au filtre existant :

```text
Grimrock.UI.PartyHud01.PersistentPortraitWidgetPresentation
Grimrock.UI.PartyHud01.PersistentRailContract
```

Le filtre `Grimrock.UI.PartyHud01` contient donc désormais **6 tests** attendus.

Validation locale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"
```

Validation C++ reçue le 16 octobre 2026 :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 6
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport TD04.2 :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20261016-174645
```

**Socle C++ UI-PARTYHUD01.2 : VALIDÉ.**

UI-PARTYHUD01.2 ne sera déclaré **CLOS** qu'après matérialisation UMG conforme dans `WBP_PartyPortraitHud` et `WBP_GridPersistentHud`, suivie d'une validation visuelle PIE.


### 31.10 Checklist UMG obligatoire pour obtenir un rendu en PIE

La validation Automation C++ ne matérialise aucun `.uasset`. Pour que le rail soit réellement visible en PIE, **les quatre conditions suivantes sont obligatoires** :

```text
1. WBP_PartyPortraitHud
   Parent Class = UGridPartyPortraitHudWidget

2. WBP_GridPersistentHud
   contient obligatoirement :
   SizeBox_PartyPortraitRail
   └── Panel_PartyPortraits [VerticalBox]
   Panel_PartyPortraits : Is Variable = true

3. WBP_GridPersistentHud / Class Defaults
   Party Portrait Widget Class = WBP_PartyPortraitHud

4. BP_GrimrockPartyPawn / Class Defaults
   Persistent Hud Widget Class = WBP_GridPersistentHud
```

La condition 4 existe déjà dans le flux UI-GLOBALHUD01 lorsque la navigation et la barre d'actions persistantes sont visibles. Elle doit néanmoins être vérifiée si le Persistent HUD complet n'apparaît pas.

Le presenter créé dynamiquement est ajouté à un `VerticalBox` en taille automatique. `WBP_PartyPortraitHud` doit donc fournir une **Desired Size non nulle**. La solution recommandée est un `SizeBox` root avec Width/Height Override explicites. Un Canvas root sans taille désirée peut produire un widget runtime de taille nulle et donc invisible.

Diagnostic du C++ :

```text
Panel_PartyPortraits absent
    -> aucun rail créé

PartyPortraitWidgetClass non renseigné
    -> Panel_PartyPortraits vidé, aucun portrait créé

GetPartyPortraitViews() vide
    -> rail vide

WBP_PartyPortraitHud avec Desired Size = 0
    -> widgets créés mais visuellement invisibles
```

Ces contrôles UMG font partie de la fermeture de UI-PARTYHUD01.2 et ne sont pas couverts par les tests Automation de réflexion C++.


### 31.11 Terminologie UE5.5.4 — Visibility

Dans le code C++, l'enum reste :

```cpp
ESlateVisibility::SelfHitTestInvisible
ESlateVisibility::HitTestInvisible
```

Mais dans le **Designer UMG de Unreal Engine 5.5.4**, les libellés affichés sont :

```text
SelfHitTestInvisible -> Not Hit-Testable (Self Only)
HitTestInvisible     -> Not Hit-Testable (Self & All Children)
```

La documentation UI-PARTYHUD01 doit utiliser les libellés du Designer lorsqu'elle donne une manipulation manuelle dans l'éditeur.


## 32. Validation finale UI-PARTYHUD01.2 — 16 octobre 2026

Le socle C++ a été validé via TD04.2 :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 6
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20261016-174645
```

La matérialisation UMG a ensuite été validée en PIE par capture utilisateur :

```text
WBP_PartyPortraitHud
    -> visible dans le rail persistant

WBP_GridPersistentHud
    -> SizeBox_PartyPortraitRail
        -> Panel_PartyPortraits

Groupe actif
    -> 2 entrées visibles à gauche
    -> portrait du personnage principal visible
    -> deuxième entrée conservée malgré l'absence actuelle de portrait du personnage de rencontre
```

L'absence d'image du deuxième personnage est une donnée de présentation manquante du personnage de rencontre et non une défaillance du rail.

**UI-PARTYHUD01.2 est VALIDÉ et CLOS.**

Prochain sous-jalon :

```text
UI-PARTYHUD01.3 — Character Selection
```


## 33. UI-PARTYHUD01.3 — Character Selection

### 33.1 Autorité inchangée

Le clic sur un portrait ne crée aucune nouvelle sélection.

Chaîne autoritaire :

```text
UGridPartyPortraitHudWidget
    -> OnPortraitClicked(CharacterId)
UGridPersistentHudWidget
    -> ResolveActiveCharacterIndexById(CharacterId)
UGridPartyInventoryComponent
    -> SetSelectedCharacterIndex(CharacterIndex)
    -> OnPartyInventoryChanged
```

`SelectedCharacterIndex` reste l'unique autorité runtime.

### 33.2 Identité transportée par le clic

Le portrait diffuse exclusivement :

```text
CharacterId : FGuid
```

Il ne diffuse pas `CharacterIndex` comme identité. Le parent résout l'index au moment de la transaction afin de rester correct si le roster runtime évolue.

### 33.3 Interaction native sans nouveau contrôle UMG

Aucun `Button_Portrait` n'est ajouté au WBP.

Le `UGridPartyPortraitHudWidget` lui-même devient la surface interactive :

```text
MouseDown gauche
    -> armement du clic
    -> DetectDragIfPressed

MouseUp gauche sans drag
    -> RequestSelection()
    -> OnPortraitClicked(CharacterId)

drag détecté
    -> annulation du clic
    -> aucune réorganisation dans .3
```

Le drag réel et son opération dédiée restent réservés à UI-PARTYHUD01.4.

### 33.4 Hit testing

Le rail parent conserve dans le Designer UE5.5.4 :

```text
Panel_PartyPortraits
Visibility = Not Hit-Testable (Self Only)
```

Cela autorise ses enfants à recevoir la souris.

Le portrait individuel est forcé runtime à :

```cpp
ESlateVisibility::Visible
```

car il constitue désormais une surface interactive.

Les éléments décoratifs internes (portrait, barres, cadre) restent non bloquants.

### 33.5 Synchronisation des surfaces

Après `SetSelectedCharacterIndex()`, la notification autoritaire existante rafraîchit les consommateurs déjà abonnés :

```text
Character Sheet
Inventory
Skills / Talents
Spellbook
held visual
portrait rail
```

Le Persistent HUD exécute ensuite `RefreshFromSources()` afin de resynchroniser son propre chrome et sa barre d'actions après le retour de la notification.

### 33.6 Aucun changement UMG requis

Si UI-PARTYHUD01.2 est déjà matérialisé et visible, UI-PARTYHUD01.3 ne demande :

```text
aucun nouveau widget
aucun nouveau Button
aucun Event Graph
aucun binding Blueprint
```

Important : ne pas rendre le contenu complet de `WBP_PartyPortraitHud` `Not Hit-Testable (Self & All Children)`, car le portrait doit pouvoir recevoir le clic.

### 33.7 Automation

Tests ajoutés :

```text
Grimrock.UI.PartyHud01.PortraitInteractionContract
Grimrock.UI.PartyHud01.CharacterSelection
```

Le filtre contient désormais **8 tests** attendus.

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"
```

PIE attendu :

```text
clic portrait 2
    -> cadre sélection portrait 2
    -> SelectedCharacterIndex = personnage 2
    -> surfaces character-scoped suivent personnage 2

clic portrait 1
    -> sélection revient au personnage 1
```

UI-PARTYHUD01.3 ne sera déclaré VALIDÉ qu'après Automation locale et vérification PIE du clic.


## 34. Validation finale UI-PARTYHUD01.3 — 17 octobre 2026

Validation TD04.2 :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 8
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20261017-070714
```

Validation PIE :

```text
clic portrait 2 -> sélection personnage 2
clic portrait 1 -> sélection personnage 1
```

Le clic fonctionne donc via le portrait lui-même, sans `Button` UMG additionnel et sans nouvelle autorité de sélection.

**UI-PARTYHUD01.3 est VALIDÉ et CLOS.**

Prochain sous-jalon :

```text
UI-PARTYHUD01.4 — Portrait Drag Reorder
```


## 35. UI-PARTYHUD01.4 — Portrait Drag Reorder

### 35.1 Opération de drag dédiée

UI-PARTYHUD01.4 introduit :

```text
UGridPartyPortraitDragDropOperation
```

Le payload contient uniquement :

```text
SourceCharacterId : FGuid
```

Il ne contient :

```text
aucun item
aucun CursorItem
aucun CharacterIndex persistant
aucune donnée de combat
```

Il est donc strictement séparé de `UGridInventoryDragDropOperation` et de la hotbar.

### 35.2 Détection du drag

Le MouseDown de UI-PARTYHUD01.3 utilisait déjà `DetectDragIfPressed`.

UI-PARTYHUD01.4 matérialise désormais l'opération dans `NativeOnDragDetected()` :

```text
portrait valide
    -> UGridPartyPortraitDragDropOperation
    -> SourceCharacterId
    -> Pivot = MouseDown
```

Le clic est annulé dès que le drag est détecté.

### 35.3 Drop avant / après

Le portrait cible calcule la position locale verticale du pointeur :

```text
moitié supérieure -> insertion AVANT la cible
moitié inférieure -> insertion APRÈS la cible
milieu exact      -> APRÈS
```

Le choix est fourni par :

```cpp
UGridPartyPortraitHudWidget::ResolveInsertAfter()
```

Puis :

```text
UGridPartyPortraitHudWidget::NativeOnDrop
    -> UGridPersistentHudWidget::HandlePartyPortraitDrop
    -> UGridPartyInventoryComponent::ReorderPartyPortrait
```

### 35.4 Invariants

La transaction modifie exclusivement :

```text
PartyPortraitOrder
```

Elle ne réordonne jamais :

```text
ActiveCharacters
ActiveEquipment
SelectedCharacterIndex
inventaires
skills
talents
spellbook
états combat
```

Un drop sur le portrait source lui-même est rejeté.

Un drop hors portrait ne possède aucune cible et ne modifie rien.

### 35.5 Rafraîchissement

`ReorderPartyPortrait()` utilise le chemin de notification existant :

```text
PartyPortraitOrder modifié
    -> NotifyPartyInventoryChanged(INDEX_NONE)
    -> OnPartyInventoryChanged
    -> RefreshPartyPortraits()
```

Le Persistent HUD reconstruit alors le rail dans le nouvel ordre des `CharacterId`.

### 35.6 UMG

Aucune nouvelle modification de `WBP_PartyPortraitHud` n'est requise pour le fonctionnement de UI-PARTYHUD01.4.

Le contrat UI-PARTYHUD01.2/.3 reste :

```text
Panel_PartyPortraits = Not Hit-Testable (Self Only)
portrait runtime     = hit-testable
contenu décoratif    = non bloquant
```

Un indicateur graphique explicite de ligne d'insertion n'est pas introduit dans ce sous-jalon ; la cible et la moitié de drop déterminent la transaction.

### 35.7 Automation

Deux tests supplémentaires :

```text
Grimrock.UI.PartyHud01.DedicatedPortraitDragPayload
Grimrock.UI.PartyHud01.PortraitDropReorder
```

Le filtre contient désormais **10 tests** attendus.

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"
```

PIE :

```text
avec au moins 2 portraits :
- drag portrait 2 vers moitié haute du portrait 1 -> 2 passe avant 1
- drag vers moitié basse -> insertion après
- la sélection courante reste inchangée
- un drag ne déclenche pas un clic parasite
```

UI-PARTYHUD01.4 sera clos après validation Automation et PIE.


## 36. Validation UI-PARTYHUD01.4 et correctif curseur — 17 octobre 2026

Validation fonctionnelle reçue :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 10
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20261017-073041
```

Le drag/reorder est également validé en PIE.

Un défaut visuel a cependant été observé lors du clic/drag sur un portrait :

```text
Cursor_Default custom visible
+
curseur système Windows visible
```

Cause ciblée : depuis UI-PARTYHUD01.3, le portrait est une surface UMG hit-testable. Slate peut donc résoudre un curseur UMG/platform sur cette surface en plus du curseur custom global.

Correctif :

```cpp
UGridPartyPortraitHudWidget::SetPortraitView()
    -> SetCursor(EMouseCursor::None)
```

Cette correction est volontairement locale au portrait :

- aucun changement de `WBP_GridMouseCursor` ;
- aucun changement du PlayerController ;
- aucun changement de l'inventaire ou de la hotbar ;
- aucun changement du drag/reorder validé.

UI-PARTYHUD01.4 sera déclaré **CLOS** après vérification PIE que seul `Cursor_Default` reste visible pendant clic et drag.


## 37. Validation finale UI-PARTYHUD01.4 — 17 octobre 2026

Validation TD04.2 :

```text
Filter                 : Grimrock.UI.PartyHud01
Succeeded              : 10
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20261017-073041
```

Validation PIE :

```text
drag portrait avant/après -> OK
SelectedCharacterIndex    -> inchangé pendant le reorder
clic parasite             -> absent
double curseur Windows    -> corrigé et validé
Cursor_Default custom     -> seul curseur visible
```

**UI-PARTYHUD01.4 est VALIDÉ et CLOS.**

Prochain sous-jalon :

```text
UI-PARTYHUD01.5 — Save / Load
```


## 38. UI-PARTYHUD01.5 — Save / Load

### 38.1 Schéma durable

UI-PARTYHUD01.5 ajoute au `UGrimrockPartySaveGame` :

```cpp
UPROPERTY(SaveGame)
TArray<FGuid> PartyPortraitOrder;
```

Le SaveGame passe :

```text
v24 -> v25
```

Politique inchangée :

```text
SaveVersion == 25 -> accepté
SaveVersion != 25 -> rejet
aucune migration arrière
```

La v24 n'est donc pas migrée silencieusement.

### 38.2 Séparation runtime / disque

Le composant conserve :

```text
UGridPartyInventoryComponent::PartyPortraitOrder
    -> Transient
    -> ordre runtime de présentation
```

Le Save contient une copie durable :

```text
UGrimrockPartySaveGame::PartyPortraitOrder
    -> SaveGame
    -> TArray<FGuid>
```

Aucun ordre n'est ajouté à `FGridPartyInventoryState`, afin de ne pas mélanger roster gameplay et préférence de présentation UI.

### 38.3 Sauvegarde

`AGrimrockPartyPawn::SaveCurrentGame()` :

```text
SanitizePartyPortraitOrder()
    -> copie PartyInventoryState
    -> copie PartyPortraitOrder
    -> SaveGameToSlot
```

### 38.4 Chargement

`LoadCurrentGameData()` restaure d'abord le roster puis applique :

```text
RestorePartyInventoryState(...)
RestorePartyPortraitOrder(SaveGame->PartyPortraitOrder)
```

`RestorePartyPortraitOrder()` resanitise contre les personnages actifs et notifie le HUD.

Ainsi :

```text
id invalide      -> supprimé
id dupliqué      -> dédupliqué
id obsolète      -> supprimé
recrue manquante -> ajoutée à la fin
ordre vide       -> reconstruit depuis ActiveCharacters
```

### 38.5 Atomicité / rollback

Le chargement capture aussi `PreviousPortraitOrder`. Si une étape ultérieure échoue, le rollback restaure conjointement le roster, l'ordre des portraits, le runtime du donjon et la position du groupe.

### 38.6 New Game

`ResetPartyForNewGame()` efface explicitement `PartyPortraitOrder` avant de reconstruire le roster initial.

### 38.7 Automation / PIE

Tests ajoutés :

```text
Grimrock.UI.PartyHud01.SaveSchemaV25
Grimrock.UI.PartyHud01.SavePortraitOrderRoundTrip
Grimrock.UI.PartyHud01.RestorePortraitOrderSanitization
```

Total attendu : **13 tests**.

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.PartyHud01"
```

Validation PIE / Save-Continue :

```text
1. inverser l'ordre des portraits ;
2. sauvegarder ;
3. quitter/recharger via Continue ;
4. vérifier que l'ordre est restauré ;
5. vérifier que SelectedCharacterIndex reste l'autorité de sélection.
```

Aucune modification UMG n'est requise pour UI-PARTYHUD01.5.


## 39. Validation finale UI-PARTYHUD01.5 — 17 octobre 2026

Validation utilisateur reçue après implémentation du schéma **v25 exact-match**.

### Filtre principal

```text
Grimrock.UI.PartyHud01
Succeeded              : 13
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20261017-081236
```

### Régressions ciblées

```text
Grimrock.Map.MON21_6_5.Persistence
Succeeded : 4
Warnings  : 0
Failed    : 0
ExitCode  : 0
Report    : Saved/Automation/TD04/TD04-20261017-081249

Grimrock.RPG.LEVELUX01
Succeeded : 8
Warnings  : 0
Failed    : 0
ExitCode  : 0
Report    : Saved/Automation/TD04/TD04-20261017-081303

Grimrock.TechnicalDebt.TD07_3_2.SaveContract
Succeeded : 4
Warnings  : 0
Failed    : 0
ExitCode  : 0
Report    : Saved/Automation/TD04/TD04-20261017-081324
```

Le Save/Load du rail est accepté comme validé par l'utilisateur.

**UI-PARTYHUD01.5 est VALIDÉ et CLOS.**

Prochain sous-jalon :

```text
UI-PARTYHUD01.6 — Tooltip
```


## 40. UI-PARTYHUD01.6 — Tooltip

### 40.1 Objectif

Le survol d'un portrait expose uniquement les informations compactes qui ne
peuvent pas être lues directement dans les petites barres du rail :

```text
Nom
Classe - Niveau
PV courant / maximum
Mana courant / maximum        seulement si MaxMana > 0
Armure physique courant / référence
Armure magique courant / référence
```

Le tooltip reste volontairement distinct de la feuille de personnage. Il
n'affiche pas :

```text
équipement
poids
caractéristiques détaillées
résistances détaillées
Skills / Talents
liste des statuts
description narrative
```

### 40.2 Read model

`FGridPartyPortraitView` ajoute uniquement des données de présentation
transitoires :

```cpp
FText DisplayName;
FText ClassDisplayName;
int32 Level;
```

Le widget ne relit jamais directement le personnage et ne recalcule pas les
valeurs RPG.

### 40.3 Référence d'armure RPG03.3

UI-PARTYHUD01.6 ferme le gap historique identifié en UI-PARTYHUD01.1.

La référence affichée et utilisée par les barres suit désormais le contrat
RPG03.3 complet :

```text
Physical reference
    = Class.BasePhysicalArmor
    + EquipmentStatBonus.ArmorBonus
    + modificateurs C2 de référence

Magical reference
    = Class.BaseMagicalArmor
    + modificateurs C2 de référence
```

Le calcul réutilise les autorités existantes :

```text
FGridCombatModifierResolver::CollectCharacterModifiers
FGridCombatModifierResolver::Resolve
FGridCombatArmorEffectResolver::ApplyReferenceModifiers
```

Aucun second calcul d'armure n'est créé dans UMG.

### 40.4 Présentation

Aucun nouveau Widget Blueprint n'est nécessaire.

Le portrait réutilise la classe de tooltip compacte déjà configurée sur le
Persistent HUD :

```text
WBP_GridPersistentHud
Navigation Tooltip Widget Class
        |
        v
UGridNavigationTooltipWidget
        |
        +-- boutons ESC / I / K / G / M / J / H
        +-- portraits du groupe
```

Si cette classe n'est pas configurée, le portrait conserve un fallback natif
`ToolTipText`. Le tooltip est non hit-testable et ne prend aucune autorité
sur le clic, le drag ou le curseur custom.

Exemple :

```text
Elias
Mage - Niveau 5
PV 75 / 100
Mana 25 / 50
Armure physique 8 / 10
Armure magique 2 / 4
```

Pour un personnage sans mana, la ligne Mana disparaît entièrement.

### 40.5 Refresh

Le tooltip est reconstruit uniquement lors de `SetPortraitView()`, lui-même
alimenté par le refresh événementiel du rail. Aucun polling RPG par Tick n'est
ajouté.

### 40.6 Persistance

Aucun champ durable supplémentaire n'est introduit :

```text
CurrentSaveVersion = 25
```

UI-PARTYHUD01.6 ne modifie donc pas le schéma SaveGame.

### 40.7 Automation

Trois tests sont ajoutés :

```text
Grimrock.UI.PartyHud01.TooltipReadModel
Grimrock.UI.PartyHud01.TooltipEffectiveArmorReference
Grimrock.UI.PartyHud01.TooltipPresentation
```

Le filtre principal doit maintenant exécuter **16 tests**.

### 40.8 Validation PIE attendue

1. survoler le portrait d'un personnage avec mana ;
2. vérifier les six lignes du tooltip ;
3. survoler un personnage sans mana ;
4. vérifier que la ligne Mana est absente ;
5. vérifier que le tooltip disparaît en quittant le portrait ;
6. vérifier que clic de sélection et drag/reorder restent fonctionnels ;
7. vérifier que seul `Cursor_Default` reste visible.

Aucune modification UMG n'est requise pour ce sous-jalon.
