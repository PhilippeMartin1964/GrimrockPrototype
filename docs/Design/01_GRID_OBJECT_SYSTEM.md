# GrimrockPrototype — Système d’objets de donjon

## Objectif

Ce document définit le modèle cible des objets placés dans le donjon.

Le but est de simplifier l’architecture existante et de la rapprocher de l’esprit *Legend of Grimrock*, tout en gardant les objets concrets distincts dans l’éditeur.

---

## Principe validé

Un objet du donjon est défini par :

```text
Objet concret
-> Archétype
-> Catégorie
-> Classe runtime
-> État initial
-> Événements émis
-> Commandes acceptées
-> Liens logiques
```

Les objets doivent rester distincts dans la palette lorsqu’ils sont différents visuellement ou fonctionnellement pour le level designer.

Exemples :

```text
Button_Normal
Button_Secret
Button_Wall
```

sont trois objets différents dans la palette, mais peuvent partager :

```cpp
AGridButtonActor
```

---

## Catégories d’objets

Le projet doit rester conservateur et ne pas supprimer inutilement les catégories existantes.

Version recommandée :

```cpp
UENUM(BlueprintType)
enum class EGridObjectCategory : uint8
{
    None UMETA(DisplayName = "None"),

    Mechanism UMETA(DisplayName = "Mechanism"),
    Receptacle UMETA(DisplayName = "Receptacle"),
    Passage UMETA(DisplayName = "Passage"),
    Item UMETA(DisplayName = "Item"),
    Decoration UMETA(DisplayName = "Decoration"),

    Readable UMETA(DisplayName = "Readable"),
    Spawn UMETA(DisplayName = "Spawn"),
    Trigger UMETA(DisplayName = "Trigger"),
    Light UMETA(DisplayName = "Light"),
    Hazard UMETA(DisplayName = "Hazard")
};
```

Remarque : `Light` et `Hazard` peuvent être ajoutés plus tard si l’on veut rester minimaliste.

---

## Rôle des catégories

| Catégorie | Rôle |
|---|---|
| `Mechanism` | Objet déclencheur ou contrôlable : bouton, levier, plaque, timer |
| `Receptacle` | Objet recevant un item : alcôve, support de torche, autel, bol d’offrande, serrure |
| `Passage` | Objet modifiant ou bloquant le déplacement : porte, porte secrète, trappe, téléporteur |
| `Item` | Objet manipulable ou transportable : clé, pièce, torche |
| `Decoration` | Objet décoratif sans logique obligatoire |
| `Readable` | Objet lisible, narratif, notamment `WallInscription` |
| `Spawn` | Marqueur de génération : joueur, monstre, item |
| `Trigger` | Déclencheur invisible ou logique |
| `Light` | Objet ou effet lié à une source lumineuse |
| `Hazard` | Piège, danger, dégâts |

---

## Objets concrets minimaux

### Mécanismes

| Objet concret | Catégorie | Classe / base | Événements émis | État initial | Activé par |
|---|---|---|---|---|---|
| Bouton | `Mechanism` | `AGridButtonActor` | `OnActivate` | relâché / pressé | clic / Use |
| Bouton secret | `Mechanism` | `AGridButtonActor` | `OnActivate` | relâché / pressé | clic / Use |
| Bouton mural | `Mechanism` | `AGridButtonActor` | `OnActivate` | relâché / pressé | clic / Use |
| Levier | `Mechanism` | `AGridLeverActor` | `OnActivate`, `OnDeactivate`, `OnToggle` | activé / désactivé | clic / Use |
| Plaque de pression | `Mechanism` | `AGridPressurePlateActor` | `OnActivate`, `OnDeactivate`, `OnToggle` | désactivée par défaut | marcher dessus / item |
| Trigger de sol | `Trigger` | `AGridTriggerActor` à créer | `OnEnter`, `OnExit`, `OnActivate` | actif / inactif | marcher dessus |
| Rune magique | `Mechanism` / `Decoration` / `Light` | acteur générique ou futur `AGridRuneActor` | `OnActivate`, `OnDeactivate`, `OnToggle` | activée / désactivée | clic / marche / lien |
| Timer | `Mechanism` | `AGridTimerActor` à créer | `OnTimer` | arrêté | script / lien |

---

### Réceptacles

| Objet concret | Catégorie | Classe / base | Événements émis | État initial | Activé par |
|---|---|---|---|---|---|
| Réceptacle générique | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem`, `OnRemoveItem` | vide / occupé | ajout / retrait item |
| Alcove | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem`, `OnRemoveItem` | vide / occupée | ajout / retrait item |
| Support de torche | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem`, `OnRemoveItem` | vide / occupé | ajout / retrait torche |
| Autel | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem`, `OnRemoveItem` | vide / occupé | ajout / retrait item |
| Bol d’offrande | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem`, `OnRemoveItem` | vide / occupé | ajout / retrait item |
| Fente à pièce | `Receptacle` | `AGridReceptacleActor` | `OnInsertItem` | vide | ajout pièce |
| Serrure | `Receptacle` | `AGridReceptacleActor` ou futur `AGridLockActor` | `OnInsertItem`, `OnUnlock` | verrouillée | ajout clé |

---

### Passages

| Objet concret | Catégorie | Classe / base | État initial | Commandes principales |
|---|---|---|---|---|
| Porte | `Passage` | `AGridDoorActor` | ouverte / fermée / verrouillée | `Open`, `Close`, `ToggleOpen`, `Lock`, `Unlock` |
| Porte secrète | `Passage` | `AGridSecretDoorActor` | ouverte / fermée / cachée | `Open`, `Close`, `ToggleOpen` |
| Trappe | `Passage` | futur `AGridTrapdoorActor` | ouverte / fermée | `Open`, `Close`, `ToggleOpen` |
| Téléporteur | `Passage` | futur `AGridTeleporterActor` | activé / désactivé | `Enable`, `Disable`, `Toggle`, `Teleport` |

---

### Items

| Objet concret | Catégorie | Classe / base | État initial | Usage |
|---|---|---|---|---|
| Clé | `Item` | futur `AGridItemActor` | N/A | serrure |
| Pièce | `Item` | futur `AGridItemActor` | N/A | fente à pièce / offrande |
| Torche | `Item` ou `Light` | futur `AGridItemActor` | allumée / éteinte si nécessaire | inventaire / support de torche |

---

### Readable et Spawn

| Objet concret | Catégorie | Classe / base | Événements émis | Remarque |
|---|---|---|---|---|
| WallInscription | `Readable` | système existant WallInscription | `OnUse` | ne pas renommer |
| Spawn joueur | `Spawn` | marker / données de niveau | aucun | position initiale |
| Spawn monstre | `Spawn` | futur système spawn | `OnSpawn` optionnel | génération future |
| Spawn item | `Spawn` | futur système spawn | optionnel | énigmes / scripts |

---

## Hiérarchie C++ recommandée

Hiérarchie cible, sans obligation de tout créer immédiatement :

```text
AActor
└── AGridObjectActorBase
    ├── AGridButtonActor
    ├── AGridLeverActor
    ├── AGridPressurePlateActor
    ├── AGridTriggerActor
    ├── AGridReceptacleActor
    ├── AGridDoorActor
    │   └── AGridSecretDoorActor
    ├── AGridTeleporterActor
    ├── AGridTimerActor
    ├── AGridItemActor
    └── AGridReadableActor / système WallInscription existant
```

Court terme :

```text
AGridButtonActor
AGridLeverActor
AGridPressurePlateActor
AGridReceptacleActor
AGridDoorActor
AGridSecretDoorActor
WallInscription existant
```

Plus tard :

```text
AGridTriggerActor
AGridTimerActor
AGridTeleporterActor
AGridItemActor
```

---

## Règles importantes

1. Ne pas multiplier inutilement les classes C++.
2. Garder les objets distincts dans les archétypes et la palette.
3. Factoriser le comportement par classes communes.
4. Ne pas faire communiquer directement les objets entre eux.
5. Passer par un système central `Event -> Command`.
6. Conserver `WallInscription` comme objet existant.
7. Conserver `Spawn` et `Readable` dans les catégories.
8. Ajouter explicitement `Door_Secret` / porte secrète comme objet `Passage`.
9. Conserver `Alcove`, `TorchHolder`, `Altar`, `OfferingBowl` comme réceptacles concrets.

