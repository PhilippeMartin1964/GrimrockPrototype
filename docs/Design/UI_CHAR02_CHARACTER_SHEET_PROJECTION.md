# UI-CHAR02 — Character Sheet Projection and Layout Contract

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ + CONTRAT UMG ; validation locale UE5.5.4 à fournir**

## Objectif

UI-CHAR02 prépare la refonte visible de la feuille de personnage en réutilisant les données et widgets existants.

Le ticket ne crée pas de nouveau modèle de personnage ni de second ViewModel. La projection continue de partir de :

```text
UGridPartyInventoryComponent
    -> FGridInventoryCharacterSummary
        -> UGridInventoryWidget
            -> widgets UMG optionnels
```

## Sections de la feuille

La cible fonctionnelle est organisée en cinq blocs.

### Identité

Champs existants réutilisés :

```text
Text_CharacterName
Text_CharacterRace
Text_CharacterClass
Text_CharacterLevel
Text_CharacterExperience
```

### Attributs

Champs existants :

```text
Text_CharacterStrength
Text_CharacterDexterity
Text_CharacterConstitution
Text_CharacterIntelligence
Text_CharacterWisdom
Text_CharacterCharisma
```

Les bonus d'équipement restent affichés par le format existant `valeur finale (+bonus)`.

### Ressources et charge

UI-CHAR02 ajoute les barres optionnelles :

```text
ProgressBar_CharacterHealth
ProgressBar_CharacterMana
ProgressBar_CharacterCarryWeight
```

Elles sont alimentées depuis les valeurs déjà autoritaires :

```text
Resources.CurrentHealth / DerivedStats.MaxHealth
Resources.CurrentMana   / DerivedStats.MaxMana
CurrentWeight           / MaxWeight
```

Le ratio est clampé dans `[0..1]`. Un maximum nul produit 0.

Le texte reste disponible en parallèle :

```text
Text_CharacterHealth
Text_CharacterMana
Text_CharacterCarryWeight
```

Le compteur de cases d'inventaire est désormais projeté vers :

```text
Text_CharacterInventorySlots
```

sous la forme :

```text
UsedInventorySlots / MaxInventorySlots
```

### Combat

Nouveaux bindings optionnels :

```text
Text_CharacterPhysicalArmor
Text_CharacterMagicalArmor
Text_CharacterInitiative
Text_CharacterAccuracy
Text_CharacterEvasion
```

Ils lisent strictement les champs canoniques déjà présents dans le résumé.

`Text_CharacterArmor` est conservé temporairement comme alias legacy de l'armure physique pendant la migration UMG.

### Résistances

Le champ manquant `PhysicalResistance` est désormais exposé via :

```text
Text_ResistancePhysical
```

Les champs déjà existants sont conservés :

```text
Text_ResistanceFire
Text_ResistanceIce
Text_ResistanceLightning
Text_ResistancePoison
Text_ResistanceHoly
Text_ResistanceNecrotic
Text_ResistanceArcane
```

## Ce qui n'est PAS inventé dans UI-CHAR02

La maquette de travail évoquait aussi dégâts, critique et autres statistiques avancées.

Le résumé autoritaire actuel ne possède pas de champs canoniques dédiés pour toutes ces valeurs. UI-CHAR02 ne fabrique donc aucun calcul UI local pour les simuler.

Règle :

> une statistique n'apparaît dans la feuille que lorsqu'elle existe dans une autorité runtime clairement définie.

Cette décision évite une seconde logique de combat cachée dans l'interface.

## Paper doll

Le paper doll existant reste la cible de l'équipement.

UI-CHAR02 ne remplace pas :

- les `SlotWidget_*` ;
- `RegisterEquipmentSlotWidget()` ;
- les règles de compatibilité item/slot ;
- le drag/drop existant.

Le layout manuel décrit dans `UI_INV2_CHARACTER_EQUIPMENT_PANEL.md` reste applicable.

Concernant l'image centrale, le runtime possède aujourd'hui un portrait autoritaire mais pas encore une donnée `FullBody` canonique persistée. Le ticket ne crée donc pas une nouvelle autorité visuelle. `Image_CharacterPortrait` peut être réutilisé pendant la transition visuelle ; l'art plein corps définitif sera traité lorsque sa source de données sera fixée.

## Layout UMG cible

Dans `Panel_CharacterSheet` :

```text
VerticalBox_CharacterSheet
├── HorizontalBox_PartySelector
│   └── PartyMember_1..6
├── HorizontalBox_CharacterBody
│   ├── Border_Attributes
│   │   └── FOR / DEX / CON / INT / SAG / CHA
│   ├── Border_PaperDoll
│   │   └── personnage + slots équipement
│   └── VerticalBox_CharacterStats
│       ├── Border_Identity
│       ├── Border_Vitals
│       │   ├── Health + ProgressBar_CharacterHealth
│       │   ├── Mana + ProgressBar_CharacterMana
│       │   └── Carry + ProgressBar_CharacterCarryWeight
│       ├── Border_Combat
│       │   ├── armure physique
│       │   ├── armure magique
│       │   ├── initiative
│       │   ├── précision
│       │   └── esquive
│       └── Border_Resistances
│           └── physique / feu / glace / foudre / poison / sacré / nécrotique / arcanique
```

La feuille reste dans le panneau gauche du workspace. Elle ne doit pas recouvrir la barre persistante UI-NAV01.

## Style

Le C++ ne fixe aucune couleur.

UMG reste responsable de :

- couleur normale ;
- sélection ;
- danger ;
- surcharge ;
- style des progress bars ;
- typographie ;
- icônes ;
- cadres.

Le C++ ne fournit que les valeurs.

## Automation

Filtre :

```text
Grimrock.UI.Character02
```

Tests :

```text
Grimrock.UI.Character02.SheetProjection
Grimrock.UI.Character02.ProgressClamp
```

Ils vérifient notamment :

- PV et mana current/max ;
- ratios de progress bar ;
- clamp des ratios ;
- compteur de slots ;
- armures physique/magique ;
- initiative, précision et esquive ;
- résistance physique.

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Character02"
```

Après la passe UMG, contrôler en PIE :

1. changement de personnage ;
2. identité et attributs synchronisés ;
3. PV/mana/charge synchronisés ;
4. barres cohérentes avec les textes ;
5. statistiques combat correctes ;
6. résistances correctes ;
7. équipement/drag-drop inchangés ;
8. barre de navigation toujours visible ;
9. aucun `BindWidget` critique.
